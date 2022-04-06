#include "magpielib.h"
#include "logger.h"
#include "time.h"

int setup_context(struct magpie_context* context, char* key_filepath, int is_server) {
    //everything null to start
    memset(context, 0, sizeof(struct magpie_context));

    // Initialize logger
    logger_init(TRACE, false);

    // Initialize LibHydrogen
    if (hydro_init() != 0)
    {
        logger(FATAL, "LibHydrogen failed to initialize. Aborting :(");
        exit(1);
    }

    // Setup local Diffie-Hellman keypair
    load_hydro_kx_keypair(&context->local_kp, key_filepath);
    char buffer[MAX_STRING_LEN];
    format_keypair(buffer, &context->local_kp);
    logger(DEBUG, "%s keypair:\n%s", is_server ? "Server" : "Client", buffer);

    // Randomly select a local_id
    char randomness[MAX_STRING_LEN];
    memset(randomness, 77, MAX_STRING_LEN);
    sprintf(randomness, "%d__%d__%ld", rand(), rand(), time(NULL));
    context->local_id = hash(randomness, MAX_STRING_LEN);

    // Set initial state based on whether or not we are the server
    context->is_server = is_server;
    context->state = context->is_server ? AWAITING_XX_1 : AWAITING_BEGIN;
    context->send_buffer.is_empty = 1;  // Default to empty send buffer

    context->rx_seq_num = context->tx_seq_num = TRANSFER_START_SEQ_NUM;

    return 0;
}

int set_buffer(struct magpie_buffer* mag_buffer, void* buffer, int buffer_len) {
    memset(mag_buffer, 0, sizeof(struct magpie_buffer));
    mag_buffer->buffer_len = buffer_len;
    mag_buffer->is_io_buffer = buffer_len == 0;
    mag_buffer->is_empty = 0;
    if (mag_buffer->is_io_buffer)
        mag_buffer->io_buffer = (FILE*) buffer;
    else {
        // deep copy data in case buffer is freed
        mag_buffer->char_buffer = (char*) buffer;
        //mag_buffer->char_buffer = (char*) calloc(buffer_len, sizeof(char));
        //memcpy(mag_buffer->char_buffer, buffer, buffer_len);
    }

    return 0;
}

int set_input_buffer(struct magpie_context* context, void* buffer, int buffer_len) {
    set_buffer(&context->send_buffer, buffer, buffer_len);
    return 0;
}

int set_output_buffer(struct magpie_context* context, void* buffer, int buffer_len) {
    set_buffer(&context->recv_buffer, buffer, buffer_len);
    return 0;
}

int read_from_mag_buffer(char* out_buffer, struct magpie_buffer* in_mag_buffer, int num_bytes) {
    if (num_bytes <= 0)
        return 0;

    int n = 0;
    if (in_mag_buffer->is_io_buffer) {
        char c;
        while ((c = fgetc(in_mag_buffer->io_buffer)) != EOF) {
            out_buffer[n++] = c;
            if (n >= num_bytes)
                break;
        }
        if (c == EOF)
            in_mag_buffer->is_empty = 1;
    } else {
        if (in_mag_buffer->buffer_len - in_mag_buffer->num_bytes_read < num_bytes)
            n = in_mag_buffer->buffer_len - in_mag_buffer->num_bytes_read;
        else
            n = num_bytes;
        memcpy(out_buffer, &in_mag_buffer->char_buffer[in_mag_buffer->num_bytes_read], n);
        in_mag_buffer->num_bytes_read += n;
        if (in_mag_buffer->buffer_len == in_mag_buffer->num_bytes_read)
            in_mag_buffer->is_empty = 1;
    }
    return n;
}

int write_to_mag_buffer(struct magpie_buffer* out_mag_buffer, char* in_buffer, int num_bytes) {
    if (num_bytes <= 0)
        return 0;

    if (out_mag_buffer->is_io_buffer) {
        for(int i = 0; i < num_bytes; i++)
            fputc(in_buffer[i], out_mag_buffer->io_buffer);
    } else {
        memcpy(&out_mag_buffer->char_buffer[out_mag_buffer->num_bytes_read], in_buffer, num_bytes);
        out_mag_buffer->num_bytes_read += num_bytes;
    }
    return num_bytes;
}

