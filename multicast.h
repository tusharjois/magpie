#ifndef MULTICAST_H
#define MULTICAST_H

#include "structs.h"

void await_message(char* mess_buff, int* from_ip, int* mess_len, struct magpie_context* context);

int recv_mc_msg(char* mess_buff, int* mess_len, struct sockaddr_in* src_addr, struct magpie_context* context);

int send_mc_msg(char* msg_buff, int msg_len, struct magpie_context* context);

int get_mcast_socket(int* sk, struct sockaddr_in* remote_addr, int mcast_addr, int mcast_port);

void get_my_ip_address(int* myAddress);

void format_ip_address(char* buffer, int ip);

int receive(int sk, char* mess);

#endif