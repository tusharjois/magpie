#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdlib.h>
#include "hydrogen.h"

#define PAYLOAD_SIZE 1400
#define PLAINTEXT_SIZE (PAYLOAD_SIZE - hydro_secretbox_HEADERBYTES)
#define CIPHERTEXT_SIZE (PAYLOAD_SIZE)
#define FIRST_MESSAGE_IDX 1
#define MAX_FILE_NAME 256
#define START_SEQ_NUM 1

enum Type {
    HANDSHAKE_XX_1 = 1, 
    HANDSHAKE_XX_2 = 2,
    HANDSHAKE_XX_3 = 3,
    HANDSHAKE_XX_4 = 4,
    READ_FILE = 5,
    WRITE_FILE = 6,
    DELETE_FILE = 7,
    TEST_MESSAGE = 8
};

struct Packet {
    uint16_t seq_num;
    enum Type type;
    int sender_id;
    uint8_t probe[hydro_secretbox_PROBEBYTES];
    char payload[PAYLOAD_SIZE];
};


/* operation request initiated by the client */
struct ReadRequest {
    char filename[MAX_FILE_NAME];
};

struct TestRequest {
    int num;
};



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