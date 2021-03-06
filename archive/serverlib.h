#ifndef CLIENTLIB_H
#define CLIENTLIB_H

#include "structs.h"
#include "keys.h"

int handle_handshake_xx_1(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context);

int handle_handshake_xx_3(struct magpie_packet* packet, struct magpie_context* context);

int server_handle_packet(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context);

int server_send_test_message(struct magpie_packet* packet, struct magpie_context* context, int num);

int handle_read_request(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context);

int handle_write_request(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context);

int magpie_server_generator(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context);


#endif