int generate_handshake_xx_1(struct magpie_context* context, struct magpie_packet* packet) {
    if (context->state != AWAITING_BEGIN)
        return HC_INCORRECT_CONTEXT_STATE;

    // Create and send first packet
    int ret = hydro_kx_xx_1(&context->hydro_state, context->handshake_xx_1, NULL);
    if (ret < 0)
        return HC_LIBHYDROGEN_ERROR;

    packet->meta.sender_id = packet->meta.sender_id;
    packet->meta.seq_num = HANDSHAKE_XX_1_SEQ_NUM;
    memcpy(packet->ciphertext, context->handshake_xx_1, hydro_kx_XX_PACKET1BYTES);
    context->state = AWAITING_XX_2;

    return HC_ONE_TO_SEND;
}

int handle_handshake_xx_1(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "Received handshake 1 from client, sending handshake 2...");

    //check that server is ready for xx1
    if (context->state != AWAITING_XX_1)
        return HC_INCORRECT_CONTEXT_STATE;

    if (packet->meta.seq_num != HANDSHAKE_XX_1_SEQ_NUM)
        return HC_INCORRECT_PACKET_TYPE;
    
    //process packet
    memcpy(context->handshake_xx_1, packet->ciphertext, hydro_kx_XX_PACKET1BYTES);
    if (hydro_kx_xx_2(&context->hydro_state, context->handshake_xx_2, context->handshake_xx_1, NULL, &context->local_kp) == 0) {
        context->handshake_xx_1_done = 1;
        return HC_OKAY;
    } else {
        logger(WARN, "Failed to process client's first handshake packet");
        return HC_LIBHYDROGEN_ERROR;
    }
}

int generate_handshake_xx_2(struct magpie_context* context, struct magpie_packet* packet) {
    if (!context->handshake_xx_1_done)
        return HC_OKAY;

    // Construct new packet
    packet->meta.sender_id = context->local_id;
    packet->meta.seq_num = HANDSHAKE_XX_2_SEQ_NUM;
    memcpy(packet->ciphertext, context->handshake_xx_2, hydro_kx_XX_PACKET2BYTES);
    context->state = AWAITING_XX_3;
    // Done! session_kp.tx is the key for sending data to the server,
    // and session_kp.rx is the key for receiving data from the server.

    return HC_ONE_TO_SEND;
}

int handle_handshake_xx_2(struct magpie_context* context, struct magpie_packet* packet) {
    //check that client is ready for xx2
    if (context->state != AWAITING_XX_2)
        return HC_INCORRECT_CONTEXT_STATE;
    
    if (packet->meta.seq_num != HANDSHAKE_XX_2_SEQ_NUM)
        return HC_INCORRECT_PACKET_TYPE;

    logger(DEBUG, "Received handshake 2 from server");
    //process packet
    memcpy(context->handshake_xx_2, packet->ciphertext, hydro_kx_XX_PACKET2BYTES);
    if (hydro_kx_xx_3(&context->hydro_state, &context->session_kp, context->handshake_xx_3, 
        NULL, context->handshake_xx_2, NULL, &context->local_kp) == 0
    ) {
        context->handshake_xx_2_done = 1;
        return HC_OKAY;
    } else {
        logger(WARN, "Failed to process server's second handshake packet");
        return HC_LIBHYDROGEN_ERROR;
    }

    return HC_UNKNOWN_ERROR;   
}

int generate_handshake_xx_3(struct magpie_context* context, struct magpie_packet* packet) {
    if (!context->handshake_xx_2_done)
        return HC_OKAY;

    logger(DEBUG, "Sending packet 3 from client to server");

    packet->meta.sender_id = context->local_id;
    packet->meta.seq_num = HANDSHAKE_XX_3_SEQ_NUM;
    memcpy(packet->ciphertext, context->handshake_xx_3, hydro_kx_XX_PACKET3BYTES);
    // Done! session_kp.tx is the key for sending data to the server,
    // and session_kp.rx is the key for receiving data from the server.
    context->state = READY;

    return HC_MORE_TO_SEND; // We start sending immediately after sending XX_3
}

