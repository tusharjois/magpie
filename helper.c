//helper.c
#include <time.h>
#include "helper.h"

void setup_context(struct magpie_context* context, void* buffer, int buffer_len, char* key_filepath, int is_server) {
    //everything null to start
    memset(context, 0, sizeof(struct magpie_context));

    // Initialize logger
    logger_init(TRACE, false);

    // Initialize LibHydrogen
    if (hydro_init() != 0)
    {
        logger(FATAL, "LibHydrogen failed to initialize. Aborting :(");
        exit(1);
    }

    // key exchange (xx variant): server 
    load_hydro_kx_keypair(&context->local_kp, key_filepath);
    char buffer[MAX_STRING_LEN];
    format_keypair(buffer, &context->local_kp);
    logger(DEBUG, "%s keypair:\n%s", is_server ? "Server" : "Client", buffer);

    char randomness[MAX_STRING_LEN];
    memset(randomness, 77, MAX_STRING_LEN);
    sprintf(randomness, "%d__%d__%ld", rand(), rand(), time(NULL));
    context->local_id = hash(randomness, MAX_STRING_LEN);
    context->is_server = is_server;
    context->state = context->is_server ? AWAITING_XX_1 :AWAITING_BEGIN;
    context->timeout.tv_sec = context->is_server ? 15 : 1;
    get_mcast_socket(&context->sk, &context->remote_addr, MCAST_ADDR, MCAST_PORT);
    context->is_io_buffer = buffer_len == 0;
    context->buffer_len = buffer_len;

    if (context->is_io_buffer)
        context->io_buffer = (FILE*) buffer;
    else 
        context->char_buffer = (char*) buffer;  // Assumes this memory won't be freed during magpie_process

    context->rx_seq_num = context->tx_seq_num = START_SEQ_NUM - 1;
}

void reset_context(struct magpie_context* context)
{
    if (context->fd) {
        fclose(context->fd);
        context->fd = NULL;
    }

    // Re-initialize LibHydrogen
    if (hydro_init() != 0)
    {
        logger(FATAL, "LibHydrogen failed to initialize. Aborting :(");
        exit(1);
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    double time_elapsed = timediff(&context->operation_start, &now);
    logger(INFO, "Operation Complete [ data=%dB latency=%.3fms throughput=%.3fkbps ]", 
        context->bytes_transferred, time_elapsed * 1000, context->bytes_transferred / (time_elapsed * 1000));
    
    struct magpie_context new_context;
    memset(&new_context, 0, sizeof(struct magpie_context));
    new_context.sk = context->sk;
    new_context.local_id = context->local_id;
    memcpy(&new_context.remote_addr, &context->remote_addr, sizeof(struct sockaddr_in));
    memcpy(&new_context.local_kp, &context->local_kp, sizeof(hydro_kx_keypair));
    new_context.is_server = context->is_server;
    new_context.state = new_context.is_server ? AWAITING_XX_1 : AWAITING_BEGIN;
    new_context.timeout.tv_sec = new_context.is_server ? 15 : 1;

    context->rx_seq_num = context->tx_seq_num = START_SEQ_NUM - 1;

    memcpy(context, &new_context, sizeof(struct magpie_context));

    logger(INFO, "Server ready to serve another client...");
}

int load_hydro_kx_keypair(hydro_kx_keypair* kp, char* filepath) {
    FILE* fd;
    fd = fopen(filepath, "r");

    fread(kp->pk, hydro_kx_PUBLICKEYBYTES, sizeof(char), fd);
    fread(kp->sk, hydro_kx_SECRETKEYBYTES, sizeof(char), fd);
    fclose(fd);
    return 0;
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

int encrypt_packet(char* plaintext, struct magpie_packet* packet, struct magpie_context* context) {

    char ciphertext[CIPHERTEXT_SIZE];

    hydro_secretbox_encrypt((uint8_t *)ciphertext, plaintext, PLAINTEXT_SIZE, 0, CONTEXT, context->session_kp.tx);
    memcpy(packet->payload, ciphertext, CIPHERTEXT_SIZE);

    //create the probe
    hydro_secretbox_probe_create(packet->probe, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.tx);

    //set packet number
    packet->seq_num = ++context->tx_seq_num;
    
    return 0;
} 

/* decrypt the packet after receiving, include probe checking */
int decrypt_packet(char* plaintext, struct magpie_packet* packet, struct magpie_context* context) {

    char* ciphertext = packet->payload;

    //check for correct order (Cannot decrypt out of order packets. Also, you cannot decrypt the same packet multiple times)
    if (packet->seq_num != context->rx_seq_num + 1) {
        logger(DEBUG, "Out of sequence packet (Expecting: %d, Actual: %d)", context->rx_seq_num + 1, packet->seq_num);
        return -3;
    }
    
    //check the probe
    int ret = hydro_secretbox_probe_verify(packet->probe, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.rx);
    if (ret != 0) {
        logger(DEBUG, "Probe Failed to Verify");
        return -4;
    }

    //decrypt packet
    if (hydro_secretbox_decrypt(plaintext, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, 0, CONTEXT, context->session_kp.rx) != 0) {
        logger(DEBUG, "Message forged!");
        return -1;
    } 

    context->rx_seq_num++;

    return 0;

}

static int NUM_SENT = 0;
static int MAX_NUM_SENT = 10000;  // TODO: remove this

/* send the message over multicast to the remote address */
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

void format_keypair(char* buffer, hydro_kx_keypair* kp) {
    memset(buffer, '\0', strlen(buffer));
    sprintf(&buffer[strlen(buffer)], "public = {");
    for (int i=0; i < hydro_kx_PUBLICKEYBYTES; i++) {
      sprintf(&buffer[strlen(buffer)], "%d, ", kp->pk[i]);
    }
    sprintf(&buffer[strlen(buffer)], "}");
    sprintf(&buffer[strlen(buffer)], "\nprivate = ");
    sprintf(&buffer[strlen(buffer)], "{");
    for (int i=0; i < hydro_kx_SECRETKEYBYTES; i++) {
      sprintf(&buffer[strlen(buffer)], "%d, ", kp->sk[i]);
    }
    sprintf(&buffer[strlen(buffer)], "}");
}

double timediff(struct timeval* start, struct timeval* end) {
    return (end->tv_sec + 0.000001 * end->tv_usec) - (start->tv_sec + 0.000001 * start->tv_usec);
}


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