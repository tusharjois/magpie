//serverlib.c

#include "serverlib.h" 
#include "messages.h"
#include "helper.h"

int handle_handshake_xx_1(struct Packet* packet, struct Context* context) {

    //check that client is ready for xx1
    if (context->state != AWAITING_XX_1) {
        return -1;
    }

    //check that the packet type is xx1
    if (packet->type != HANDSHAKE_XX_1) {
        logger(DEBUG, "Received a packet with incorrect type (expecting handshake 1)");
        return -2;
    }
    
    //process packet
    memcpy(context->handshake_xx_1, packet->payload, hydro_kx_XX_PACKET1BYTES);
    if (hydro_kx_xx_2(&context->hydro_state, context->handshake_xx_2, context->handshake_xx_1, NULL, &context->local_kp) != 0) {
        logger(WARN, "Failed to process client's first handshake packet");
        return -3;
    } else {
        return 0;
    }
}

int create_handshake_xx_2(struct Packet* packet, struct Context* context) {

    packet->sender_id = context->local_ip;
    packet->type = HANDSHAKE_XX_2;
    memcpy(packet->payload, context->handshake_xx_2, hydro_kx_XX_PACKET2BYTES);
    // Done! session_kp.tx is the key for sending data to the server,
    // and session_kp.rx is the key for receiving data from the server.

    return 0;
}

int handle_handshake_xx_3(struct Packet* packet, struct Context* context) {

    //check that client is ready for xx3
    if (context->state != AWAITING_XX_3) {
        return -1;
    }

    //check that the packet type is xx3
    if (packet->type != HANDSHAKE_XX_3) {
        logger(DEBUG, "Received a packet with incorrect type (expecting handshake 3)");
        return -2;
    }

    //process packet 3
    memcpy(context->handshake_xx_3, packet->payload, hydro_kx_XX_PACKET3BYTES);
    if (hydro_kx_xx_4(&context->hydro_state, &context->session_kp, NULL, context->handshake_xx_3, NULL) != 0) {
        logger(WARN, "Failed to process client handshake request packet 3");
        return -3;
    } else {
        return 0;
        // Done! session_kp.tx is the key for sending data to the client,
        // and session_kp.rx is the key for receiving data from the client.
        // The session keys are the same as those computed by the client, but swapped.
    }


}

int server_handle_test_messages(struct Packet* packet, struct Context* context) {
    
    //check that server is ready for test messages
    if (context->state != TEST) {
        return -1;
    }

    //check that this is a Test message type
    if (packet->type != TEST_MESSAGE) {
        return -2;
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
    int ret = hydro_secretbox_probe_verify(packet->probe, (uint8_t *) ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.rx);
    if (ret != 0) {
        logger(DEBUG, "Probe Failed to Verify");
        return -4;
    }

    //decrypt packet
    if (hydro_secretbox_decrypt(plaintext, (uint8_t *) ciphertext, CIPHERTEXT_SIZE, 0, CONTEXT, context->session_kp.rx) != 0) {
        logger(DEBUG, "Message forged!");
    } else {
        struct TestRequest req;
        memcpy(&req, plaintext, sizeof(struct TestRequest));
        logger(TRACE, "Message content: %d", req.num);
        context->rx_seq_num++;
        server_send_test_message(packet, context, req.num + 1);
    }

    return 0;
}

/* Send a sample message with integer num as the message content */
int server_send_test_message(struct Packet* packet, struct Context* context, int num) {

    char plaintext[PLAINTEXT_SIZE];
    char ciphertext[CIPHERTEXT_SIZE];
    struct TestRequest req;
    memset(plaintext, 0, PLAINTEXT_SIZE);

    req.num = num;
    logger(TRACE, "Message content: %d", req.num);

    packet->sender_id = context->local_ip;
    packet->type = TEST_MESSAGE;
    memcpy(plaintext, &req, sizeof(struct TestRequest));
    packet->seq_num = ++context->tx_seq_num;

    hydro_secretbox_encrypt((uint8_t *) ciphertext, plaintext, PLAINTEXT_SIZE, 0, CONTEXT, context->session_kp.tx);
    memcpy(packet->payload, ciphertext, CIPHERTEXT_SIZE);

    //create the probe
    hydro_secretbox_probe_create(packet->probe, (uint8_t *) ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.tx);

    //logger(DEBUG, "hash of packet before sending: %u", hash(packet, sizeof(struct Packet)));
    
    sendto(context->ss, packet, sizeof(struct Packet), 0, (struct sockaddr *)&(context->remote_addr), sizeof(struct sockaddr));
    logger(DEBUG, "Sending example message from server to client");
    sleep(1);

    return 0;
}

int handle_read_request(struct Packet* packet, struct Context* context) {
    
    char plaintext[PLAINTEXT_SIZE];
    struct ReadRequest req;

    //check that server is ready for messages
    if (context->state != READY) {
        return -1;
    }

    //check that this is a read file message type
    if (packet->type != READ_FILE) {
        return -2;
    }

    //check for correct order
    if (packet->seq_num != context->rx_seq_num + 1) {
        logger(DEBUG, "Out of sequence packet (Expecting: %d, Actual: %d)", context->rx_seq_num + 1, packet->seq_num);
        return -3;
    }
    context->rx_seq_num = packet->seq_num;

    decrypt_packet(plaintext, packet, context);
    memcpy(&req, plaintext, sizeof(struct ReadRequest));
    logger(DEBUG, "Received request to send filename %s", req.filename);

    if (context->fd == NULL) {
        //then we are sending a new file
        logger(DEBUG, "First time opening this file %s", req.filename);
        strcpy(context->filename, req.filename);
        context->fd = fopen(context->filename, "r");
        if (context->fd == NULL)
        {
            logger(WARN, "Can't open file: %s", context->filename);
            return -4;
            //TODO: Populate return packet with an error code to indicate bad filename
        }
    }  else {
        //else, just continue because fd and filename are already set up
        logger(DEBUG, "Continuing to send file %s", req.filename);
    }

    //fill the response with the next set of data from the file
    struct ReadResponse res;
    int i;
    res.is_last_packet = 0;

    for(i = 0; i < READ_DATA_SIZE; i++) {
        char c = fgetc(context->fd);
        if (c != EOF) {
            res.data[i] = c;
        } else {
            logger(DEBUG, "This will be the last packet. EOF at index: %d", i);
            res.is_last_packet = 1;
            fclose(context->fd);
            context->fd = NULL;
            memset(context->filename, 0, MAX_FILE_NAME);
            break;
        }
    }
    res.num_bytes = i;

    logger(DEBUG, "Setting up the rest of the packet");
    //Set up the rest of the packet
    packet->sender_id = context->local_ip;
    packet->type = READ_FILE;
    packet->seq_num = ++context->tx_seq_num;
    memcpy(plaintext, &res, sizeof(struct ReadResponse));

    logger(DEBUG, "Encrypting");
    //finally, encrypt it
    encrypt_packet(plaintext, packet, context);

    logger(DEBUG, "Done, ready to send");
    //done, packet is ready to send
    return 0;
}

