//serverlib.c

#include "serverlib.h" 
#include "messages.h"
#include "helper.h"

int handle_handshake_xx_1(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context) {
    logger(DEBUG, "Received handshake 1 from client, sending handshake 2...");

    //check that client is ready for xx1
    if (context->state != AWAITING_XX_1) {
        return HC_INCORRECT_CONTEXT_STATE;
    }

    //process packet
    memcpy(context->handshake_xx_1, in_packet->payload, hydro_kx_XX_PACKET1BYTES) {
    if (hydro_kx_xx_2(&context->hydro_state, context->handshake_xx_2, context->handshake_xx_1, NULL, &context->local_kp) == 0) {
        // Construct new packet
        out_packet->sender_id = context->local_id;
        out_packet->type = HANDSHAKE_XX_2;
        memcpy(out_packet->payload, context->handshake_xx_2, hydro_kx_XX_PACKET2BYTES);
        context->state = AWAITING_XX_3;
        // Done! session_kp.tx is the key for sending data to the server,
        // and session_kp.rx is the key for receiving data from the server.

        return HC_ONE_TO_SEND;
    } else {
        logger(WARN, "Failed to process client's first handshake packet");
        return HC_LIBHYDROGEN_ERROR;
    }
}

int handle_handshake_xx_3(struct magpie_packet* packet, struct magpie_context* context) {
    logger(DEBUG, "Received handshake 3 from client");

    //check that client is ready for xx3
    if (context->state != AWAITING_XX_3) {
        return HC_INCORRECT_CONTEXT_STATE;
    }

    //check that the packet type is xx3
    if (packet->type != HANDSHAKE_XX_3) {
        logger(DEBUG, "Received a packet with incorrect type (expecting handshake 3)");
        return HC_INCORRECT_PACKET_TYPE;
    }

    //process packet 3
    memcpy(context->handshake_xx_3, packet->payload, hydro_kx_XX_PACKET3BYTES);
    if (hydro_kx_xx_4(&context->hydro_state, &context->session_kp, NULL, context->handshake_xx_3, NULL) == 0) {
        logger(INFO, "Ready to receive");
        context->state = READY;
        return HC_OKAY;
        // Done! session_kp.tx is the key for sending data to the client,
        // and session_kp.rx is the key for receiving data from the client.
        // The session keys are the same as those computed by the client, but swapped.
    } else {
        logger(WARN, "Failed to process client handshake request packet 3");
        return HC_LIBHYDROGEN_ERROR;
    }


}

/* 
int server_handle_test_messages(struct Packet* packet, struct magpie_context* context) {
    
    //check that server is ready for test messages
    if (context->state != TEST) {
        return -1;
    }

    //check that this is a Test message type
    if (packet->type != TEST_MESSAGE) {
        return -2;
    }

    //check for correct order
    if (packet->seq_num != context->rx_seq_num + 1) {
        logger(DEBUG, "Out of sequence packet (Expecting: %d, Actual: %d)", context->rx_seq_num + 1, packet->seq_num);
        return -3;
    }
    
    char plaintext[PLAINTEXT_SIZE];
    char ciphertext[CIPHERTEXT_SIZE];
    memset(plaintext, 0, PLAINTEXT_SIZE);
    memcpy(ciphertext, packet->payload, CIPHERTEXT_SIZE);

    //check the probe
    int ret = hydro_secretbox_probe_verify(packet->probe, (uint8_t *) ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.rx);
    if (ret != 0) {
        logger(DEBUG, "Probe Failed to Verify");
        return -4;
    }

    //decrypt packet
    if (hydro_secretbox_decrypt(plaintext, (uint8_t *) ciphertext, CIPHERTEXT_SIZE, 0, CONTEXT, context->session_kp.rx) != 0) {
        logger(DEBUG, "Message forged!");
    } else {
        struct TestRequest req;
        memcpy(&req, plaintext, sizeof(struct TestRequest));
        logger(TRACE, "Message content: %d", req.num);
        context->rx_seq_num++;
        server_send_test_message(packet, context, req.num + 1);
    }

    return 0;
}*/

/* Send a sample message with integer num as the message content */
/*int server_send_test_message(struct Packet* packet, struct magpie_context* context, int num) {

    char plaintext[PLAINTEXT_SIZE];
    char ciphertext[CIPHERTEXT_SIZE];
    struct TestRequest req;
    memset(plaintext, 0, PLAINTEXT_SIZE);

    req.num = num;
    logger(TRACE, "Message content: %d", req.num);

    packet->sender_id = context->local_ip;
    packet->type = TEST_MESSAGE;
    memcpy(plaintext, &req, sizeof(struct TestRequest));

    hydro_secretbox_encrypt((uint8_t *) ciphertext, plaintext, PLAINTEXT_SIZE, 0, CONTEXT, context->session_kp.tx);
    memcpy(packet->payload, ciphertext, CIPHERTEXT_SIZE);

    //create the probe
    hydro_secretbox_probe_create(packet->probe, (uint8_t *) ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.tx);

    //logger(DEBUG, "hash of packet before sending: %u", hash(packet, sizeof(struct Packet)));
    
    sendto(context->ss, packet, sizeof(struct Packet), 0, (struct sockaddr *)&(context->remote_addr), sizeof(struct sockaddr));
    logger(DEBUG, "Sending example message from server to client");
    sleep(1);

    return 0;
}
*/

int handle_read_request(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context) {
    logger(DEBUG, "Received a Read Request");

    char plaintext[PLAINTEXT_SIZE];
    struct magpie_read_request req;

    //check that server is ready for messages
    if (context->state != READY) {
        return HC_INCORRECT_CONTEXT_STATE;
    }

    //check that this is a read file message type
    if (packet->type != READ_FILE) {
        return HC_INCORRECT_PACKET_TYPE;
    }

    decrypt_packet(plaintext, in_packet, context);
    memcpy(&req, plaintext, sizeof(struct magpie_read_request));
    logger(DEBUG, "Received request to send filename %s", req.filename);

    if (context->fd == NULL) {
        //then we are sending a new file
        logger(DEBUG, "First time opening this file %s", req.filename);
        strcpy(context->filename, req.filename);
        context->fd = fopen(context->filename, "r");
        if (context->fd == NULL)
        {
            logger(WARN, "Can't open file: %s", context->filename);
            return HC_FOPEN_FAILED;
            //TODO: Populate return packet with an error code to indicate bad filename
        }
    }  else {
        //else, just continue because fd and filename are already set up
        logger(DEBUG, "Continuing to send file %s", req.filename);
    }

    //fill the response with the next set of data from the file
    struct magpie_read_response res;
    memset(&res, 0, sizeof(struct magpie_read_response));
    int i;
    res.is_last_packet = 0;

    int ret = HC_ONE_TO_SEND;
    for(i = 0; i < READ_DATA_SIZE; i++) {
        char c = fgetc(context->fd);
        if (c != EOF) {
            res.data[i] = c;
        } else {
            logger(DEBUG, "This will be the last packet. EOF at index: %d", i);
            res.is_last_packet = 1;
            fclose(context->fd);
            context->fd = NULL;
            memset(context->filename, 0, MAX_FILE_NAME);
            ret = HC_LAST_TO_SEND;
            break;
        }
    }
    res.num_bytes = i;

    logger(DEBUG, "Setting up the rest of the packet");
    //Set up the rest of the packet
    out_packet->sender_id = context->local_id;
    out_packet->type = READ_FILE;
    memcpy(plaintext, &res, sizeof(struct magpie_read_response));

    logger(DEBUG, "Encrypting");
    //finally, encrypt it
    if (encrypt_packet(plaintext, out_packet, context) < 0)
        ret = HC_ENCRYPTION_FAILED;
    else
        logger(DEBUG, "Done, ready to send");

    //done, packet is ready to send
    return ret;
}

int handle_write_request(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context) {
    logger(DEBUG, "Received a Write Request");

    char plaintext[PLAINTEXT_SIZE];
    struct magpie_write_request req;

    //check that server is ready for messages
    if (context->state != READY) {
        return HC_INCORRECT_CONTEXT_STATE;
    }

    //check that this is a read file message type
    if (packet->type != WRITE_FILE) {
        return HC_INCORRECT_PACKET_TYPE;
    }

    int ret = decrypt_packet(plaintext, packet, context);
    if (ret != 0) {
        //TODO: do something more productive than just return
        return HC_DECRYPTION_FAILED;
    }

    memcpy(&req, plaintext, sizeof(struct magpie_write_request));

    //check to see if this is the first one we receive
    if (req.is_filename) {
        logger(DEBUG, "Received request to write to filename %s", req.data);
        
        //TODO: temporary because using ugrad
        logger(DEBUG, "Saving data to file temp.txt");
        strcpy(context->filename, "temp.txt");

        context->fd = fopen(context->filename, req.mode);
        if (context->fd == NULL)
        {
            logger(WARN, "Can't open file: %s", context->filename);
            return -4;
            //TODO: Populate return packet with an error code to indicate bad filename
        }
    } else {
        //this does not contain the filename and thus we've already opened it
        logger(DEBUG, "File already open, continuing to add");

        for(int i = 0; i < req.num_bytes; i++) {
            char c = req.data[i];
            fputc(c, context->fd);
        }

        if (req.is_last_packet) {
            //last packet, close file and be done
            logger(DEBUG, "Last packet, closing the file");
            fclose(context->fd);
            context->fd = NULL;
            return HC_OKAY;
        }
    }

    //send response packet to client asking for more data
    struct magpie_write_response res;
    memset(&res, 0, sizeof(struct magpie_write_response));
    strcpy(res.filename, context->filename);
    packet->sender_id = context->local_ip;
    packet->type = WRITE_FILE;
    memcpy(plaintext, &res, sizeof(struct magpie_write_response));

    logger(DEBUG, "Encrypting");
    //finally, encrypt it
    if (encrypt_packet(plaintext, out_packet, context) < 0)
        return HC_ENCRYPTION_FAILED;
    
    logger(DEBUG, "Done setting up next write response, ready to send");
    return HC_ONE_TO_SEND;
}

int server_handle_packet(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context) {
    logger(DEBUG, "Handling Packet");

    //check that server is ready for messages
    if (context->state != READY) 
        return HC_INCORRECT_CONTEXT_STATE;

    char plaintext[PLAINTEXT_SIZE];

    if (in_packet) {
        if(decrypt_packet(plaintext, in_packet, context); < 0) // DEcrypt fails if meta data is forged!
            //TODO: do something more productive than just return
            return HC_DECRYPTION_FAILED;
        
        if (context->is_io_buffer) {
            for(int i = 0; i < req.num_bytes; i++)
                fputc(req.data[i], context->fd);
        else 
            memcpy(&context->char_buffer[context->bytes_transferred], plaintext, req.num_bytes);
        
        context->bytes_transferred += req.num_bytes;
    }





    //check to see if this is the first one we receive
    if (req.is_filename) {
        logger(DEBUG, "Received request to write to filename %s", req.data);
        
        //TODO: temporary because using ugrad
        logger(DEBUG, "Saving data to file temp.txt");
        strcpy(context->filename, "temp.txt");

        context->fd = fopen(context->filename, req.mode);
        if (context->fd == NULL)
        {
            logger(WARN, "Can't open file: %s", context->filename);
            return HC_FOPEN_FAILED;
            //TODO: Populate return packet with an error code to indicate bad filename
        }
    } else {
        //this does not contain the filename and thus we've already opened it
        logger(DEBUG, "File already open, continuing to add");

        context->bytes_transferred += req.num_bytes;
        for(int i = 0; i < req.num_bytes; i++) {
            char c = req.data[i];
            fputc(c, context->fd);
        }

        if (req.is_last_packet) {
            //last packet, close file and be done
            logger(DEBUG, "Last packet, closing the file");
            reset_context(context);
            return HC_OKAY;
        }
    }

    //send response packet to client asking for more data
    struct magpie_write_response res;
    memset(&res, 0, sizeof(struct magpie_write_response));
    strcpy(res.filename, context->filename);
    out_packet->sender_id = context->local_id;
    out_packet->type = WRITE_FILE;
    memcpy(plaintext, &res, sizeof(struct magpie_write_response));

    logger(DEBUG, "Encrypting");
    //finally, encrypt it
    if (encrypt_packet(plaintext, out_packet, context) < 0)
        return HC_ENCRYPTION_FAILED;
    
    logger(DEBUG, "Done setting up next write response, ready to send");
    return HC_ONE_TO_SEND;
}


int magpie_server_generator(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context) {
    //ignore packets from self
    if (in_packet->sender_id == context->local_id) {
        logger(DEBUG, "Ignoring packet from ourself");
        return HC_MESSAGE_FROM_SELF;
    }

    if (in_packet->sender_id != context->remote_id) {
        if (context->remote_id == 0 && in_packet->type == HANDSHAKE_XX_1) {
            context->remote_id = in_packet->sender_id;
            gettimeofday(&context->operation_start, NULL);  // start operation timer
            logger(INFO, "Server has opened communication with remote_id %u", context->remote_id);
        } else {
            logger(INFO, "Server cannot open communication with remote_id %u [ remote_id=%u packet_type=%d ] ", 
                in_packet->sender_id, context->remote_id, in_packet->type);
            return HC_SERVER_BUSY;
        }
    }

    // If we're here, then in_packet->sender_id = context->remote_id
    gettimeofday(&context->last_heartbeat, NULL);  // record last heartbeat

    //process by type
    switch (in_packet->type) {
        case HANDSHAKE_XX_1: {
            return handle_handshake_xx_1(out_packet, in_packet, context);
        }

        case HANDSHAKE_XX_3: {
            return handle_handshake_xx_3(in_packet, context);
        }

        case TRANSFER: {
            return handle_packet(out_packet, in_packet, context);
        }

        case READ_FILE: {
            return handle_read_request(out_packet, in_packet, context);
        }

        case WRITE_FILE: {
            return handle_write_request(out_packet, in_packet, context);
        }

        default: {
            logger(DEBUG, "Undefined type %d", in_packet->type);
            return HC_INCORRECT_PACKET_TYPE;
        }
    }

    return HC_UNKNOWN_ERROR;
}



