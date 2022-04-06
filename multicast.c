//Multicast functions (for testing purposes)
#include <time.h>
#include "magpielib.h"

void await_message(char* mess_buff, int* from_ip, int* mess_len, struct magpie_context* context) {
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
                if (has_flushed) {
                    printf("\n");
                }
                
                char local_ip_buff[128];
                char remote_ip_buff[128];
                memset(local_ip_buff, 0, 128);
                memset(remote_ip_buff, 0, 128);

                format_ip_address(local_ip_buff, context->local_ip);
                format_ip_address(remote_ip_buff, *from_ip);

                unsigned int h = hash(mess_buff, *mess_len);

                logger(DEBUG, "Received %d byte message from %s to %s. Digest: %u", *mess_len, remote_ip_buff, local_ip_buff, h);

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

int recv_mc_msg(char* mess_buff, int* mess_len, struct sockaddr_in* src_addr, struct magpie_context* context) {
    socklen_t dummy_len;
    fd_set mask;
    struct timeval timeout;
    struct timeval tick_timeout;
    struct timeval now;
    int tick_counter = 0;
    char ticks[] = {'-', '\\', '|', '/'};
    tick_timeout.tv_sec = 1;
    tick_timeout.tv_usec = 0;
    printf("\rawaiting message [%c]  ", ticks[tick_counter++ % 4]);
    for (;;)
    {
        FD_ZERO(&mask);
        FD_SET(context->sk, &mask);
        timeout.tv_sec = tick_timeout.tv_sec;
        timeout.tv_usec = tick_timeout.tv_usec;

        int num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);
        if (num > 0)
        {
            //we've received a packet
            if (FD_ISSET(context->sk, &mask))
            {
                *mess_len = recvfrom(context->sk, mess_buff, MESS_BUFF_LEN, 0, (struct sockaddr *)src_addr, &dummy_len);
                if (context->state == AWAITING_XX_1 || context->state == AWAITING_XX_2 || context->state == AWAITING_XX_3)
                    usleep(10000);  // TODO, fixme, this is currently a hack
                logger(DEBUG, "Received %dB message. Digest: %u", *mess_len, hash(mess_buff, *mess_len));
                return 0;
            }
        }
        else
        {
            gettimeofday(&now, NULL);
            if (context->is_server && context->remote_id && timediff(&context->last_heartbeat, &now) > context->timeout.tv_sec) {
                logger(WARN, "client connection timeout [ remote_id=%d idle_time=%dsec", context->remote_id, context->timeout.tv_sec);
                reset_context(context);
            } else if (!context->is_server) {
                logger(WARN, "local timeout");
                return -1;
            }
            //timeout
            printf("\rawaiting message [%c]  ", ticks[tick_counter++ % 4]);
            fflush(0);
        }
    }
}

int receive(int sk, char* mess) {
    socklen_t dummy_len;
    struct sockaddr_in src_addr;
    fd_set mask;
    struct timeval timeout;

    FD_ZERO(&mask);
    FD_SET(sk, &mask);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    int num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);
    if (num > 0)
    {
        //we've received a packet
        if (FD_ISSET(sk, &mask))
        {
            int mess_len = recvfrom(sk, mess, MESS_BUFF_LEN, 0, (struct sockaddr *)&src_addr, &dummy_len);
            usleep(10000);  // TODO, fixme, this is currently a hack
            //logger(DEBUG, "Received %dB message. Digest: %u", mess_len, hash(mess, mess_len));
            logger(DEBUG, "Received %dB message", mess_len);
            return mess_len;
        }
    }
    
    return 0;
}

int send_mc_msg(char* msg_buff, int msg_len, struct magpie_context* context) {
    logger(DEBUG, "sending multicast [ size=%dB digest=%u ]", msg_len, hash(msg_buff, msg_len));
    
    sendto(context->sk, msg_buff, msg_len, 0, 
        (struct sockaddr *)&(context->remote_addr), sizeof(struct sockaddr));
    
    if (++NUM_SENT >= MAX_NUM_SENT) {
        logger(FATAL, "Max sends reached");
        exit(1);
    }  
    return 0;
}

int get_mcast_socket(int* sk, struct sockaddr_in* remote_addr, int mcast_addr, int mcast_port) {
    struct ip_mreq mreq;
    struct sockaddr_in local_addr;

    //set up socket to receive messages on
    *sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (*sk < 0)
    {
        logger(FATAL, "Mcast: socket");
        exit(1);
    }

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(mcast_port);

    if (bind(*sk, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    {
        logger(FATAL, "Mcast: bind");
        exit(1);
    }

    // join the multicast group
    mreq.imr_multiaddr.s_addr = htonl(mcast_addr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(*sk, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, sizeof(mreq)) < 0)
    {
        logger(FATAL, "Mcast: problem in setsockopt to join multicast address");
    }

    remote_addr->sin_family = AF_INET;
    remote_addr->sin_addr.s_addr = htonl(mcast_addr);
    remote_addr->sin_port = htons(mcast_port);

    char addr_str[MAX_STRING_LEN];
    format_ip_address(addr_str, mcast_addr);
    logger(INFO, "Receeive socket initialized [ sk=%d mcast=%s:%d ]", *sk, addr_str, mcast_port);
    return 0;
}

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