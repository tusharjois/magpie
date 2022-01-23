//serverlib.c

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include "hydrogen.h"
#include "serverlib.h"
#include "logger.h"

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

    //decrypt packet
    memcpy(ciphertext, packet->payload, CIPHERTEXT_SIZE);

    if (hydro_secretbox_decrypt(plaintext, ciphertext, CIPHERTEXT_SIZE, 0, CONTEXT, context->session_kp.rx) != 0) {
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

    hydro_secretbox_encrypt(ciphertext, plaintext, PLAINTEXT_SIZE, 0, CONTEXT, context->session_kp.tx);
    memcpy(packet->payload, ciphertext, CIPHERTEXT_SIZE);

    //logger(DEBUG, "hash of packet before sending: %u", hash(packet, sizeof(struct Packet)));
    
    sendto(context->ss, packet, sizeof(struct Packet), 0, (struct sockaddr *)&(context->remote_addr), sizeof(struct sockaddr));
    logger(DEBUG, "Sending example message from server to client");
    sleep(1);

    return 0;
}