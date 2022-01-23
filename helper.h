#ifndef HELPER_H
#define HELPER_H

#include "logger.h"
#include <stdlib.h>
#include "hydrogen.h"
#include "messages.h"

#define PORT 10225
#define MCAST_ADDR (225 << 24 | 1 << 16 | 1 << 8 | 140) /* (225.1.1.140) */
#define NAME_LENGTH 100
#define MESS_BUFF_LEN 4096
#define BURST_SIZE 5
#define CONTEXT "HYDROGEN"

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

void setup(struct Context* context);

void get_my_ip_address(int* myAddress);

void format_ip_address(char* buffer, int ip);

void await_message(char* mess_buff, int* from_ip, int* mess_len, struct Context* context);

void setup_mcast(struct Context* context);

void deepcopy_state(struct hydro_kx_state* server_state_cpy, struct hydro_kx_state* server_state);

unsigned int hash(char *str, int len);

#endif