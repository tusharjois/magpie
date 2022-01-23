//messages.c

#include "messages.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>


/* ask server to send the current contents specific file back to the client */
void req_send_file(int curr_index, char* filename) {
    // TODO 
}

/* ask server to delete a specific file */
void req_delete_file(int curr_index, char* filename) {
    // TODO 
}

/* ask server to create a new file */
void req_create_file(int curr_index, char* filename) {
    // TODO 
}

/* ask server to update a specific file */
void req_update_file(int curr_index, char* filename, char* update) {
    // TODO 
}

/* ask server to send back diagnostic data */
void req_send_diagnostics(int curr_index) {
    // TODO 
}

/* ask server to stop & restart */
void req_bounce(int curr_index) {
    // TODO 
}

/* tell server to add a new client pk to the list of trusted clients, can only be done by client with certain auth level */
void req_add_client_pk(int curr_index, hydro_kx_keypair new_client_static_kp, char* new_client_name) {
    // TODO 
}

/** SERVER MESSAGES **/

/* acknowledge message from client */
void ack_message(int curr_index, int mess_indx, uint16_t mess_type, int sender_IP) {
    // TODO 
}

/* send the current contents specific file back to the client */
void send_file(int curr_index, char* filename, int sender_IP){
    // TODO 
}

/* send back diagnostic data */
void send_diagnostics(int curr_index, int sender_IP){
    // TODO 
}



