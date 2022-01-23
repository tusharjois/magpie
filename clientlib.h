#ifndef CLIENTLIB_H
#define CLIENTLIB_H

#include "helper.h"
#include "messages.h"
#include <stdlib.h>


int create_handshake_xx_1(struct Packet* packet, struct Context* context);
    
int handle_handshake_xx_2(struct Packet* packet, struct Context* context);

int create_handshake_xx_3(struct Packet* packet, struct Context* context);

int client_handle_test_messages(struct Packet* packet, struct Context* context);

/* Send a sample message with integer num as the message content */
int client_send_test_message(struct Packet* packet, struct Context* context, int num);


#endif