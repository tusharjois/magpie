#include "time.h"
#include "stdio.h"

#include "magpielib.h"
#include "helper.h"
#include "keys.h"

//"frontend" functions

int magpie_setup_context(struct magpie_context* context, char* key_filepath, int is_server, char* logger_level) {
    logger(TRACE, "Entered context setup");
    //everything null to start
    memset(context, 0, sizeof(struct magpie_context));

    // Initialize logger
    int level = get_logger_level(logger_level);
    logger_init(level, false);

    // Initialize LibHydrogen
    if (hydro_init() != 0)
    {
        logger(FATAL, "LibHydrogen failed to initialize. Aborting :(\n");
        return HC_LIBHYDROGEN_ERROR;
    }

    //make sure key_filepath exists
    FILE *file;
    if ((file = fopen(key_filepath, "r"))) {
        fclose(file); 
    } else {
        logger(FATAL, "Incorrect or nonexistent key_filepath: %s - Aborting :(\n", key_filepath);
        return HC_KEYFILE_ERROR;
    }

    // Setup local Diffie-Hellman keypair
    load_hydro_kx_keypair(&context->local_kp, key_filepath);
    char buffer[MAX_STRING_LEN];
    memset(buffer, 0, MAX_STRING_LEN);
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

    context->rx_seq_num = context->tx_seq_num = INIT_SEQ_NUM;

    return HC_OKAY;
}

int magpie_set_input_buffer(struct magpie_context* context, void* buffer, int buffer_len) {
    logger(DEBUG, "Setting the input buffer");
    set_buffer(&context->send_buffer, buffer, buffer_len);
    return 0;
}

int magpie_set_output_buffer(struct magpie_context* context, void* buffer, int buffer_len) {
    logger(DEBUG, "Setting the output buffer");
    set_buffer(&context->recv_buffer, buffer, buffer_len);
    return 0;
}

int magpie_generate_packet(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "generate_packet() [ state=%d is_server=%d tx=%d rx=%d ]", context->state, context->is_server, context->tx_seq_num, context->rx_seq_num);

    //Generate a handshake packet based on the state
    if (context->state == AWAITING_BEGIN)
        return generate_handshake_xx_1(context, packet);
    if (context->state == AWAITING_XX_1)
        return generate_handshake_xx_2(context, packet);
    if (context->state == AWAITING_XX_2)
        return generate_handshake_xx_3(context, packet);
    if (context->state == AWAITING_XX_3)
        return HC_OKAY;

    //otherwise, generate a normal packet containing data from the send buffer
    struct magpie_message message;
    memset(&message, 0, sizeof(struct magpie_message));
    message.num_bytes = read_from_mag_buffer(message.payload, &context->send_buffer, PAYLOAD_SIZE);
    logger(TRACE, "bytes=%d\n", message.num_bytes);
    message.type = TRANSFER;
    message.is_last_packet = context->send_buffer.is_empty;
    if (encrypt_packet(&message, packet, context) < 0) {
        logger(ERROR, "Packet failed to encrypt");
        return HC_ENCRYPTION_FAILED;
    }
    if (context->send_buffer.is_empty)
        return HC_TRANSFER_COMPELTE;

    return HC_ONE_TO_SEND;
}

int magpie_handle_packet(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "handle_packet() [ state=%d is_server=%d tx=%d rx=%d ]", context->state, context->is_server, context->tx_seq_num, context->rx_seq_num);

    //we might receive our own packet on a broadcast network
    //if (packet->meta.sender_id != context->local_id) {
    //    logger(DEBUG, "ignoring packet from ourselves");
    //    return HC_OKAY;
    //}

    //Handle a handshake packet based on the state
    if (context->state == AWAITING_BEGIN) {
        return HC_OKAY;
    }
    if (context->state == AWAITING_XX_1) {
        return handle_handshake_xx_1(context, packet);
    }
    if (context->state == AWAITING_XX_2) {
        return handle_handshake_xx_2(context, packet);
    }
    if (context->state == AWAITING_XX_3) {
        return handle_handshake_xx_3(context, packet);
    }
    
    //otherwise, handle the normal packet and write the plaintext to the receive buffer
    struct magpie_message message;
    memset(&message, 0, sizeof(struct magpie_message));
    if (decrypt_packet(&message, packet, context) < 0) {
        logger(ERROR, "Packet failed to decrypt");
        return HC_DECRYPTION_FAILED;
    }
    write_to_mag_buffer(&context->recv_buffer, message.payload, message.num_bytes);
    logger(TRACE, "message payload size: %d", sizeof(message.payload));
    logger(TRACE, "%d %d %d\n", message.num_bytes, message.is_last_packet, message.type);

    logger(DEBUG, "finished handling");
    if (message.is_last_packet)
        return HC_TRANSFER_COMPELTE;
    else 
        return HC_OKAY;
}

