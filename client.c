//client.c - the phone/tablet

#include "clientlib.h"
#include "helper.h"
#include "messages.h"

int main(int argc, char *argv[])
{
    struct magpie_context context;
    setup(&context);

    //TODO: make this persistent instead of running once per command
    if (argc != 3)
    {
        logger(FATAL, "usage: ./client <operation> <filename> \n Possible operations: read_file, write_file");
        exit(1);
    }

    //for now, read data from a file. eventually this will be a different source
    strcpy(context.operation, argv[1]);
    strcpy(context.filename, argv[2]);

    /** key exchange (xx variant): client **/

    //load client static kp
    //load_client_kp(&context.local_kp);
    char buffer[1024];

    load_local_kp("keys/keypair_0", &context.local_kp);
    format_keypair(buffer, &context.local_kp);
    logger(DEBUG, "Client keypair:\n%s", buffer);

    struct magpie_packet packet;
    logger(DEBUG, "Creating & sending packet 1 from client to server");
    create_handshake_xx_1(&packet, &context);   
    send_mc_msg(&packet, sizeof(struct magpie_packet), &context); 
    //sendto(context.ss, &packet, sizeof(struct magpie_packet), 0, (struct sockaddr *)&(context.remote_addr), sizeof(struct sockaddr)); //broadcast to multicast port
    context.state = AWAITING_XX_2;

    //wait until you receive response from the server
    char mess_buff[MESS_BUFF_LEN];
    int from_ip, mess_len;
    while (true) {
        //wait for message
        await_message(mess_buff, &from_ip, &mess_len, &context);  // block until we receive a message
        memcpy(&packet, mess_buff, sizeof(struct magpie_packet));

        //ignore packets from self (because of multicast)
        if (packet.sender_id == context.local_ip) {
            logger(DEBUG, "Received packet from self");
            continue;
        }

        //process by type
        switch (packet.type) {
            case HANDSHAKE_XX_2: {
                int ret = handle_handshake_xx_2(&packet, &context);
                if (ret == 0) {
                    //create handshake 3
                    usleep(10000);
                    create_handshake_xx_3(&packet, &context);
                    logger(DEBUG, "Sending packet 3 from client to server");
                    send_mc_msg(&packet, sizeof(struct magpie_packet), &context);
                    //sendto(context.ss, &packet, sizeof(struct Packet), 0, (struct sockaddr *)&(context.remote_addr), sizeof(struct sockaddr)); //broadcast to multicast port
                    context.state = READY;
                    //client_send_test_message(&packet, &context, 1);

                    //TODO: standardize operation name
                    if (strcmp("read_file", context.operation) == 0) {
                        create_read_req(&packet, &context);
                        send_mc_msg(&packet, sizeof(struct magpie_packet), &context);
                    } else if (strcmp("write_file", context.operation) == 0) {
                        create_write_req(&packet, &context);
                        send_mc_msg((char*)&packet, sizeof(struct magpie_packet), &context);
                    } 
                }
                break;
            }
            /*case TEST_MESSAGE: {
                logger(DEBUG, "Received a test message");
                client_handle_test_messages(&packet, &context);
                break;
            }*/

            case READ_FILE: {
                logger(DEBUG, "Received a read file message");
                int ret = handle_read_response(&packet, &context);
                if (ret == 0) {
                    send_mc_msg((char*)&packet, sizeof(struct magpie_packet), &context);
                } if (ret > 0) {
                    //we've finished sending, so exit (TODO: make this continuous)
                    exit(0);
                }
                break;
            }

            case WRITE_FILE: {
                create_write_data(&packet, &context);
                send_mc_msg((char*)&packet, sizeof(struct magpie_packet), &context);
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