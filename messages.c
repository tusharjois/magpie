//messages.c

#include "messages.h"
#include "helper.h"

/** General Functions **/

/* encrypt the packet in order to send it, include the probe */
int encrypt_packet(char* plaintext, struct Packet* packet, struct Context* context) {

    char ciphertext[CIPHERTEXT_SIZE];

    hydro_secretbox_encrypt((uint8_t *)ciphertext, plaintext, PLAINTEXT_SIZE, 0, CONTEXT, context->session_kp.tx);
    memcpy(packet->payload, ciphertext, CIPHERTEXT_SIZE);

    //create the probe
    hydro_secretbox_probe_create(packet->probe, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.tx);

    return 0;
} 

/* decrypt the packet after receiving, include probe checking */
int decrypt_packet(char* plaintext, struct Packet* packet, struct Context* context) {

    char* ciphertext = packet->payload;
    
    //check the probe
    int ret = hydro_secretbox_probe_verify(packet->probe, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.rx);
    if (ret != 0) {
        logger(DEBUG, "Probe Failed to Verify");
        return -4;
    }

    //decrypt packet
    if (hydro_secretbox_decrypt(plaintext, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, 0, CONTEXT, context->session_kp.rx) != 0) {
        logger(DEBUG, "Message forged!");
        return -1;
    } 

    return 0;

}

static int NUM_SENT = 0;

/* send the message over multicast to the remote address */
int send_mc_msg(char* msg_buff, int msg_len, struct Context* context) {


    char local_ip_buff[128];
    char remote_ip_buff[128];

    memset(local_ip_buff, 0, 128);
    memset(remote_ip_buff, 0, 128);

    format_ip_address(local_ip_buff, context->local_ip);
    format_ip_address(remote_ip_buff, context->remote_addr.sin_addr.s_addr);

    unsigned int h = hash(msg_buff, msg_len);

    logger(DEBUG, "Sending %d byte message from %s to %s. Digest: %u", msg_len, local_ip_buff, remote_ip_buff, h);
    
    sendto(context->ss, msg_buff, msg_len, 0, 
        (struct sockaddr *)&(context->remote_addr), sizeof(struct sockaddr));

    if (++NUM_SENT >= 50) {
        logger(FATAL, "Max sends reached");
        exit(1);
    }

    logger(DEBUG, "Sent successfully");
    

    return 0;
}




/** CLIENT MESSAGES **/ 
//note: moving these to client lib soon

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



