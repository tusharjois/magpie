//clientlib.c

#include "clientlib.h"
#include "messages.h"
#include "helper.h"


int create_handshake_xx_1(struct Packet* packet, struct Context* context) {
    // Create and send first packet
    int ret = hydro_kx_xx_1(&context->hydro_state, context->handshake_xx_1, NULL);

    packet->sender_id = context->local_ip;
    packet->type = HANDSHAKE_XX_1;
    memcpy(packet->payload, context->handshake_xx_1, hydro_kx_XX_PACKET1BYTES);
    return ret;
}

int handle_handshake_xx_2(struct Packet* packet, struct Context* context) {

    //check that client is ready for xx2
    if (context->state != AWAITING_XX_2) {
        logger(DEBUG, "Incorrect context state");
        return -1;
    }

    logger(DEBUG, "Received handshake 2 from server");
    //process packet
    memcpy(context->handshake_xx_2, packet->payload, hydro_kx_XX_PACKET2BYTES);
    if (hydro_kx_xx_3(&context->hydro_state, &context->session_kp, context->handshake_xx_3, NULL, context->handshake_xx_2, NULL,
                &context->local_kp) != 0) {
        logger(WARN, "Failed to process server's second handshake packet");
        return -3;
    } else {
        return 0;
    }

    return 0;
}

int create_handshake_xx_3(struct Packet* packet, struct Context* context) {

    packet->sender_id = context->local_ip;
    packet->type = HANDSHAKE_XX_3;
    memcpy(packet->payload, context->handshake_xx_3, hydro_kx_XX_PACKET3BYTES);
    // Done! session_kp.tx is the key for sending data to the server,
    // and session_kp.rx is the key for receiving data from the server.

    return 0;
}


int client_handle_test_messages(struct Packet* packet, struct Context* context) {

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
}

/* Send a sample message with integer num as the message content */
int client_send_test_message(struct Packet* packet, struct Context* context, int num) {

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

/* File Transfer: Create Request Packet to ask server for current contents of a specified file */
int create_read_req(struct Packet* packet, struct Context* context) {

    struct ReadRequest req;
    memcpy(req.filename, context->filename, NAME_LENGTH);

    char plaintext[PLAINTEXT_SIZE];
    memset(plaintext, 0, PLAINTEXT_SIZE);
    memcpy(plaintext, &req, sizeof(struct ReadRequest));

    packet->sender_id = context->local_ip;
    packet->type = READ_FILE;
    packet->seq_num = ++context->tx_seq_num;


    encrypt_packet(plaintext, packet, context);

    //done! packet is ready to send

    return 0;
}

int handle_read_response(struct Packet* packet, struct Context* context) {

    char plaintext[PLAINTEXT_SIZE];
    struct ReadResponse res;

    //check that client is ready for messages
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

    memcpy(&res, plaintext, sizeof(struct ReadResponse));
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
            return -4;
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
    } else {
        //send up packet asking server for the next part
        create_read_req(packet, context);
        logger(DEBUG, "Done setting up next read request, ready to send");
    }


    return 0;

}