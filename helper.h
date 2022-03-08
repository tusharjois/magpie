#ifndef HELPER_H
#define HELPER_H

#include "structs.h"

void setup(struct magpie_context* context);

void get_my_ip_address(int* myAddress);

void format_ip_address(char* buffer, int ip);

void await_message(char* mess_buff, int* from_ip, int* mess_len, struct magpie_context* context);

void setup_mcast(struct magpie_context* context);

void deepcopy_state(struct hydro_kx_state* server_state_cpy, struct hydro_kx_state* server_state);

unsigned int hash(char *str, int len);

#endif