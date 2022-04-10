//client.c - the phone/tablet

#include "magpielib.h"
#include "multicast.h"


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
        //wait to receive from the server
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
