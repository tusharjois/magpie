#ifndef HELPER_H
#define HELPER_H

#include "structs.h"

void setup_context(struct magpie_context* context, void* buffer, int buffer_len, char* key_filepath, int is_server);

void reset_context(struct magpie_context* context);

int get_mcast_socket(int* sk, struct sockaddr_in* remote_addr, int mcast_addr, int mcast_port);

int load_hydro_kx_keypair(hydro_kx_keypair* kp, char* filepath);

void setup(struct magpie_context* context);

// General Functions
int encrypt_packet(char* plaintext, struct magpie_packet* packet, struct magpie_context* context); 

// decrypt the packet after receiving, include probe checking
int decrypt_packet(char* plaintext, struct magpie_packet* packet, struct magpie_context* context);

void get_my_ip_address(int* myAddress);

int recv_mc_msg(char* mess_buff, int* mess_len, struct sockaddr_in* src_addr, struct magpie_context* context);

void format_ip_address(char* buffer, int ip);

void await_message(char* mess_buff, int* from_ip, int* mess_len, struct magpie_context* context);

void deepcopy_state(struct hydro_kx_state* server_state_cpy, struct hydro_kx_state* server_state);

// send the message over multicast to the remote address
int send_mc_msg(char* msg_buff, int msg_len, struct magpie_context* context);

unsigned int hash(char *str, int len);

void format_keypair(char* buffer, hydro_kx_keypair* kp);

double timediff(struct timeval* start, struct timeval* end);

#endif