int handle_handshake_xx_3(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "Received handshake 3 from client [ state=% seq=%d ]", context->state, packet->meta.seq_num);

    //check that client is ready for xx3
    if (context->state != AWAITING_XX_3)
        return HC_INCORRECT_CONTEXT_STATE;

    if (packet->meta.seq_num != HANDSHAKE_XX_3_SEQ_NUM)
        return HC_INCORRECT_PACKET_TYPE;

    //check that the packet type is xx3
    //if (packet->type != HANDSHAKE_XX_3) {
    //    logger(DEBUG, "Received a packet with incorrect type (expecting handshake 3)");
    //    return HC_INCORRECT_PACKET_TYPE;
    //}

    //process packet 3
    memcpy(context->handshake_xx_3, packet->ciphertext, hydro_kx_XX_PACKET3BYTES);
    if (hydro_kx_xx_4(&context->hydro_state, &context->session_kp, NULL, context->handshake_xx_3, NULL) == 0) {
        logger(INFO, "Ready to receive");
        context->state = READY;
        return HC_OKAY;
        // Done! session_kp.tx is the key for sending data to the client,
        // and session_kp.rx is the key for receiving data from the client.
        // The session keys are the same as those computed by the client, but swapped.
    } else {
        logger(WARN, "Failed to process client handshake request packet 3");
        return HC_LIBHYDROGEN_ERROR;
    }
}

int encrypt_packet(struct magpie_message* plaintext_message, struct magpie_packet* encrypted_packet, struct magpie_context* context) {
    char* plaintext = calloc(PLAINTEXT_SIZE, sizeof(char));
    char* ciphertext = calloc(CIPHERTEXT_SIZE, sizeof(char));

    encrypted_packet->meta.sender_id = context->local_id;
    encrypted_packet->meta.seq_num = context->tx_seq_num + 1;
    // printf("is_server=%d tx_num=%d rx_num=%d\n", context->is_server, context->tx_seq_num, context->rx_seq_num);
    plaintext_message->meta_hash = hash((char*) &encrypted_packet->meta, sizeof(struct magpie_meta));

    memcpy(plaintext, plaintext_message, sizeof(struct magpie_message));
    hydro_secretbox_encrypt((uint8_t *)ciphertext, plaintext, PLAINTEXT_SIZE, 0, CONTEXT, context->session_kp.tx);
    memcpy(encrypted_packet->ciphertext, ciphertext, CIPHERTEXT_SIZE);

    //create the probe
    hydro_secretbox_probe_create(encrypted_packet->probe, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.tx);

    // update tx_seq_num
    context->tx_seq_num++;
    
    free(plaintext);
    free(ciphertext);
    
    return HC_OKAY;
} 

/* decrypt the packet after receiving, include probe checking */
int decrypt_packet(struct magpie_message* plaintext_message, struct magpie_packet* encrypted_packet, struct magpie_context* context) {
    char* ciphertext = encrypted_packet->ciphertext;
    // printf("is_server=%d tx_num=%d rx_num=%d\n", context->is_server, context->tx_seq_num, context->rx_seq_num);

    //check for correct order (Cannot decrypt out of order packets. Also, you cannot decrypt the same packet multiple times)
    if (encrypted_packet->meta.seq_num != context->rx_seq_num + 1) {
        logger(DEBUG, "Out of sequence packet (Expecting: %d, Actual: %d)", context->rx_seq_num + 1, encrypted_packet->meta.seq_num);
        return HC_DECRYPTION_OUT_OF_ORDER;
    }
    
    //check the probe
    int ret = hydro_secretbox_probe_verify(encrypted_packet->probe, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.rx);
    if (ret != 0) {
        logger(DEBUG, "Probe Failed to Verify");
        return HC_DECRYPTION_PROBE_FAILED;
    }

    //decrypt packet
    char* plaintext = calloc(PLAINTEXT_SIZE, sizeof(char));
    if (hydro_secretbox_decrypt(plaintext, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, 0, CONTEXT, context->session_kp.rx) != 0) {
        logger(DEBUG, "Message forged!");
        free(plaintext);
        return HC_DECRYPTION_MESSAGE_FORGED;
    } 

    context->rx_seq_num++;

    memcpy(plaintext_message, plaintext, sizeof(struct magpie_message));
    free(plaintext);
    return HC_OKAY;

}

