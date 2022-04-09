#ifndef MAGPIELIB_H
#define MAGPIELIB_H

#include "structs.h"

//"frontend" functions

int magpie_setup_context(struct magpie_context* context, char* key_filepath, int is_server, char* logger_level);

int magpie_set_input_buffer(struct magpie_context* context, void* buffer, int buffer_len);

int magpie_set_output_buffer(struct magpie_context* context, void* buffer, int buffer_len);

int magpie_generate_packet(struct magpie_context* context, struct magpie_packet* packet);

int magpie_handle_packet(struct magpie_context* context, struct magpie_packet* packet);

//"backend" functions

int set_buffer(struct magpie_buffer* mag_buffer, void* buffer, int buffer_len);

int generate_handshake_xx_1(struct magpie_context* context, struct magpie_packet* packet);

int handle_handshake_xx_1(struct magpie_context* context, struct magpie_packet* packet);

int generate_handshake_xx_2(struct magpie_context* context, struct magpie_packet* packet);

int handle_handshake_xx_2(struct magpie_context* context, struct magpie_packet* packet);

int generate_handshake_xx_3(struct magpie_context* context, struct magpie_packet* packet);

int handle_handshake_xx_3(struct magpie_context* context, struct magpie_packet* packet);

int encrypt_packet(struct magpie_message* plaintext_message, struct magpie_packet* encrypted_packet, struct magpie_context* context);

int decrypt_packet(struct magpie_message* plaintext_message, struct magpie_packet* encrypted_packet, struct magpie_context* context);

#endif