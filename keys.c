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

#include "keys.h"

/*

STATIC KEYPAIRS:
    * these are constants, that are determined & known before the protocol runs
    * if you need to generate keypairs, use the generate_keys script located in this library

EXAMPLE: 

SERVER:
   public key {187, 118, 150, 119, 29, 47, 169, 69, 189, 19, 188, 81, 252, 170, 53, 182, 5, 5, 52, 171, 25, 22, 159, 200, 227, 6, 236, 113, 239, 190, 245, 24}
   secret key {160, 18, 128, 110, 56, 88, 31, 5, 22, 11, 253, 238, 77, 140, 88, 100, 166, 80, 37, 239, 173, 57, 99, 123, 45, 52, 136, 98, 161, 16, 32, 10}


CLIENT:
    public: {242, 14, 178, 229, 249, 162, 234, 146, 45, 111, 243, 52, 20, 39, 170, 12, 93, 229, 203, 192, 201, 70, 141, 235, 225, 50, 172, 0, 217, 139, 124, 25}
    secret: {174, 220, 123, 182, 231, 141, 11, 204, 183, 152, 237, 53, 163, 109, 9, 123, 209, 161, 183, 239, 249, 243, 132, 82, 177, 48, 29, 46, 223, 11, 251, 42}
*/

int load_hydro_kx_keypair(hydro_kx_keypair* kp, char* filepath) {
    FILE* fd;
    fd = fopen(filepath, "r");

    fread(kp->pk, hydro_kx_PUBLICKEYBYTES, sizeof(char), fd);
    fread(kp->sk, hydro_kx_SECRETKEYBYTES, sizeof(char), fd);
    fclose(fd);
    return 0;
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