
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "logger.h"
#include "hydrogen.h"

/* A script to generate a local keypair. 
The new keypair will be saved in a directory called keys and a file called keypair_x.txt */

int main(int argc, char *argv[])
{
    int num_kps = 1;
    if (argc > 2) {
        printf("Usage: ./generate_keypair <optional: number of keypairs to generate>");
    } else if (argc == 2) {
        num_kps = atoi(argv[1]);
    }

    struct stat st = {0};
    if (stat("keys", &st) == -1) {
        mkdir("keys", 0700);
    }

    char filename[50];
    FILE* fd;

    for(int i = 0; i < num_kps; i++) {
        sprintf(filename, "keys/kp%d", i);
        fd = fopen(filename, "w");

        hydro_kx_keypair new_kp;
        hydro_kx_keygen(&new_kp); 

        fwrite(new_kp.pk, hydro_kx_PUBLICKEYBYTES, sizeof(char), fd);
        fwrite(new_kp.sk, hydro_kx_SECRETKEYBYTES, sizeof(char), fd);
        fclose(fd);
    }

    return 0;

}