//"backend" functions

int set_buffer(struct magpie_buffer* mag_buffer, void* buffer, int buffer_len) {
    memset(mag_buffer, 0, sizeof(struct magpie_buffer));
    mag_buffer->buffer_len = buffer_len;
    mag_buffer->is_io_buffer = buffer_len == 0;
    mag_buffer->is_empty = 0;
    if (mag_buffer->is_io_buffer)
        mag_buffer->io_buffer = (FILE*) buffer;
    else {
        mag_buffer->char_buffer = (char*) buffer;
    }

    return 0;
}

int generate_handshake_xx_1(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "Generating handshake 1 packet");
    if (context->state != AWAITING_BEGIN) {
        logger(ERROR, "Incorrect state, something went wrong\n");
        return HC_INCORRECT_CONTEXT_STATE;
    }

    // Create and send first packet
    int ret = hydro_kx_xx_1(&context->hydro_state, context->handshake_xx_1, NULL);
    if (ret < 0) {
        logger(FATAL, "Libhydrogen error\n");
        return HC_LIBHYDROGEN_ERROR;
    }

    packet->meta.sender_id = packet->meta.sender_id;
    packet->meta.seq_num = HANDSHAKE_XX_1_SEQ_NUM;
    memcpy(packet->ciphertext, context->handshake_xx_1, hydro_kx_XX_PACKET1BYTES);
    context->state = AWAITING_XX_2;
    context->tx_seq_num++;

    return HC_ONE_TO_SEND;
}

int handle_handshake_xx_1(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "Received handshake 1 from client [ state=%d seq=%d ]", context->state, packet->meta.seq_num);

    //check that server is ready for xx1
    if (context->state != AWAITING_XX_1) {
        logger(ERROR, "Incorrect state, something went wrong\n");
        return HC_INCORRECT_CONTEXT_STATE;
    }

    if (packet->meta.seq_num != HANDSHAKE_XX_1_SEQ_NUM) {
        logger(ERROR, "Incorrect sequence num, something went wrong\n");
        return HC_INCORRECT_PACKET_TYPE;
    } 
    
    //process packet
    logger(DEBUG, "copying to context buffer");
    memcpy(context->handshake_xx_1, packet->ciphertext, hydro_kx_XX_PACKET1BYTES);
    logger(DEBUG, "creating second handshake");
    if (hydro_kx_xx_2(&context->hydro_state, context->handshake_xx_2, context->handshake_xx_1, NULL, &context->local_kp) == 0) {
        context->handshake_xx_1_done = 1;
        context->rx_seq_num++;
        return HC_OKAY;
    } else {
        logger(WARN, "Failed to process client's first handshake packet");
        return HC_LIBHYDROGEN_ERROR;
    }
}

int generate_handshake_xx_2(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "Generating handshake 2");
    if (!context->handshake_xx_1_done)
        return HC_OKAY;

    // Construct new packet
    packet->meta.sender_id = context->local_id;
    packet->meta.seq_num = HANDSHAKE_XX_2_SEQ_NUM;
    memcpy(packet->ciphertext, context->handshake_xx_2, hydro_kx_XX_PACKET2BYTES);
    context->state = AWAITING_XX_3;
    context->tx_seq_num++;

    return HC_ONE_TO_SEND;
}

int handle_handshake_xx_2(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "Received handshake 2 from server [ state=%d seq=%d ]", context->state, packet->meta.seq_num);
    //check that client is ready for xx2
    if (context->state != AWAITING_XX_2)
        return HC_INCORRECT_CONTEXT_STATE;
    
    if (packet->meta.seq_num != HANDSHAKE_XX_2_SEQ_NUM)
        return HC_INCORRECT_PACKET_TYPE;

    //process packet
    memcpy(context->handshake_xx_2, packet->ciphertext, hydro_kx_XX_PACKET2BYTES);
    if (hydro_kx_xx_3(&context->hydro_state, &context->session_kp, context->handshake_xx_3, 
        NULL, context->handshake_xx_2, NULL, &context->local_kp) == 0
    ) {
        context->handshake_xx_2_done = 1;
        context->rx_seq_num++;
        return HC_OKAY;
    } else {
        logger(WARN, "Failed to process server's second handshake packet");
        return HC_LIBHYDROGEN_ERROR;
    }

    return HC_UNKNOWN_ERROR;   
}

int generate_handshake_xx_3(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "Generating handshake 3");
    if (!context->handshake_xx_2_done)
        return HC_OKAY;

    packet->meta.sender_id = context->local_id;
    packet->meta.seq_num = HANDSHAKE_XX_3_SEQ_NUM;
    memcpy(packet->ciphertext, context->handshake_xx_3, hydro_kx_XX_PACKET3BYTES);
    // Done! session_kp.tx is the key for sending data to the server,
    // and session_kp.rx is the key for receiving data from the server.
    context->state = READY;
    context->tx_seq_num++;

    logger(INFO, "Ready to send communication data");
    return HC_MORE_TO_SEND; // We start sending immediately after sending XX_3
}

