#ifndef CLIENTLIB_H
#define CLIENTLIB_H

#include "structs.h"
#include "keys.h"

int handle_handshake_xx_1(struct Packet* packet, struct Context* context);

int create_handshake_xx_2(struct Packet* packet, struct Context* context);

int handle_handshake_xx_3(struct Packet* packet, struct Context* context);

int server_handle_test_messages(struct Packet* packet, struct Context* context);

int server_send_test_message(struct Packet* packet, struct Context* context, int num);

int handle_read_request(struct Packet* packet, struct Context* context, struct ReadRequest* req);

#endif