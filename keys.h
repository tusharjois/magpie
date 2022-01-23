//keys.h

#ifndef keys_H
#define keys_H

#include <stdlib.h>

void load_server_kp(hydro_kx_keypair* server_static_kp);

void load_client_kp(hydro_kx_keypair* client_static_kp);

void load_server_pk(hydro_kx_keypair* server_static_kp);

void load_client_pk(hydro_kx_keypair* client_static_kp);

void print_keypair(hydro_kx_keypair* kp);

void format_keypair(char* buffer, hydro_kx_keypair* kp);

#endif