int generate_packet(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "generate_packet() [ state=%d is_server=%d tx=%d rx=%d ]", context->state, context->is_server, context->tx_seq_num, context->rx_seq_num);

    if (context->state == AWAITING_BEGIN)
        return generate_handshake_xx_1(context, packet);
    if (context->state == AWAITING_XX_1)
        return generate_handshake_xx_2(context, packet);
    if (context->state == AWAITING_XX_2)
        return generate_handshake_xx_3(context, packet);
    if (context->state == AWAITING_XX_3)
        return HC_OKAY;

    if (context->send_buffer.is_empty)
        return HC_TRANSFER_COMPELTE;

    struct magpie_message message;
    message.num_bytes = read_from_mag_buffer(message.payload, &context->send_buffer, PAYLOAD_SIZE);
    // printf("bytes=%d\n", message.num_bytes);
    message.type = TRANSFER;
    message.is_last_packet = context->send_buffer.is_empty;
    if (encrypt_packet(&message, packet, context) < 0)
        return HC_ENCRYPTION_FAILED;
    
    //if (message.is_last_packet)
    //    return HC_LAST_TO_SEND;
    //else 
    return HC_ONE_TO_SEND;
}

int handle_packet(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "handle_packet() [ state=%d is_server=%d tx=%d rx=%d ]", context->state, context->is_server, context->tx_seq_num, context->rx_seq_num);

    if (packet->meta.sender_id != context->local_id) {
        logger(DEBUG, "ignoring packet fro ourselves");
        return HC_OKAY;
    }

    if (context->state == AWAITING_BEGIN)
        return HC_OKAY;
    if (context->state == AWAITING_XX_1)
        return handle_handshake_xx_1(context, packet);
    if (context->state == AWAITING_XX_2)
        return handle_handshake_xx_2(context, packet);
    if (context->state == AWAITING_XX_3)
        return handle_handshake_xx_3(context, packet);
    
    struct magpie_message message;
    if (decrypt_packet(&message, packet, context) < 0)
        return HC_ENCRYPTION_FAILED;
    write_to_mag_buffer(&context->recv_buffer, message.payload, message.num_bytes);
    //printf("%d %d %d\n", message.num_bytes, message.is_last_packet, message.type);

    if (message.is_last_packet)
        return HC_TRANSFER_COMPELTE;
    else 
        return HC_OKAY;
}

unsigned int hash(char *str, int len) {
    unsigned int hash = 5381;
    int c;
    if (len==0) {
        while ((c = *str++))
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    } else {
        for (int i = 0; i < len; i++)
            hash = ((hash << 5) + hash) + str[i]; /* hash * 33 + c */
    }
    return hash;
}

int load_hydro_kx_keypair(hydro_kx_keypair* kp, char* filepath) {
    FILE* fd;
    fd = fopen(filepath, "r");

    fread(kp->pk, hydro_kx_PUBLICKEYBYTES, sizeof(char), fd);
    fread(kp->sk, hydro_kx_SECRETKEYBYTES, sizeof(char), fd);
    fclose(fd);
    return 0;
}

void format_keypair(char* buffer, hydro_kx_keypair* kp) {
    memset(buffer, '\0', strlen(buffer));
    sprintf(&buffer[strlen(buffer)], "public = {");
    for (int i=0; i < hydro_kx_PUBLICKEYBYTES; i++) {
      sprintf(&buffer[strlen(buffer)], "%d, ", kp->pk[i]);
    }
    sprintf(&buffer[strlen(buffer)], "}");
    sprintf(&buffer[strlen(buffer)], "\nprivate = ");
    sprintf(&buffer[strlen(buffer)], "{");
    for (int i=0; i < hydro_kx_SECRETKEYBYTES; i++) {
      sprintf(&buffer[strlen(buffer)], "%d, ", kp->sk[i]);
    }
    sprintf(&buffer[strlen(buffer)], "}");
}

void format_ip_address(char* buffer, int ip) {
    sprintf(buffer, "%d.%d.%d.%d", 
        (htonl(ip) & 0xff000000)>>24,
        (htonl(ip) & 0x00ff0000)>>16,
        (htonl(ip) & 0x0000ff00)>>8,
        (htonl(ip) & 0x000000ff));
}