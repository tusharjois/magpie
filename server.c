//server.c - the device/implant
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
#include "keys.h"
#include "helper.h"
#include "messages.h"
#include "logger.h"
#include "serverlib.h"

int main(int argc, char *argv[])
{
    struct Context context;
    setup(&context);

    if (argc != 1)
    {
        logger(FATAL, "usage: ./server");
        exit(1);
    }

    /** key exchange (kk variant): server **/

    //load server static kp
    load_server_kp(&context.local_kp);
    char buffer[1024];
    format_keypair(buffer, &context.local_kp);
    logger(DEBUG, "Server keypair:\n%s", buffer);

    struct Packet packet;
    char mess_buff[MESS_BUFF_LEN];
    int from_ip, mess_len;
    context.state = AWAITING_XX_1;

    while (true) {

        //wait to receive from the client
        await_message(mess_buff, &from_ip, &mess_len, &context);
        format_ip_address(buffer, from_ip);

        //process the initial request from the client & populate packet2
        memcpy(&packet, mess_buff, sizeof(struct Packet));

        if (packet.sender_id == context.local_ip) {
            logger(DEBUG, "Received packet from self");
            continue;
        }

        int ret = handle_handshake_xx_1(&packet, &context);
        if (ret == 0) {
            //create handshake 2
            logger(DEBUG, "Received handshake 1 from client");
            create_handshake_xx_2(&packet, &context);
            logger(DEBUG, "Sending packet 2 from server to client");
            sendto(context.ss, &packet, sizeof(struct Packet), 0, (struct sockaddr *)&(context.remote_addr), sizeof(struct sockaddr)); //broadcast to multicast port
            context.state = AWAITING_XX_3;
            continue;
        }

        ret = handle_handshake_xx_3(&packet, &context);
        if (ret == 0) {
            logger(DEBUG, "Received handshake 3 from client, ready to receive.");
            //all done with handshake
            context.state = TEST;
            continue;
        }

        ret = handle_test_messages(&packet, &context);
        if (ret == 0) {
            continue;
        }

    }
    return 0;
}