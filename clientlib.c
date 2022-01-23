//clientlib.c

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
#include "clientlib.h"
#include "logger.h"

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
        return -1;
    }

    //check that the packet type is xx2
    if (packet->type != HANDSHAKE_XX_2) {
        logger(DEBUG, "Received a packet with incorrect type (expecting handshake 2)");
        return -2;
    }
    
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


int handle_test_messages(struct Packet* packet, struct Context* context) {
    
    //check that client is ready for test messages
    if (context->state != TEST) {
        return -1;
    }
    
    char plaintext[PLAINTEXT_SIZE];
    char ciphertext[CIPHERTEXT_SIZE];
    memset(plaintext, 0, PLAINTEXT_SIZE);

    //decrypt packet
    memcpy(ciphertext, packet->payload, CIPHERTEXT_SIZE);

    if (hydro_secretbox_decrypt(plaintext, ciphertext, CIPHERTEXT_SIZE, 0, CONTEXT, context->session_kp.rx) != 0) {
        logger(DEBUG, "Message forged!");
    } else {
        int x;
        memcpy(&x, plaintext, sizeof(int));
        logger(TRACE, "Message content: %d", x);
        send_test_message(packet, context, x + 1);
    }

    return 0;
}

/* Send a sample message with integer num as the message content */
int send_test_message(struct Packet* packet, struct Context* context, int num) {

    char plaintext[PLAINTEXT_SIZE];
    char ciphertext[CIPHERTEXT_SIZE];
    memset(plaintext, 0, PLAINTEXT_SIZE);

    packet->sender_id = context->local_ip;
    int x = num;
    memcpy(plaintext, &x, sizeof(int));

    logger(TRACE, "Message content: %d", x);

    hydro_secretbox_encrypt(ciphertext, plaintext, PLAINTEXT_SIZE, 0, CONTEXT, context->session_kp.tx);
    memcpy(packet->payload, ciphertext, CIPHERTEXT_SIZE);

    logger(DEBUG, "hash of packet before sending: %u", hash(packet, sizeof(struct Packet)));
    
    sendto(context->ss, packet, sizeof(struct Packet), 0, (struct sockaddr *)&(context->remote_addr), sizeof(struct sockaddr));
    logger(DEBUG, "Sending example message from client to server");
    sleep(1);

    return 0;
}