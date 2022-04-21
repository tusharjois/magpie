#ifndef TEST_H
#define TEST_H

#include "helper.h"
#include "magpielib.h"


int setup_and_handshake(struct magpie_context* server_context, struct magpie_context* client_context, char* logger_level);

int timing_tests_small(char* logger_level);

int timing_tests_medium(char* logger_level);

int timing_tests_large(char* logger_level);

int test_generate_time(struct magpie_context* client_context, char* filepath);

int test_handle_time(struct magpie_context* client_context, struct magpie_context* server_context, char* filepath);

int test_generate_and_handle_time(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath);

int test_MITM_ciphertext_packet(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath);

int test_MITM_meta_packet(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath);

int test_MITM_probe_packet(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath);

int test_replay_attack(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath);

int test_replay_false_seq_attack(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath);

#endif