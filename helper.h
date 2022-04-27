#ifndef HELPER_H
#define HELPER_H

#include "structs.h"

/* Magpie helper functions */

double timediff(struct timeval* start, struct timeval* end);

void deepcopy_state(struct hydro_kx_state* server_state_cpy, struct hydro_kx_state* server_state);

int read_from_mag_buffer(char* out_buffer, struct magpie_buffer* in_mag_buffer, int num_bytes);

int write_to_mag_buffer(struct magpie_buffer* out_mag_buffer, char* in_buffer, int num_bytes);

unsigned int hash(char *str, int len);

#endif