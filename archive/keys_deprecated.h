//contains deprecated functions from keys.c

#ifndef keys_deprecated_H
#define keys_deprecated_H
#include <stdlib.h>

void load_local_kp(char* filepath, hydro_kx_keypair* kp);

void load_server_kp(hydro_kx_keypair* server_static_kp);

void load_client_kp(hydro_kx_keypair* client_static_kp);

void load_server_pk(hydro_kx_keypair* server_static_kp);

void load_client_pk(hydro_kx_keypair* client_static_kp);

#endif