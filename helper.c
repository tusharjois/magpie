//helper.c
#include <time.h>
#include "helper.h"

/* Magpie helper functions */

double timediff(struct timeval* start, struct timeval* end) {
    return (end->tv_sec + 0.000001 * end->tv_usec) - (start->tv_sec + 0.000001 * start->tv_usec);
}

void deepcopy_state(struct hydro_kx_state* state_cpy, struct hydro_kx_state* state) {

    memcpy(&(state_cpy->eph_kp), &(state->eph_kp), sizeof(struct hydro_kx_keypair));
    memcpy(&(state_cpy->h_st), &(state->h_st), sizeof(struct hydro_hash_state));

}

int read_from_mag_buffer(char* out_buffer, struct magpie_buffer* in_mag_buffer, int num_bytes) {
    if (num_bytes <= 0)
        return 0;

    int n = 0;
    if (in_mag_buffer->is_io_buffer) {
        char c;
        while ((c = fgetc(in_mag_buffer->io_buffer)) != EOF) {
            out_buffer[n++] = c;
            if (n >= num_bytes)
                break;
        }
        if (c == EOF)
            in_mag_buffer->is_empty = 1;
    } else {
        if (in_mag_buffer->buffer_len - in_mag_buffer->num_bytes_read < num_bytes)
            n = in_mag_buffer->buffer_len - in_mag_buffer->num_bytes_read;
        else
            n = num_bytes;
        memcpy(out_buffer, &in_mag_buffer->char_buffer[in_mag_buffer->num_bytes_read], n);
        in_mag_buffer->num_bytes_read += n;
        if (in_mag_buffer->buffer_len == in_mag_buffer->num_bytes_read)
            in_mag_buffer->is_empty = 1;
    }
    return n;
}

int write_to_mag_buffer(struct magpie_buffer* out_mag_buffer, char* in_buffer, int num_bytes) {
    if (num_bytes <= 0)
        return 0;
        
    int n = 0;
    if (out_mag_buffer->is_io_buffer) {
        for(int i = 0; i < num_bytes; i++)
            fputc(in_buffer[i], out_mag_buffer->io_buffer);
    } else {
        if (out_mag_buffer->buffer_len - out_mag_buffer->num_bytes_read < num_bytes)
            n = out_mag_buffer->buffer_len - out_mag_buffer->num_bytes_read;
        else
            n = num_bytes;
        memcpy(&out_mag_buffer->char_buffer[out_mag_buffer->num_bytes_read], in_buffer, n);
        out_mag_buffer->num_bytes_read += n;
    }
    return num_bytes;
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
