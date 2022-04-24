//test.c

#include <sys/time.h>
#include <assert.h>

#include "test.h"

int main(int argc, char *argv[])
{
    printf("Beginning test...\n");

    if (argc != 2)
    {
        logger(FATAL, "usage: ./test <logger level>");
        exit(1);
    }

    char* logger_level = argv[1];
    //timing_tests(logger_level);

    struct magpie_context client_context;
    struct magpie_context server_context;
    setup_and_handshake(&server_context, &client_context, logger_level);

    timing_tests_small(logger_level);
    timing_tests_medium(logger_level);
    //timing_tests_large(logger_level);

    test_MITM_ciphertext_packet(&client_context, &server_context, "testfiles/test_1M.txt", "MITM.txt");
    test_MITM_meta_packet(&client_context, &server_context, "testfiles/test_1M.txt", "MITM.txt");
    test_MITM_probe_packet(&client_context, &server_context, "testfiles/test_1M.txt", "MITM.txt");
    
    setup_and_handshake(&server_context, &client_context, logger_level); //need to reset context 
    test_replay_attack(&client_context, &server_context, "testfiles/test_1M.txt", "MITM.txt");

    setup_and_handshake(&server_context, &client_context, logger_level);
    test_replay_false_seq_attack(&client_context, &server_context, "testfiles/test_1M.txt", "MITM.txt");

    logger(INFO, "All tests successful!");
    return 0;
}

int timing_tests_large(char* logger_level){

    struct magpie_context client_context;
    struct magpie_context server_context;

    /* Time test 1: How long just to generate the packets (Client side) */
    printf("\n Testing just generate (large)\n");
    setup_and_handshake(&server_context, &client_context, logger_level);

    test_generate_time(&client_context, "testfiles/test_100M.txt");
    test_generate_time(&client_context, "testfiles/test_250M.txt");
    test_generate_time(&client_context, "testfiles/test_500M.txt");
    test_generate_time(&client_context, "testfiles/test_750M.txt");
    test_generate_time(&client_context, "testfiles/test_1000M.txt");

    /* Time test 2: How long to just handle the packets (Server side) */
    printf("\n Testing just handle time (large)\n"); 
    setup_and_handshake(&server_context, &client_context, logger_level); //need to reset context since in test 1 it never gets "handled"

    test_handle_time(&client_context, &server_context, "testfiles/test_100M.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_250M.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_500M.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_750M.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_1000M.txt");

    /* Time test 3: How long to both generate and handle? */
    printf("\n Testing both generate and handle (large)\n"); 
    setup_and_handshake(&server_context, &client_context, logger_level); //need to reset context since in test 1 it never gets "handled"
    
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_100M.txt", "testfiles/100M_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_250M.txt", "testfiles/250M_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_500M.txt", "testfiles/500M_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_750M.txt", "testfiles/750M_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_1000M.txt", "testfiles/1000M_out.txt");
   
    return 0;
}

int timing_tests_small(char* logger_level){

    struct magpie_context client_context;
    struct magpie_context server_context;

    /* Time test 1: How long just to generate the packets (Client side) */
    printf("\n Testing just generate (small)\n");
    setup_and_handshake(&server_context, &client_context, logger_level);

    test_generate_time(&client_context, "testfiles/test_100.txt");
    test_generate_time(&client_context, "testfiles/test_250.txt");
    test_generate_time(&client_context, "testfiles/test_500.txt");
    test_generate_time(&client_context, "testfiles/test_750.txt");
    test_generate_time(&client_context, "testfiles/test_1000.txt");

    /* Time test 2: How long to just handle the packets (Server side) */
    printf("\n Testing just handle time (small) \n"); 
    setup_and_handshake(&server_context, &client_context, logger_level); //need to reset context since in test 1 it never gets "handled"

    test_handle_time(&client_context, &server_context, "testfiles/test_100.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_250.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_500.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_750.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_1000.txt");

    /* Time test 3: How long to both generate and handle? */
    printf("\n Testing both generate and handle(small) \n"); 
    setup_and_handshake(&server_context, &client_context, logger_level); //need to reset context since in test 1 it never gets "handled"
    
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_100.txt", "testfiles/100_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_250.txt", "testfiles/250_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_500.txt", "testfiles/500_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_750.txt", "testfiles/750_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_1000.txt", "testfiles/1000_out.txt");
   
    return 0;
}

