//client.c - the phone/tablet

#include "clientlib.h"
#include "helper.h"
#include "messages.h"

int main(int argc, char *argv[])
{
    struct Context context;
    setup(&context);

    //TODO: make this persistent instead of running once per command
    if (argc != 3)
    {
        logger(FATAL, "usage: ./client <operation> <filename>");
        exit(1);
    }

    //for now, read data from a file. eventually this will be a different source
    strcpy(context.operation, argv[1]);
    strcpy(context.filename, argv[2]);

    // TODO: Do this later
    // TODO: check operation, execute based on what it is
    /*FILE *fd = fopen(context.filename, "r");
    if (fd == NULL)
    {
        logger(FATAL, "Can't open file: %s", context.filename);
        exit(1);
    }*/

    /** key exchange (xx variant): client **/

    //load client static kp
    load_client_kp(&context.local_kp);
    char buffer[1024];
    format_keypair(buffer, &context.local_kp);
    logger(DEBUG, "Client keypair:\n%s", buffer);

    struct Packet packet;
    logger(DEBUG, "Creating & sending packet 1 from client to server");
    create_handshake_xx_1(&packet, &context);   
    send_mc_msg(&packet, sizeof(struct Packet), &context); 
    //sendto(context.ss, &packet, sizeof(struct Packet), 0, (struct sockaddr *)&(context.remote_addr), sizeof(struct sockaddr)); //broadcast to multicast port
    context.state = AWAITING_XX_2;

    //wait until you receive response from the server
    char mess_buff[MESS_BUFF_LEN];
    int from_ip, mess_len;
    while (true) {
        //wait for message
        await_message(mess_buff, &from_ip, &mess_len, &context);  // block until we receive a message
        memcpy(&packet, mess_buff, sizeof(struct Packet));

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
                    send_mc_msg(&packet, sizeof(struct Packet), &context);
                    //sendto(context.ss, &packet, sizeof(struct Packet), 0, (struct sockaddr *)&(context.remote_addr), sizeof(struct sockaddr)); //broadcast to multicast port
                    context.state = READY;
                    //client_send_test_message(&packet, &context, 1);

                    //TODO: standardize operation name
                    if (strcmp("read_file", context.operation) == 0) {
                        create_read_req(&packet, &context);
                        send_mc_msg(&packet, sizeof(struct Packet), &context);
                    }
                }
                break;
            }
            case TEST_MESSAGE: {
                logger(DEBUG, "Received a test message");
                client_handle_test_messages(&packet, &context);
                break;
            }

            case READ_FILE: {
                logger(DEBUG, "Received a read file message");
                int ret = handle_read_response(&packet, &context);
                if (ret == 0) {
                    send_mc_msg(&packet, sizeof(struct Packet), &context);
                } if (ret > 0) {
                    //we've finished sending, so exit (TODO: make this continuous)
                    exit(0);
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