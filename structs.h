#ifndef STRUCTS_H
#define STRUCTS_H

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
// #define NAME_LENGTH 100
#define MAX_STRING_LEN 1024
#define MESS_BUFF_LEN 2048

// #define BURST_SIZE 5
#define CONTEXT "HYDROGEN"

#define PACKET_SIZE 1400
#define CIPHERTEXT_SIZE (PACKET_SIZE - hydro_secretbox_PROBEBYTES - 16)  // being safe
#define PLAINTEXT_SIZE (CIPHERTEXT_SIZE - hydro_secretbox_HEADERBYTES)
#define PAYLOAD_SIZE (PLAINTEXT_SIZE - 16)

#define INIT_SEQ_NUM 0
#define HANDSHAKE_XX_1_SEQ_NUM 1
#define HANDSHAKE_XX_2_SEQ_NUM 2
#define HANDSHAKE_XX_3_SEQ_NUM 3
#define TRANSFER_START_SEQ_NUM 4

#define WINDOW_SIZE 4

enum magpie_type {
    HANDSHAKE_XX_1 = 1, 
    HANDSHAKE_XX_2 = 2,
    HANDSHAKE_XX_3 = 3,
    TRANSFER = 8
};

enum magpie_state {
    UNKNOWN = 0,  
    AWAITING_BEGIN = 1,
    AWAITING_XX_1 = 2,
    AWAITING_XX_2 = 3,
    AWAITING_XX_3 = 4,
    READY = 5
};

enum magpie_handler_codes {
    HC_UNKNOWN_ERROR = -100,
    HC_DECRYPTION_HASH_FORGED = -11,
    HC_DECRYPTION_MESSAGE_FORGED = -10,
    HC_DECRYPTION_PROBE_FAILED = -9,
    HC_DECRYPTION_OUT_OF_ORDER = -8,
    HC_SERVER_BUSY = -7,
    HC_FOPEN_FAILED = -6,
    HC_ENCRYPTION_FAILED = -5,
    HC_DECRYPTION_FAILED = -4,
    HC_INCORRECT_PACKET_TYPE = -3,
    HC_INCORRECT_CONTEXT_STATE = -2,
    HC_LIBHYDROGEN_ERROR = -1,
    HC_OKAY = 0,
    HC_ONE_TO_SEND = 1,
    HC_MORE_TO_SEND = 2,
    HC_LAST_TO_SEND = 3,
    HC_MESSAGE_FROM_SELF = 4,
    HC_TRANSFER_COMPELTE = 5
};

struct magpie_meta {
    unsigned int seq_num;
    unsigned short sender_id;
};

struct magpie_packet {
    struct magpie_meta meta;  // Meta data about the magpie_packet
    uint8_t probe[hydro_secretbox_PROBEBYTES];  // probe for quick packet verification
    char ciphertext[CIPHERTEXT_SIZE];  // encrypted payload
};

// plaintext
struct magpie_message {
    enum magpie_type type;
    unsigned int meta_hash;
    unsigned short num_bytes;
    unsigned short is_last_packet;
    char payload[PAYLOAD_SIZE];
};

struct magpie_buffer {
    int is_io_buffer;
    int buffer_len;
    FILE* io_buffer;
    char* char_buffer;
    int num_bytes_read;
    int is_empty;
};

struct magpie_context {
    int is_server;
    unsigned short local_id;
    unsigned short remote_id;
    
    struct timeval operation_start;
    int bytes_transferred;

    struct magpie_buffer send_buffer;
    struct magpie_buffer recv_buffer;

    hydro_kx_state hydro_state;
    hydro_kx_session_keypair session_kp;
    hydro_kx_keypair remote_kp;
    hydro_kx_keypair local_kp;
    uint8_t handshake_xx_1[hydro_kx_XX_PACKET1BYTES];
    uint8_t handshake_xx_2[hydro_kx_XX_PACKET2BYTES];
    uint8_t handshake_xx_3[hydro_kx_XX_PACKET3BYTES];
    int handshake_xx_1_done;
    int handshake_xx_2_done;

    enum magpie_state state;
    int tx_seq_num; //remember which packet number was last sent
    int rx_seq_num; //remember which packet number was last received
};


#endif