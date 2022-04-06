#ifndef HELPER_H
#define HELPER_H

#include "structs.h"

/* Magpie helper functions */

void format_keypair(char* buffer, hydro_kx_keypair* kp);

double timediff(struct timeval* start, struct timeval* end);

void deepcopy_state(struct hydro_kx_state* server_state_cpy, struct hydro_kx_state* server_state);

unsigned int hash(char *str, int len);

#endif