int timing_tests_medium(char* logger_level){

    struct magpie_context client_context;
    struct magpie_context server_context;

    /* Time test 1: How long just to generate the packets (Client side) */
    printf("\n Testing just generate (medium)\n");
    setup_and_handshake(&server_context, &client_context, logger_level);

    test_generate_time(&client_context, "testfiles/test_100k.txt");
    test_generate_time(&client_context, "testfiles/test_250k.txt");
    test_generate_time(&client_context, "testfiles/test_500k.txt");
    test_generate_time(&client_context, "testfiles/test_750k.txt");
    test_generate_time(&client_context, "testfiles/test_1000k.txt");

    /* Time test 2: How long to just handle the packets (Server side) */
    printf("\n Testing just handle time (medium) \n"); 
    setup_and_handshake(&server_context, &client_context, logger_level); //need to reset context since in test 1 it never gets "handled"

    test_handle_time(&client_context, &server_context, "testfiles/test_100k.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_250k.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_500k.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_750k.txt");
    test_handle_time(&client_context, &server_context, "testfiles/test_1000k.txt");

    /* Time test 3: How long to both generate and handle? */
    printf("\n Testing both generate and handle(medium) \n"); 
    setup_and_handshake(&server_context, &client_context, logger_level); //need to reset context since in test 1 it never gets "handled"
    
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_100k.txt", "testfiles/100k_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_250k.txt", "testfiles/250k_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_500k.txt", "testfiles/500k_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_750k.txt", "testfiles/750k_out.txt");
    test_generate_and_handle_time(&client_context, &server_context, "testfiles/test_1000k.txt", "testfiles/1000k_out.txt");
   
    return 0;
}

int setup_and_handshake(struct magpie_context* server_context, struct magpie_context* client_context, char* logger_level) {

    //"Client" sets up their context
    magpie_setup_context(client_context, "keys/keypair0", false, logger_level);
    struct magpie_packet packet_from_client;
    printf("Client context loaded...\n");

    // "Server" sets up their context
    magpie_setup_context(server_context, "keys/keypair1", true, logger_level);
    struct magpie_packet packet_from_server;
    printf("Server context loaded...\n");

    // XX_1 
    printf("Starting handshake...\n");
    magpie_generate_packet(client_context, &packet_from_client);
    magpie_handle_packet(server_context, &packet_from_client);
    printf("XX 1 Complete...\n");

    // XX_2
    magpie_generate_packet(server_context, &packet_from_server);
    magpie_handle_packet(client_context, &packet_from_server);
    printf("XX 2 Complete...\n");

    // XX_3
    magpie_generate_packet(client_context, &packet_from_client);
    magpie_handle_packet(server_context, &packet_from_client);
    printf("XX 3 Complete...\n");

    return 0;
}

int test_generate_time(struct magpie_context* client_context, char* filepath) {
    //just to test how long it takes to generate packets for a given file
    struct timeval tv_start;
    struct timeval tv_end;
    
    FILE* client_in = fopen(filepath, "r");
    magpie_set_input_buffer(client_context, client_in, 0);
    struct magpie_packet packet_from_client;

    int client_ret;
    double total_time = 0;
    while (true) {
        gettimeofday(&tv_start, NULL);
        client_ret = magpie_generate_packet(client_context, &packet_from_client);
        gettimeofday(&tv_end, NULL);
        double gen_time = timediff(&tv_start, &tv_end);
        total_time += gen_time;
        if (client_ret == HC_TRANSFER_COMPLETE) {
           break;
        }   
    }

    printf("time to generate packets for file: %f\n", total_time);
    fclose(client_in);

    return 0;
}

int test_handle_time(struct magpie_context* client_context, struct magpie_context* server_context, char* filepath) {

    struct timeval tv_start;
    struct timeval tv_end;

    struct magpie_packet packet_from_client;
    memset(&packet_from_client, 0, sizeof(struct magpie_packet));
    
    FILE* client_in = fopen(filepath, "r");
    magpie_set_input_buffer(client_context, client_in, 0);

    FILE* server_out = fopen("testfiles/server_output.txt", "w");
    magpie_set_output_buffer(server_context, server_out, 0);

    int server_ret;
    double total_time = 0;
    while (true) {
        magpie_generate_packet(client_context, &packet_from_client);
        gettimeofday(&tv_start, NULL);
        server_ret = magpie_handle_packet(server_context, &packet_from_client);
        gettimeofday(&tv_end, NULL);
        double handle_time = timediff(&tv_start, &tv_end);
        total_time += handle_time;
        if (server_ret == HC_TRANSFER_COMPLETE) {
           break;
        }   
    }

    printf("time to just handle packets: %f\n", total_time);
    fclose(server_out);
    fclose(client_in);

    return 0;
}


int test_generate_and_handle_time(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath) {
    //just to test how long it takes to generate packets for a given file and handle them
    struct timeval tv_start;
    struct timeval tv_end;

    struct magpie_packet packet_from_client;
    memset(&packet_from_client, 0, sizeof(struct magpie_packet));
    
    FILE* client_in = fopen(in_filepath, "r");
    magpie_set_input_buffer(client_context, client_in, 0);

    FILE* server_out = fopen(out_filepath, "w");
    magpie_set_output_buffer(server_context, server_out, 0);

    //start timer
    gettimeofday(&tv_start, NULL);

    int server_ret;
    while (true) {
        magpie_generate_packet(client_context, &packet_from_client);
        server_ret = magpie_handle_packet(server_context, &packet_from_client);
        //printf("Loop [ counter=%d ret1=%d ]\n", coutner++, client_ret);
        if (server_ret == HC_TRANSFER_COMPLETE) {
           break;
        }   
    }
    gettimeofday(&tv_end, NULL);
    double generate_time = timediff(&tv_start, &tv_end);

    printf("time to both generate and handle packets: %f\n", generate_time);
    fclose(server_out);
    fclose(client_in);

    return 0;
}

