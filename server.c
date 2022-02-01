//server.c - the device/implant

#include "serverlib.h"
#include "helper.h"
#include "messages.h"

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
        memcpy(&packet, mess_buff, sizeof(struct Packet));

        //ignore packets from self
        if (packet.sender_id == context.local_ip) {
            logger(DEBUG, "Received packet from self");
            continue;
        }

        //process by type
        switch (packet.type) {

            case HANDSHAKE_XX_1: {
                int ret = handle_handshake_xx_1(&packet, &context);
                if (ret == 0) {
                    //create handshake 2
                    logger(DEBUG, "Received handshake 1 from client");
                    create_handshake_xx_2(&packet, &context);
                    usleep(10000);
                    logger(DEBUG, "Sending packet 2 from server to client");
                    send_mc_msg((char*)&packet, sizeof(struct Packet), &context);
                    context.state = AWAITING_XX_3;
                }
                break;
            }

            case HANDSHAKE_XX_3: {
                int ret = handle_handshake_xx_3(&packet, &context);
                if (ret == 0) {
                    logger(DEBUG, "Received handshake 3 from client, ready to receive.");
                    //all done with handshake
                    context.state = READY;
                }
                break;
            }

            case TEST_MESSAGE: {
                server_handle_test_messages(&packet, &context);
                break;
            }

            case READ_FILE: {
                logger(DEBUG, "Received a Read Request");
                handle_read_request(&packet, &context);
                send_mc_msg((char*)&packet, sizeof(struct Packet), &context);
                break;
            }

            case WRITE_FILE: {
                logger(DEBUG, "Received a Write Request");
                int ret = handle_write_request(&packet, &context);
                if (ret == 0) {
                    send_mc_msg((char*)&packet, sizeof(struct Packet), &context);
                } 
                break;
            }

            default: {
                logger(DEBUG, "Undefined type %d", packet.type);
                break;
            }
        }

    }
    return 0;
}