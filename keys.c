//keys.c

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
#include "hydrogen.h"

/*

    typedef struct hydro_kx_keypair {
	    uint8_t pk[hydro_kx_PUBLICKEYBYTES];
	    uint8_t sk[hydro_kx_SECRETKEYBYTES];
	} hydro_kx_keypair;

    #define hydro_kx_PUBLICKEYBYTES  32	

STATIC KEYPAIRS:
    * these are constants, that are determined & known before the protocol runs
    * if you need to generate keypairs, use the generate_keys script located in this library
    * the ones hardcoded here were determined using libhydrogen's function 'hydro_kx_keygen(&static_kp);'

EXAMPLE: 

SERVER:
   public key {187, 118, 150, 119, 29, 47, 169, 69, 189, 19, 188, 81, 252, 170, 53, 182, 5, 5, 52, 171, 25, 22, 159, 200, 227, 6, 236, 113, 239, 190, 245, 24}
   secret key {160, 18, 128, 110, 56, 88, 31, 5, 22, 11, 253, 238, 77, 140, 88, 100, 166, 80, 37, 239, 173, 57, 99, 123, 45, 52, 136, 98, 161, 16, 32, 10}


CLIENT:
    public: {242, 14, 178, 229, 249, 162, 234, 146, 45, 111, 243, 52, 20, 39, 170, 12, 93, 229, 203, 192, 201, 70, 141, 235, 225, 50, 172, 0, 217, 139, 124, 25}
    secret: {174, 220, 123, 182, 231, 141, 11, 204, 183, 152, 237, 53, 163, 109, 9, 123, 209, 161, 183, 239, 249, 243, 132, 82, 177, 48, 29, 46, 223, 11, 251, 42}
*/


void load_local_kp(char* filepath, hydro_kx_keypair* kp) {
    //load the keypair from a file located at filepath 
    //must be formatted as a key generated from the hydro_kx_keygen(&static_kp) function

    FILE* fd;
    fd = fopen(filepath, "r");

    fread(kp->pk, hydro_kx_PUBLICKEYBYTES, sizeof(char), fd);
    fread(kp->sk, hydro_kx_SECRETKEYBYTES, sizeof(char), fd);
    fclose(fd);

}


void load_server_kp(hydro_kx_keypair* server_static_kp) {
    //example key for server

    //populate the server's static kp - hardcoded or stored in a file that we read from (not published)
    //must remain consistent
    uint8_t pk[hydro_kx_PUBLICKEYBYTES] = {187, 118, 150, 119, 29, 47, 169, 69, 189, 19, 188, 81, 252, 170, 53, 182, 5, 5, 52, 171, 25, 22, 159, 200, 227, 6, 236, 113, 239, 190, 245, 24};
    uint8_t sk[hydro_kx_SECRETKEYBYTES] = {160, 18, 128, 110, 56, 88, 31, 5, 22, 11, 253, 238, 77, 140, 88, 100, 166, 80, 37, 239, 173, 57, 99, 123, 45, 52, 136, 98, 161, 16, 32, 10};

    memcpy(&(server_static_kp->pk), pk, hydro_kx_PUBLICKEYBYTES);
    memcpy(&(server_static_kp->sk), sk, hydro_kx_SECRETKEYBYTES);

}

void load_server_pk(hydro_kx_keypair* server_static_kp) {
    //just load the public key, for cases such as the client who should only know the public key

    uint8_t pk[hydro_kx_PUBLICKEYBYTES] = {187, 118, 150, 119, 29, 47, 169, 69, 189, 19, 188, 81, 252, 170, 53, 182, 5, 5, 52, 171, 25, 22, 159, 200, 227, 6, 236, 113, 239, 190, 245, 24};
    memcpy(&(server_static_kp->pk), pk, hydro_kx_PUBLICKEYBYTES);

}

void load_client_pk(hydro_kx_keypair* client_static_kp) {
    //just load the public key, for cases such as the client who should only know the public key

    uint8_t pk[hydro_kx_PUBLICKEYBYTES] = {242, 14, 178, 229, 249, 162, 234, 146, 45, 111, 243, 52, 20, 39, 170, 12, 93, 229, 203, 192, 201, 70, 141, 235, 225, 50, 172, 0, 217, 139, 124, 25};
    memcpy(&(client_static_kp->pk), pk, hydro_kx_PUBLICKEYBYTES);

}

void load_client_kp(hydro_kx_keypair* client_static_kp) {

    //populate the client's static kp - hardcoded or stored in a file that we read from (not published)
    //must remain consistent
    uint8_t pk[hydro_kx_PUBLICKEYBYTES] = {242, 14, 178, 229, 249, 162, 234, 146, 45, 111, 243, 52, 20, 39, 170, 12, 93, 229, 203, 192, 201, 70, 141, 235, 225, 50, 172, 0, 217, 139, 124, 25};
    uint8_t sk[hydro_kx_SECRETKEYBYTES] = {174, 220, 123, 182, 231, 141, 11, 204, 183, 152, 237, 53, 163, 109, 9, 123, 209, 161, 183, 239, 249, 243, 132, 82, 177, 48, 29, 46, 223, 11, 251, 42};

    memcpy(&(client_static_kp->pk), pk, hydro_kx_PUBLICKEYBYTES);
    memcpy(&(client_static_kp->sk), sk, hydro_kx_SECRETKEYBYTES);

}

void format_keypair(char* buffer, hydro_kx_keypair* kp) {
    memset(buffer, '\0', strlen(buffer));
    sprintf(&buffer[strlen(buffer)], "public = {");
    for (int i=0; i < hydro_kx_PUBLICKEYBYTES; i++) {
      sprintf(&buffer[strlen(buffer)], "%d, ", kp->pk[i]);
    }
    sprintf(&buffer[strlen(buffer)], "}");
    sprintf(&buffer[strlen(buffer)], "\nprivate = ");
    sprintf(&buffer[strlen(buffer)], "{");
    for (int i=0; i < hydro_kx_SECRETKEYBYTES; i++) {
      sprintf(&buffer[strlen(buffer)], "%d, ", kp->sk[i]);
    }
    sprintf(&buffer[strlen(buffer)], "}");
}

void print_keypair(hydro_kx_keypair* kp) {
    //for debugging
    printf("{");
    for (int i=0; i < hydro_kx_PUBLICKEYBYTES; i++) {
      printf("%d, ", kp->pk[i]);
    }
    printf("}");
    printf("\n");
    printf("{");
    for (int i=0; i < hydro_kx_SECRETKEYBYTES; i++) {
      printf("%d, ", kp->sk[i]);
    }
    printf("}\n");

}