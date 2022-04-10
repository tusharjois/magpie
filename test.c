//test.c

#include <sys/time.h>
#include "helper.h"
#include "magpielib.h"

int test_generate_time(struct magpie_context* client_context, char* filepath);
int test_generate_and_handle_time(struct magpie_context* client_context, struct magpie_context* server_context, char* filepath);

int main(int argc, char *argv[])
{
    printf("Beginning test...\n");

    if (argc != 2)
    {
        logger(FATAL, "usage: ./test <logger level>");
        exit(1);
    }

    char* logger_level = argv[1];
    struct magpie_context client_context;
    struct magpie_context server_context;
    setup_and_handshake(&server_context, &client_context, logger_level);

    printf("\n Testing both just generate\n");

    test_generate_time(&client_context, "test_1M.txt");
    test_generate_time(&client_context, "test_2M.txt");
    test_generate_time(&client_context, "test_3M.txt");
    test_generate_time(&client_context, "test_4M.txt");

    printf("\n Testing both generate and handle\n");
    printf("Reset contexts\n");
    setup_and_handshake(&server_context, &client_context, logger_level);
    
    test_generate_and_handle_time(&client_context, &server_context, "test_1M.txt");
    test_generate_and_handle_time(&client_context, &server_context, "test_2M.txt");
    test_generate_and_handle_time(&client_context, &server_context, "test_3M.txt");
    test_generate_and_handle_time(&client_context, &server_context, "test_4M.txt");
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


int test_full_sequence(struct magpie_context* server_context, struct magpie_context* client_context) {

    struct timeval tv_start;
    struct timeval tv_end;
    struct magpie_packet packet_from_client;
    struct magpie_packet packet_from_server;
    // FILE* client_in = fopen("client_input.txt", "r");
    // magpie_set_input_buffer(&client_context, client_in, 0);
    // printf("Client input set to file \"client_input.txt\"...\n");

    // Example code to read from local buffer instead of file buffer
    char* in = "Hello World :)\n"; //size 10
    //char* in = "Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678"; //size 100
    //char* in = "Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 12345678901234567890123456789012345678912345678901234"; //size 500
    //char* in = "Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 123456789012345678901234567890123456789123456789012345678 Hello World :). This is a longer sentence. 1234567890123456789012345678901234567891234567890"; //size 1000
    int num_in_bytes = sizeof(in);
    magpie_set_input_buffer(client_context, in, strlen(in));
    printf("Client buffer set to local buffer...\n");

    // FILE* client_out = fopen("client_output.txt", "w");
    // magpie_set_output_buffer(&client_context, client_out, 0);
    // printf("Client output set to file \"client_output.txt\"...\n");

    // FILE* server_in = fopen("server_in.txt", "r");
    // magpie_set_input_buffer(&server_context, server_in, 0);
    // printf("Server output set to file \"server_in.txt\"...\n");

    // FILE* server_out = fopen("server_output.txt", "w");
    // magpie_set_output_buffer(&server_context, server_out, 0);
    //printf("Server output set to file \"server_output.txt\"...\n");

    // Example code to save to local buffer instead of file buffer
    char out[1000000];
    memset(out, 0, sizeof(out));
    magpie_set_output_buffer(server_context, out, sizeof(out));
    printf("Server buffer set to local buffer...\n");

    printf("Client generating packet to send\n");
    gettimeofday(&tv_start, NULL);
    magpie_generate_packet(client_context, &packet_from_client);
    gettimeofday(&tv_end, NULL);
    double generate_time = timediff(&tv_start, &tv_end);
        
    printf("Server handling client message\n");
    gettimeofday(&tv_start, NULL);
    magpie_handle_packet(server_context, &packet_from_client);
    gettimeofday(&tv_end, NULL);
    double handle_time = timediff(&tv_start, &tv_end);
    //logger(DEBUG, "Loop [ counter=%d ret1=%d ret2=%d ]\n", counter++, client_ret, server_ret);

    printf("Size of message: %d\n %f\n Generate time: %f\n Handle time: %f\n", num_in_bytes, generate_time, handle_time);

    //printf("Server sending to client\n");
    //coutner = 0;
    //while (true) {
    //    server_ret = magpie_generate_packet(&server_context, &packet_from_server);
    //    client_ret = magpie_handle_packet(&client_context, &packet_from_server);
    //    printf("Loop [ counter=%d ret1=%d ret2=%d ]\n", coutner++, client_ret, server_ret);
    //    if (client_ret == HC_TRANSFER_COMPELTE)
    //        break;   
    //    usleep(10000);
    //}

    //fclose(client_in);

    FILE* server_out = fopen("server_output.txt", "w");  // Uncomment to use server local buffer
    fwrite(out, sizeof(char), strlen(out), server_out);  // Uncomment to use server local buffer
    fclose(server_out);

    return 0;
}

int test_generate_time(struct magpie_context* client_context, char* filepath) {
    //just to test how long it takes to generate packets for a given file
    struct timeval tv_start;
    struct timeval tv_end;
    
    FILE* client_in = fopen(filepath, "r");
    magpie_set_input_buffer(client_context, client_in, 0);
    struct magpie_packet packet_from_client;

    //start timer
    gettimeofday(&tv_start, NULL);

    int counter = 0;
    int client_ret;
    while (true) {
        client_ret = magpie_generate_packet(client_context, &packet_from_client);
        //printf("Loop [ counter=%d ret1=%d ]\n", coutner++, client_ret);
        if (client_ret == HC_TRANSFER_COMPELTE) {
           break;
        }   
    }
    gettimeofday(&tv_end, NULL);
    double generate_time = timediff(&tv_start, &tv_end);

    printf("time to generate packets for file: %f\n", generate_time);
    fclose(client_in);

    return 0;
}

int test_generate_and_handle_time(struct magpie_context* client_context, struct magpie_context* server_context, char* filepath) {
    //just to test how long it takes to generate packets for a given file
    struct timeval tv_start;
    struct timeval tv_end;

    struct magpie_packet packet_from_client;
    
    FILE* client_in = fopen(filepath, "r");
    magpie_set_input_buffer(client_context, client_in, 0);

    FILE* server_out = fopen("server_output.txt", "w");
    magpie_set_output_buffer(server_context, server_out, 0);

    //start timer
    gettimeofday(&tv_start, NULL);

    int client_ret, server_ret;
    while (true) {
        client_ret = magpie_generate_packet(client_context, &packet_from_client);
        server_ret = magpie_handle_packet(server_context, &packet_from_client);
        //printf("Loop [ counter=%d ret1=%d ]\n", coutner++, client_ret);
        if (server_ret == HC_TRANSFER_COMPELTE) {
           break;
        }   
    }
    gettimeofday(&tv_end, NULL);
    double generate_time = timediff(&tv_start, &tv_end);

    printf("time to both generate and handle packets: %f\n", generate_time);
    fclose(client_in);

    return 0;
}