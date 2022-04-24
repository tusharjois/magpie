//keys.h

#ifndef keys_H
#define keys_H

#include <stdlib.h>
#include "libhydrogen/hydrogen.h"

int load_hydro_kx_keypair(hydro_kx_keypair* kp, char* filepath);

void format_keypair(char* buffer, hydro_kx_keypair* kp);

void print_keypair(hydro_kx_keypair* kp);

#endif