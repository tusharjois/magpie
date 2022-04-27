//clientlib.c

#include "clientlib.h"
#include "messages.h"
#include "helper.h"


int create_handshake_xx_1(struct magpie_packet* packet, struct magpie_context* context) {
    // Create and send first packet
    int ret = hydro_kx_xx_1(&context->hydro_state, context->handshake_xx_1, NULL);
    if (ret < 0)
        return HC_LIBHYDROGEN_ERROR;

    packet->sender_id = context->local_ip;
    packet->type = HANDSHAKE_XX_1;
    memcpy(packet->payload, context->handshake_xx_1, hydro_kx_XX_PACKET1BYTES);
    
    return HC_ONE_TO_SEND;
}

int handle_handshake_xx_2(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context) {

    //check that client is ready for xx2
    if (context->state != AWAITING_XX_2) {
        logger(DEBUG, "Incorrect context state");
        return HC_INCORRECT_CONTEXT_STATE;
    }

    logger(DEBUG, "Received handshake 2 from server");
    //process packet
    memcpy(context->handshake_xx_2, packet->payload, hydro_kx_XX_PACKET2BYTES);
    if (hydro_kx_xx_3(&context->hydro_state, &context->session_kp, context->handshake_xx_3, NULL, context->handshake_xx_2, NULL,
                &context->local_kp) == 0) 
        {
        logger(DEBUG, "Sending packet 3 from client to server");

        out_packet->sender_id = context->local_id;
        out_packet->type = HANDSHAKE_XX_3;
        memcpy(out_packet->payload, context->handshake_xx_3, hydro_kx_XX_PACKET3BYTES);
        // Done! session_kp.tx is the key for sending data to the server,
        // and session_kp.rx is the key for receiving data from the server.
        context->state = READY;

        return HC_MORE_TO_SEND; // We start sending immediately after sending XX_3
    } else {
        logger(WARN, "Failed to process server's second handshake packet");
        return HC_LIBHYDROGEN_ERROR;
    }

    return HC_UNKNOWN_ERROR; 
}

/*
int create_handshake_xx_3(struct magpie_packet* packet, struct magpie_context* context) {

    packet->sender_id = context->local_ip;
    packet->type = HANDSHAKE_XX_3;
    memcpy(packet->payload, context->handshake_xx_3, hydro_kx_XX_PACKET3BYTES);
    // Done! session_kp.tx is the key for sending data to the server,
    // and session_kp.rx is the key for receiving data from the server.

    return HC_ONE_TO_SEND;
}
*/


/*int client_handle_test_messages(struct magpie_packet* packet, struct magpie_context* context) {

    //check that client is ready for test messages
    if (context->state != TEST) {
        return -1;
    }

    //check for correct order
    if (packet->seq_num != context->rx_seq_num + 1) {
        logger(DEBUG, "Out of sequence packet (Expecting: %d, Actual: %d)", context->rx_seq_num + 1, packet->seq_num);
        return -3;
    }
    
    char plaintext[PLAINTEXT_SIZE];
    char ciphertext[CIPHERTEXT_SIZE];
    memset(plaintext, 0, PLAINTEXT_SIZE);
    memcpy(ciphertext, packet->payload, CIPHERTEXT_SIZE);

    //check the probe
    int ret = hydro_secretbox_probe_verify(packet->probe, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.rx);
    if (ret != 0) {
        logger(DEBUG, "Probe Failed to Verify");
        return -4;
    }

    //decrypt packet
    if (hydro_secretbox_decrypt(plaintext, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, 0, CONTEXT, context->session_kp.rx) != 0) {
        logger(DEBUG, "Message forged!");
    } else {
        struct TestRequest req;
        memcpy(&req, plaintext, sizeof(struct TestRequest));
        logger(TRACE, "Message content: %d", req.num);
        context->rx_seq_num++;
        client_send_test_message(packet, context, req.num + 1);
    }

    return 0;
}*/

