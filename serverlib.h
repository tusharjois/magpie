#ifndef CLIENTLIB_H
#define CLIENTLIB_H

#include "helper.h"
#include "messages.h"
#include <stdlib.h>

int handle_handshake_xx_1(struct Packet* packet, struct Context* context);

int create_handshake_xx_2(struct Packet* packet, struct Context* context);

int handle_handshake_xx_3(struct Packet* packet, struct Context* context);

int handle_test_messages(struct Packet* packet, struct Context* context);

int send_test_message(struct Packet* packet, struct Context* context, int num);

#endif