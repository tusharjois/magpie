#ifndef CLIENTLIB_H
#define CLIENTLIB_H

#include "structs.h"
#include "keys.h"


int create_handshake_xx_1(struct magpie_packet* packet, struct magpie_context* context);
    
int handle_handshake_xx_2(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context);

int create_read_req(struct magpie_packet* packet, struct magpie_context* context);

int handle_read_response(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context);

int create_write_req(struct magpie_packet* packet, struct magpie_context* context);

int create_write_data(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context);

int magpie_client_handle_packet(struct magpie_packet* out_packet, struct magpie_packet* in_packet, struct magpie_context* context);

#endif