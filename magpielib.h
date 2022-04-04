#ifndef MAGPIELIB_H
#define MAGPIELIB_H

#include "structs.h"

int setup_context(struct magpie_context* context, char* key_filepath, int is_server);

int set_buffer(struct magpie_buffer* mag_buffer, void* buffer, int buffer_len);

int read_buffer(char* out_buffer, struct magpie_buffer* in_mag_buffer, int buffer_len);

int write_buffer(struct magpie_buffer* out_mag_buffer, char* in_buffer, int buffer_len);

int set_input_buffer(struct magpie_context* context, void* buffer, int buffer_len);

int set_output_buffer(struct magpie_context* context, void* buffer, int buffer_len);

int generate_handshake_xx_1(struct magpie_context* context, struct magpie_packet* packet);

int handle_handshake_xx_1(struct magpie_context* context, struct magpie_packet* packet);

int generate_handshake_xx_2(struct magpie_context* context, struct magpie_packet* packet);

int handle_handshake_xx_2(struct magpie_context* context, struct magpie_packet* packet);

int generate_handshake_xx_2(struct magpie_context* context, struct magpie_packet* packet);

int handle_handshake_xx_3(struct magpie_context* context, struct magpie_packet* packet);

int encrypt_packet(struct magpie_message* plaintext_message, struct magpie_packet* encrypted_packet, struct magpie_context* context);

int decrypt_packet(struct magpie_message* plaintext_message, struct magpie_packet* encrypted_packet, struct magpie_context* context);

int generate_packet(struct magpie_context* context, struct magpie_packet* packet);

int handle_packet(struct magpie_context* context, struct magpie_packet* packet);

unsigned int hash(char *str, int len);

void format_keypair(char* buffer, hydro_kx_keypair* kp);

int load_hydro_kx_keypair(hydro_kx_keypair* kp, char* filepath);

void format_ip_address(char* buffer, int ip);

#endif