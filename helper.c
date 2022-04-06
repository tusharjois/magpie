//helper.c
#include <time.h>
#include "helper.h"

/* Magpie helper functions */

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

double timediff(struct timeval* start, struct timeval* end) {
    return (end->tv_sec + 0.000001 * end->tv_usec) - (start->tv_sec + 0.000001 * start->tv_usec);
}

void deepcopy_state(struct hydro_kx_state* state_cpy, struct hydro_kx_state* state) {

    memcpy(&(state_cpy->eph_kp), &(state->eph_kp), sizeof(struct hydro_kx_keypair));
    memcpy(&(state_cpy->h_st), &(state->h_st), sizeof(struct hydro_hash_state));

}

/* not for security purposes ;) dbj2 hash */
unsigned int hash(char *str, int len) {
    unsigned int hash = 5381;
    int c;
    if (len==0) {
        while ((c = *str++))
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    } else {
        for (int i = 0; i < len; i++)
            hash = ((hash << 5) + hash) + str[i]; /* hash * 33 + c */
    }
    return hash;
}
