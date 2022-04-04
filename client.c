//client.c - the phone/tablet

#include "magpielib.h"

int get_mcast_socket(int* sk, struct sockaddr_in* remote_addr, int mcast_addr, int mcast_port);
int receive(int sk, char* mess);

int main(int argc, char *argv[])
{

    if (argc != 1)
    {
        logger(FATAL, "usage: ./server <path_to_key>");
        exit(1);
    }

    struct magpie_context client_context;
    setup_context(&client_context, "keys/keypair_0", false);
    printf("Client context loaded...\n");

    FILE* client_fd = fopen("data.txt", "r");
    set_input_buffer(&client_context, client_fd, 0);
    printf("Client buffer set to file \"data.txt\"...\n");

    struct magpie_packet recv_packet; 
    struct magpie_packet send_packet;
    struct sockaddr_in remote_addr;
    int sk, sz, ret;
    get_mcast_socket(&sk, &remote_addr, MCAST_ADDR, MCAST_PORT);

    /** key exchange (xx variant): client **/

    //First Handshake XX Packet
    ret = generate_packet(&client_context, &send_packet);
    sendto(sk, (char*) &send_packet, sizeof(struct magpie_packet), 0, 
    (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));   


    while (true) {
        /wait to receive from the server
        if ((sz = receive(sk, (char*) &recv_packet)) == sizeof(struct magpie_packet)) {
            if (recv_packet.meta.sender_id != client_context.local_id)
                handle_packet(&client_context, &recv_packet);
        } else if (sz != 0) {
            logger(INFO, "We received a malformed message :/ (size=%d)", sz);
            break;
        }
        ret = generate_packet(&client_context, &send_packet);
        if (ret == HC_ONE_TO_SEND)
            sendto(sk, (char*) &send_packet, sizeof(struct magpie_packet), 0, 
                (struct sockaddr *)&remote_addr, sizeof(struct sockaddr));   
        else if (ret == HC_TRANSFER_COMPELTE)
            break;
        else 
            logger(DEBUG, "ret=%d", ret);
        
        usleep(10000);
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
    logger(INFO, "Receive socket initialized [ sk=%d mcast=%s:%d ]", *sk, addr_str, mcast_port);
    return 0;
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
            logger(DEBUG, "Received %dB message. Digest: %u", mess_len, hash(mess, mess_len));
            return mess_len;
        }
    }
    
    return 0;
}