#ifndef MESSAGES_H
#define MESSAGES_H

#include "structs.h"

/** General Functions **/

int encrypt_packet(char* plaintext, struct Packet* packet, struct Context* context); 

/* decrypt the packet after receiving, include probe checking */
int decrypt_packet(char* plaintext, struct Packet* packet, struct Context* context);

/* send the message over multicast to the remote address */
int send_mc_msg(char* msg_buff, int msg_len, struct Context* context);

/** CLIENT MESSAGES **/

/* ask server to send the current contents specific file back to the client */
void req_send_file(int curr_index, char* filename);

/* ask server to delete a specific file */
void req_delete_file(int curr_index, char* filename);

/* ask server to create a new file */
void req_create_file(int curr_index, char* filename);

/* ask server to add to a specific file */
void req_update_file(int curr_index, char* filename, char* update);

/* ask server to send back diagnostic data */
void req_send_diagnostics(int curr_index);

/* ask server to stop & restart */
void req_bounce(int curr_index);

/* tell server to add a new client pk to the list of trusted clients, can only be done by client with certain auth level */
void req_add_client_pk(int curr_index, hydro_kx_keypair new_client_static_kp, char* new_client_name);

/** SERVER MESSAGES **/

/* acknowledge message from client */
void ack_message(int curr_index, int mess_indx, u_int16_t mess_type, int sender_IP);

/* send the current contents specific file back to the client */
void send_file(int curr_index, char* filename, int sender_IP);

/* send back diagnostic data */
void send_diagnostics(int curr_index, int sender_IP);

#endif