int handle_handshake_xx_3(struct magpie_context* context, struct magpie_packet* packet) {
    logger(DEBUG, "Received handshake 3 from client [ state=%d seq=%d ]", context->state, packet->meta.seq_num);

    //check that client is ready for xx3
    if (context->state != AWAITING_XX_3)
        return HC_INCORRECT_CONTEXT_STATE;

    if (packet->meta.seq_num != HANDSHAKE_XX_3_SEQ_NUM)
        return HC_INCORRECT_PACKET_TYPE;

    //process packet 3
    memcpy(context->handshake_xx_3, packet->ciphertext, hydro_kx_XX_PACKET3BYTES);
    if (hydro_kx_xx_4(&context->hydro_state, &context->session_kp, NULL, context->handshake_xx_3, NULL) == 0) {
        logger(INFO, "Ready to receive communication data");
        context->state = READY;
        context->rx_seq_num++;
        logger(DEBUG, "current server [ state=%d is_server=%d tx=%d rx=%d ]", context->state, context->is_server, context->tx_seq_num, context->rx_seq_num);
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
    logger(DEBUG, "Beginning encryption sequence");
    char* plaintext = calloc(PLAINTEXT_SIZE, sizeof(char));
    char* ciphertext = calloc(CIPHERTEXT_SIZE, sizeof(char));

    encrypted_packet->meta.sender_id = context->local_id;
    encrypted_packet->meta.seq_num = context->tx_seq_num + 1;
    
    //create the meta hash
    logger(DEBUG, "Creating the meta hash");
    plaintext_message->meta_hash = hash((char*) &encrypted_packet->meta, sizeof(struct magpie_meta));

    //encrypt
    logger(DEBUG, "Encrypting the packet");
    memcpy(plaintext, plaintext_message, sizeof(struct magpie_message));
    hydro_secretbox_encrypt((uint8_t *)ciphertext, plaintext, PLAINTEXT_SIZE, 0, CONTEXT, context->session_kp.tx);
    memcpy(encrypted_packet->ciphertext, ciphertext, CIPHERTEXT_SIZE);

    //create the probe
    logger(DEBUG, "Creating the probe");
    hydro_secretbox_probe_create(encrypted_packet->probe, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.tx);

    // update tx_seq_num
    context->tx_seq_num++;
    
    free(plaintext);
    free(ciphertext);
    
    return HC_OKAY;
} 

int decrypt_packet(struct magpie_message* plaintext_message, struct magpie_packet* encrypted_packet, struct magpie_context* context) {
    logger(DEBUG, "Beginning decryption sequence");
    char* ciphertext = encrypted_packet->ciphertext;

    //check for correct order (Cannot decrypt out of order packets. Also, you cannot decrypt the same packet multiple times)
    if (encrypted_packet->meta.seq_num != context->rx_seq_num + 1) {
        logger(DEBUG, "Out of sequence packet (Expecting: %d, Actual: %d)", context->rx_seq_num + 1, encrypted_packet->meta.seq_num);
        return HC_DECRYPTION_OUT_OF_ORDER;
    }
    
    //check the probe
    logger(DEBUG, "Verifying the probe");
    int ret = hydro_secretbox_probe_verify(encrypted_packet->probe, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.rx);
    if (ret != 0) {
        logger(WARN, "Probe Failed to Verify");
        return HC_DECRYPTION_PROBE_FAILED;
    }

    //decrypt packet
    logger(DEBUG, "Decrypting the packet");
    char* plaintext = calloc(PLAINTEXT_SIZE, sizeof(char));
    if (hydro_secretbox_decrypt(plaintext, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, 0, CONTEXT, context->session_kp.rx) != 0) {
        logger(WARN, "Decrypt failed, Message forged!");
        free(plaintext);
        return HC_DECRYPTION_MESSAGE_FORGED;
    } 

    //verify meta hash
    logger(DEBUG, "Verifying the meta hash");
    unsigned int meta_rehash = hash((char*) &encrypted_packet->meta, sizeof(struct magpie_meta));
    if (((struct magpie_message*)plaintext)->meta_hash != meta_rehash) {
        logger(WARN, "Meta hash mismatch, Message forged!");
        free(plaintext);
        return HC_DECRYPTION_HASH_FORGED;
    }

    context->rx_seq_num++;
    memcpy(plaintext_message, plaintext, sizeof(struct magpie_message));
    free(plaintext);
    return HC_OKAY;

}