int test_MITM_ciphertext_packet(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath) {

    printf("Testing MITM ciphertext\n");
    struct magpie_packet packet_from_client;
    memset(&packet_from_client, 0, sizeof(struct magpie_packet));

    FILE* client_in = fopen(in_filepath, "r");
    magpie_set_input_buffer(client_context, client_in, 0);

    FILE* server_out = fopen(out_filepath, "w");
    magpie_set_output_buffer(server_context, server_out, 0);

    //client generates a packet from the file
    magpie_generate_packet(client_context, &packet_from_client);

    //"man in the middle" modifies the packet
    char mal[] = "Malicious text!";
    memcpy(packet_from_client.ciphertext, mal, sizeof(mal));

    //now server receives it
    int server_ret = magpie_handle_packet(server_context, &packet_from_client);

    assert(server_ret == HC_DECRYPTION_FAILED);

    fclose(client_in);
    fclose(server_out);
    return 0;
}

int test_MITM_meta_packet(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath) {

    printf("Testing MITM meta sequence num\n");
    struct magpie_packet packet_from_client;
    memset(&packet_from_client, 0, sizeof(struct magpie_packet));

    FILE* client_in = fopen(in_filepath, "r");
    magpie_set_input_buffer(client_context, client_in, 0);

    FILE* server_out = fopen(out_filepath, "w");
    magpie_set_output_buffer(server_context, server_out, 0);

    //client generates a packet from the file
    magpie_generate_packet(client_context, &packet_from_client);

    //"man in the middle" modifies the packet sequence number in the metadata
    packet_from_client.meta.seq_num = 5;

    //now server receives it
    int server_ret = magpie_handle_packet(server_context, &packet_from_client);

    assert(server_ret == HC_DECRYPTION_FAILED);

    fclose(client_in);
    fclose(server_out);
    return 0;
}

int test_MITM_probe_packet(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath) {

    printf("Testing MITM probe\n");
    struct magpie_packet packet_from_client;
    memset(&packet_from_client, 0, sizeof(struct magpie_packet));

    FILE* client_in = fopen(in_filepath, "r");
    magpie_set_input_buffer(client_context, client_in, 0);

    FILE* server_out = fopen(out_filepath, "w");
    magpie_set_output_buffer(server_context, server_out, 0);

    //client generates a packet from the file
    magpie_generate_packet(client_context, &packet_from_client);

    //"man in the middle" modifies the probe
    char mal[] = "Modifying the probe!";
    memcpy(packet_from_client.probe, mal, sizeof(mal));

    //now server receives it
    int server_ret = magpie_handle_packet(server_context, &packet_from_client);

    assert(server_ret == HC_DECRYPTION_FAILED);

    fclose(client_in);
    fclose(server_out);
    return 0;
}

int test_replay_attack(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath) {
    printf("Testing replay attack \n");
    struct magpie_packet packet_from_client;
    memset(&packet_from_client, 0, sizeof(struct magpie_packet));

    FILE* client_in = fopen(in_filepath, "r");
    magpie_set_input_buffer(client_context, client_in, 0);

    FILE* server_out = fopen(out_filepath, "w");
    magpie_set_output_buffer(server_context, server_out, 0);

    //client generates a packet from the file
    magpie_generate_packet(client_context, &packet_from_client);

    //now server receives it, correctly
    int server_ret = magpie_handle_packet(server_context, &packet_from_client);
    assert(server_ret == HC_OKAY);

    //attacker sends packet to server again, and this time it fails
    server_ret = magpie_handle_packet(server_context, &packet_from_client);
    assert(server_ret == HC_DECRYPTION_FAILED);

    fclose(client_in);
    fclose(server_out);
    return 0;
}

int test_replay_false_seq_attack(struct magpie_context* client_context, struct magpie_context* server_context, char* in_filepath, char* out_filepath) {
    printf("Testing replay attack with false sequence number\n");
    struct magpie_packet packet_from_client;
    memset(&packet_from_client, 0, sizeof(struct magpie_packet));

    FILE* client_in = fopen(in_filepath, "r");
    magpie_set_input_buffer(client_context, client_in, 0);

    FILE* server_out = fopen(out_filepath, "w");
    magpie_set_output_buffer(server_context, server_out, 0);

    //client generates a packet from the file
    magpie_generate_packet(client_context, &packet_from_client);

    //now server receives it, correctly
    int server_ret = magpie_handle_packet(server_context, &packet_from_client);
    assert(server_ret == HC_OKAY);

    //attacker modifies seq to make it look like the correct packet
    packet_from_client.meta.seq_num = packet_from_client.meta.seq_num + 1;

    // still, it fails. 
    server_ret = magpie_handle_packet(server_context, &packet_from_client);
    assert(server_ret == HC_DECRYPTION_FAILED);

    fclose(client_in);
    fclose(server_out);
    return 0;
}