#ifndef MESSAGES_H
#define MESSAGES_H

#include "structs.h"

/** General Functions **/

int encrypt_packet(char* plaintext, struct magpie_packet* packet, struct magpie_context* context); 

/* decrypt the packet after receiving, include probe checking */
int decrypt_packet(char* plaintext, struct magpie_packet* packet, struct magpie_context* context);

/* send the message over multicast to the remote address */
int send_mc_msg(char* msg_buff, int msg_len, struct magpie_context* context);












#endif