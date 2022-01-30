#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdio.h>
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

#include "logger.h"
#include "hydrogen.h"


#define PORT 10225
#define MCAST_ADDR (225 << 24 | 1 << 16 | 1 << 8 | 140) /* (225.1.1.140) */
#define NAME_LENGTH 100
#define MESS_BUFF_LEN 4096
#define BURST_SIZE 5
#define CONTEXT "HYDROGEN"


#define PAYLOAD_SIZE 1400
#define PLAINTEXT_SIZE (PAYLOAD_SIZE - hydro_secretbox_HEADERBYTES)
#define CIPHERTEXT_SIZE (PAYLOAD_SIZE)
#define FIRST_MESSAGE_IDX 1
#define MAX_FILE_NAME 256
#define START_SEQ_NUM 1

enum Type {
    HANDSHAKE_XX_1 = 1, 
    HANDSHAKE_XX_2 = 2,
    HANDSHAKE_XX_3 = 3,
    HANDSHAKE_XX_4 = 4,
    READ_FILE = 5,
    WRITE_FILE = 6,
    DELETE_FILE = 7,
    TEST_MESSAGE = 8
};

struct Packet {
    uint16_t seq_num;
    enum Type type;
    int sender_id;
    uint8_t probe[hydro_secretbox_PROBEBYTES];
    char payload[PAYLOAD_SIZE];
};


/* operation request initiated by the client */
struct ReadRequest {
    char filename[MAX_FILE_NAME];
};

struct TestRequest {
    int num;
};

enum State {
    READY = 0,
    AWAITING_XX_1 = 1,
    AWAITING_XX_2 = 2,
    AWAITING_XX_3 = 3,
    AWAITING_XX_4 = 4,
    TEST = 5
};

struct Context {
    int ss; //socket send
    int sr; //socket receive

    fd_set mask;
    fd_set write_mask;
    fd_set excep_mask;

    int local_addr;
    int local_ip;
    char local_name[NAME_LENGTH];

    struct sockaddr_in remote_addr;
    struct timeval timeout;

    char filename[NAME_LENGTH];
    char operation[NAME_LENGTH];

    hydro_kx_state hydro_state;
    hydro_kx_session_keypair session_kp;
    hydro_kx_keypair remote_kp;
    hydro_kx_keypair local_kp;
    uint8_t handshake_xx_1[hydro_kx_XX_PACKET1BYTES];
    uint8_t handshake_xx_2[hydro_kx_XX_PACKET2BYTES];
    uint8_t handshake_xx_3[hydro_kx_XX_PACKET3BYTES];

    enum State state;
    int tx_seq_num; //remember which packet number was last sent
    int rx_seq_num; //remember which packet number was last received
};


#endif