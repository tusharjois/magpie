//helper.c

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
#include "helper.h"


void get_my_ip_address(int* my_address) {
    char host_name[256];
    struct hostent h_ent;
    struct hostent* p_h_ent;
    gethostname(host_name, 256);
    p_h_ent = gethostbyname(host_name);
    memcpy( &h_ent, p_h_ent, sizeof(h_ent));
    memcpy( my_address, h_ent.h_addr_list[0], sizeof(int) );
}

void format_ip_address(char* buffer, int ip) {
    sprintf(buffer, "%d.%d.%d.%d", 
        (htonl(ip) & 0xff000000)>>24,
        (htonl(ip) & 0x00ff0000)>>16,
        (htonl(ip) & 0x0000ff00)>>8,
        (htonl(ip) & 0x000000ff));
}

void setup(struct Context* context)
{
    // Initialize logger
    logger_init(ALL, true);

    // Initialize LibHydrogen
    if (hydro_init() != 0)
    {
        logger(FATAL, "LibHydrogen failed to initialize. Aborting :(");
        abort();
        exit(1);
    }

    // Get local IP Address
    get_my_ip_address(&(context->local_ip));
    char buffer[128];
    format_ip_address(buffer, context->local_ip);
    logger(DEBUG, "My IP Address: %s", buffer);

    setup_mcast(context);

    FD_ZERO(&(context->mask));
    FD_ZERO(&(context->write_mask));
    FD_ZERO(&(context->excep_mask));
    FD_SET(context->ss, &(context->mask));
    FD_SET(context->sr, &(context->mask));
    FD_SET((long)0, &(context->mask)); /* stdin */

    context->timeout.tv_sec = 1;
    context->timeout.tv_usec = 0;

    context->rx_seq_num = context->tx_seq_num = START_SEQ_NUM - 1;

}

void await_message(char* mess_buff, int* from_ip, int* mess_len, struct Context* context) {
    struct sockaddr_in src_addr;
    struct timeval timeout;
    socklen_t addrlen;
    int has_flushed = 0;
    fd_set read_mask;
    for (;;)
    {
        read_mask = context->mask;
        timeout.tv_sec = context->timeout.tv_sec;
        timeout.tv_usec = context->timeout.tv_usec;

        int num = select(FD_SETSIZE, &read_mask, &(context->write_mask), &(context->excep_mask), &timeout);
        if (num > 0)
        {
            //we've received a packet
            if (FD_ISSET(context->sr, &read_mask))
            {
                *mess_len = recvfrom(context->sr, mess_buff, MESS_BUFF_LEN, 0, (struct sockaddr *)&src_addr, &addrlen);
                *from_ip = src_addr.sin_addr.s_addr;
                if (has_flushed)
                    printf("\n");
                return;
            }
        }
        else
        {
            //timeout
            printf(".");
            fflush(0);
            has_flushed = 1;
        }
    }
}

void setup_mcast(struct Context* context) {
    logger(TRACE, "check");
    struct hostent h_ent;
    struct hostent *p_h_ent;
    struct sockaddr_in name;
    struct ip_mreq mreq;

    logger(TRACE, "starting");

    //get local ip address
    gethostname(context->local_name, NAME_LENGTH);
    logger(TRACE, "got localname");

    p_h_ent = gethostbyname(context->local_name);
    logger(TRACE, "got phent");
    if (p_h_ent == NULL)
    {
        logger(FATAL, "Ucast: gethostbyname error.");
        exit(1);
    }
    memcpy(&h_ent, p_h_ent, sizeof(h_ent));

    memcpy(&(context->local_addr), h_ent.h_addr_list[0], sizeof(int));
    logger(TRACE, "got local address");

    //set up socket to receive messages on
    context->sr = socket(AF_INET, SOCK_DGRAM, 0); /* socket for receiving */
    if (context->sr < 0)
    {
        logger(FATAL, "Mcast: socket");
        exit(1);
    }
    logger(TRACE, "setup receive socket");

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if (bind(context->sr, (struct sockaddr *)&name, sizeof(name)) < 0)
    {
        logger(FATAL, "Mcast: bind");
        exit(1);
    }

    // hopkins multicast address
    mreq.imr_multiaddr.s_addr = htonl(MCAST_ADDR);

    // join the multicast group
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(context->sr, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq,
                   sizeof(mreq)) < 0)
    {
        logger(FATAL, "Mcast: problem in setsockopt to join multicast address");
    }
    logger(TRACE, "joined multicast");

    //set up socket to send messages from
    context->ss = socket(AF_INET, SOCK_DGRAM, 0);
    if (context->ss < 0)
    {
        logger(FATAL, "Mcast: socket");
        exit(1);
    }

    logger(TRACE, "setup send socket");

    context->remote_addr.sin_family = AF_INET;
    context->remote_addr.sin_addr.s_addr = htonl(MCAST_ADDR);
    context->remote_addr.sin_port = htons(PORT);

    unsigned char ttl_val = 1;
    if (setsockopt(context->sr, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl_val,
                   sizeof(ttl_val)) < 0)
    {
        printf("Mcast: problem in setsockopt of multicast ttl %d - ignore in WinNT or Win95\n", ttl_val);
    }
    if (setsockopt(context->ss, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl_val,
                   sizeof(ttl_val)) < 0)
    {
        printf("Mcast: problem in setsockopt of multicast ttl %d - ignore in WinNT or Win95\n", ttl_val);
    }

    logger(TRACE, "all done with this");
}

void deepcopy_state(struct hydro_kx_state* state_cpy, struct hydro_kx_state* state) {

    memcpy(&(state_cpy->eph_kp), &(state->eph_kp), sizeof(struct hydro_kx_keypair));
    memcpy(&(state_cpy->h_st), &(state->h_st), sizeof(struct hydro_hash_state));

}

/* not for security purposes ;) dbj2 hash */
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