/* Send a sample message with integer num as the message content */
/*
int client_send_test_message(struct Packet* packet, struct magpie_context* context, int num) {

    char plaintext[PLAINTEXT_SIZE];
    memset(plaintext, 0, PLAINTEXT_SIZE);

    struct TestRequest req;

    req.num = num;
    logger(TRACE, "Message content: %d", req.num);

    packet->sender_id = context->local_ip;
    packet->type = TEST_MESSAGE;
    memcpy(plaintext, &req, sizeof(struct TestRequest));
    packet->seq_num = ++context->tx_seq_num;

    encrypt_packet(plaintext, packet, context);
    //logger(DEBUG, "hash of packet before sending: %u", hash(packet, sizeof(struct Packet)));
    
    sendto(context->ss, packet, sizeof(struct Packet), 0, (struct sockaddr *)&(context->remote_addr), sizeof(struct sockaddr));
    logger(DEBUG, "Sending example message from client to server");
    sleep(1);

    return 0;
}
*/

/* File Transfer: Create Request Packet to ask server for current contents of a specified file */
int create_read_req(struct magpie_packet* out_packet, struct magpie_context* context) {

    struct magpie_read_request req;
    memset(&req, 0, sizeof(struct magpie_read_request));
    memcpy(req.filename, context->filename, NAME_LENGTH);

    char plaintext[PLAINTEXT_SIZE];
    memset(plaintext, 0, PLAINTEXT_SIZE);
    memcpy(plaintext, &req, sizeof(struct magpie_read_request));

    out_packet->sender_id = context->local_id;
    out_packet->type = READ_FILE;
    
    encrypt_packet(plaintext, out_packet, context);

    //done! packet is ready to send

    return HC_ONE_TO_SEND
}

