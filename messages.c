//messages.c

#include "messages.h"
#include "helper.h"

/** General Functions **/

/* encrypt the packet in order to send it, include the probe */
int encrypt_packet(char* plaintext, struct magpie_packet* packet, struct magpie_context* context) {

    char ciphertext[CIPHERTEXT_SIZE];

    hydro_secretbox_encrypt((uint8_t *)ciphertext, plaintext, PLAINTEXT_SIZE, 0, CONTEXT, context->session_kp.tx);
    memcpy(packet->payload, ciphertext, CIPHERTEXT_SIZE);

    //create the probe
    hydro_secretbox_probe_create(packet->probe, (uint8_t *)ciphertext, CIPHERTEXT_SIZE, CONTEXT, context->session_kp.tx);

    //set packet number
    packet->seq_num = ++context->tx_seq_num;
    
    return 0;
} 

/* decrypt the packet after receiving, include probe checking */
int decrypt_packet(char* plaintext, struct magpie_packet* packet, struct magpie_context* context) {

    char* ciphertext = packet->payload;

    //check for correct order (Cannot decrypt out of order packets. Also, you cannot decrypt the same packet multiple times)
    if (packet->seq_num != context->rx_seq_num + 1) {
        logger(DEBUG, "Out of sequence packet (Expecting: %d, Actual: %d)", context->rx_seq_num + 1, packet->seq_num);
        return -3;
    }

    context->rx_seq_num = packet->seq_num;
    
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
int send_mc_msg(char* msg_buff, int msg_len, struct magpie_context* context) {


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