int handle_read_response(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context {
    logger(DEBUG, "Received a read file message");
    
    char plaintext[PLAINTEXT_SIZE];
    struct magpie_read_response res;

    //check that client is ready for messages
    if (context->state != READY) {
        return HC_INCORRECT_CONTEXT_STATE;
    }

    //check that this is a read file message type
    if (packet->type != READ_FILE) {
        return HC_INCORRECT_PACKET_TYPE;
    }

    int ret = decrypt_packet(plaintext, in_packet, context);
    if (ret != 0) {
        //TODO: do something more productive than just return
        return -1;
    }

    memcpy(&res, plaintext, sizeof(struct magpie_read_response));
    logger(DEBUG, "Received data for filename %s", context->filename);

    //TODO: temporary because using ugrad
    logger(DEBUG, "Saving data to file temp.txt");
    strcpy(context->filename, "temp.txt");

    if (context->fd == NULL) {
        //then we are starting a new file
        logger(DEBUG, "Starting a new file");
        context->fd = fopen(context->filename, "w");
        if (context->fd == NULL)
        {
            logger(WARN, "Can't open file: %s", context->filename);
            return HC_FOPEN_FAILED;
        }
    } else {
        //else, just continue because fd is already open so it is not a new file
        logger(DEBUG, "File already open, continuing to add");
    }
    for(int i = 0; i < res.num_bytes; i++) {
        char c = res.data[i];
        fputc(c, context->fd);
    }

    if (res.is_last_packet) {
        //last packet, close file
        logger(DEBUG, "Last packet, closing the file");
        fclose(context->fd);
        context->fd = NULL;
        return HC_LAST_TO_SEND;
    } else {
        //send up packet asking server for the next part
        create_read_req(out_packet, context);
        logger(DEBUG, "Done setting up next read request, ready to send");
    }

    return 0;

}

/* File Transfer: Create Request Packet to ask server to write to specified file */
int create_write_req(struct magpie_packet* out_packet, struct magpie_context* context) {

    //first, send filename
    struct magpie_write_request req;
    memset(&req, 0, sizeof(struct magpie_write_request));
    memcpy(req.data, context->filename, NAME_LENGTH);
    req.is_filename = 1; //Tell server that data contains the filename only
    req.is_last_packet = 0;
    req.num_bytes = NAME_LENGTH;

    //TODO: eventually, let user chose the mode
    strcpy(req.mode, "w");

    char plaintext[PLAINTEXT_SIZE];
    memset(plaintext, 0, PLAINTEXT_SIZE);
    memcpy(plaintext, &req, sizeof(struct magpie_write_request));

    out_packet->sender_id = context->local_ip;
    out_packet->type = WRITE_FILE;

    encrypt_packet(plaintext, packet, context);

    //done! packet is ready to send

    return HC_ONE_TO_SEND;
}

/* File Transfer: Create Request Packet to send data to server for a write request */
int create_write_data(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context) {

    struct magpie_write_request req;
    memset(&req, 0, sizeof(struct magpie_write_request));
    req.is_filename = 0;

    if (context->fd == NULL) {
        //then we are sending a new file
        logger(DEBUG, "First time opening this file %s", context->filename);
        context->fd = fopen(context->filename, "r");
        if (context->fd == NULL)
        {
            logger(WARN, "Can't open file: %s", context->filename);
            return HC_FOPEN_FAILED;
        }
    }  else {
        //else, just continue because fd and filename are already set up
        logger(DEBUG, "Continuing to send file %s", context->filename);
    }

    //fill the response with the next set of data from the file
    int i;
    req.is_last_packet = 0;

    int ret = HC_ONE_TO_SEND;
    for(i = 0; i < WRITE_DATA_SIZE; i++) {
        char c = fgetc(context->fd);
        if (c != EOF) {
            req.data[i] = c;
        } else {
            logger(DEBUG, "This will be the last packet. EOF at index: %d", i);
            req.is_last_packet = 1;
            fclose(context->fd);
            context->fd = NULL;
            ret = HC_LAST_TO_SEND;
            break;
        }
    }
    req.num_bytes = i;

    logger(DEBUG, "Setting up the rest of the packet");
    char plaintext[PLAINTEXT_SIZE];
    //Set up the rest of the packet
    out_packet->sender_id = context->local_id;
    out_packet->type = WRITE_FILE;
    memcpy(plaintext, &req, sizeof(struct magpie_write_request));

    logger(DEBUG, "Encrypting");
    //finally, encrypt it
    if (encrypt_packet(plaintext, out_packet, context) < 0)
        ret = HC_ENCRYPTION_FAILED;
    else 
        //done, packet is ready to send
        logger(DEBUG, "Done, ready to send");
    
    return ret;

}

int magpie_client_handle_packet(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context) {
    //ignore packets from self (because of multicast)
    if (in_packet && in_packet->sender_id == context->local_id) {
        logger(DEBUG, "Received packet from self");
        return HC_MESSAGE_FROM_SELF;
    }

    if (in_packet == NULL) {
        if (context->state == AWAITING_BEGIN) {
            logger(DEBUG, "Creating packet 1 from client to server");
            return create_handshake_xx_1(out_packet, context); 
        } 
        if (context->state == READY) {
            if (strcmp("r", context->operation) == 0)
                return create_read_req(out_packet, context);
            else if (strcmp("w", context->operation) == 0)
                return create_write_req(out_packet, context);
            else 
                return HC_UNKNOWN_ERROR;
        }
        logger(ERROR, "processing empty packet?");
        return HC_UNKNOWN_ERROR;  // Handle this later
    }

    //process by type
    switch (in_packet->type) {
        case HANDSHAKE_XX_2: {
            return handle_handshake_xx_2(out_packet, in_packet, context);
        }

        case READ_FILE: {
            return handle_read_response(out_packet, in_packet, context);
        }

        case WRITE_FILE: {
            return create_write_data(out_packet, in_packet, context);
        }

        default: {
            logger(DEBUG, "Undefined type %d", in_packet->type);
            return HC_INCORRECT_PACKET_TYPE;
        }
    }
    
    return HC_UNKNOWN_ERROR;
}
