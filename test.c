//test.c

#include "magpielib.h"

int main(int argc, char *argv[])
{

    if (argc != 1)
    {
        logger(FATAL, "usage: ./test");
        exit(1);
    }

    struct magpie_context client_context;
    setup_context(&client_context, "keys/keypair_0", false);
    struct magpie_packet packet_from_client;
    printf("Client context loaded...\n");

    struct magpie_context server_context;
    setup_context(&server_context, "keys/keypair_1", true);
    struct magpie_packet packet_from_server;
    printf("Server context loaded...\n");

    // XX_1
    generate_packet(&client_context, &packet_from_client);
    handle_packet(&server_context, &packet_from_client);
    printf("XX 1 Complete...\n");

    // XX_2
    generate_packet(&server_context, &packet_from_server);
    handle_packet(&client_context, &packet_from_server);
    printf("XX 2 Complete...\n");

    // XX_3
    generate_packet(&client_context, &packet_from_client);
    handle_packet(&server_context, &packet_from_client);
    printf("XX 3 Complete...\n");

    FILE* client_in = fopen("client_input.txt", "r");
    set_input_buffer(&client_context, client_in, 0);
    printf("Client input set to file \"client_input.txt\"...\n");

    // Example code to read from local buffer instead of file buffer
    // char* out = "Hello World :)\n";
    // set_input_buffer(&client_context, out, strlen(out));
    // printf("Client buffer set to local buffer...\n");

    //FILE* client_out = fopen("client_output.txt", "w");
    //set_output_buffer(&client_context, client_out, 0);
    //printf("Client output set to file \"client_output.txt\"...\n");

    //FILE* server_in = fopen("server_in.txt", "r");
    //set_input_buffer(&server_context, server_in, 0);
    //printf("Server output set to file \"server_in.txt\"...\n");

    FILE* server_out = fopen("server_output.txt", "w");
    set_output_buffer(&server_context, server_out, 0);
    printf("Server output set to file \"server_output.txt\"...\n");

    // Example code to save to local buffer instead of file buffer
    // char out[1000000];
    // memset(out, 0, sizeof(out));
    // set_output_buffer(&server_context, out, sizeof(out));
    // printf("Server buffer set to local buffer...\n");

    printf("Client sending to server\n");
    int coutner = 0;
    int client_ret, server_ret;
    while (true) {
        client_ret = generate_packet(&client_context, &packet_from_client);
        server_ret = handle_packet(&server_context, &packet_from_client);
        printf("Loop [ counter=%d ret1=%d ret2=%d ]\n", coutner++, client_ret, server_ret);
        if (server_ret == HC_TRANSFER_COMPELTE)
            break;   
        usleep(10000);
    }

    //printf("Server sending to client\n");
    //coutner = 0;
    //while (true) {
    //    server_ret = generate_packet(&server_context, &packet_from_server);
    //    client_ret = handle_packet(&client_context, &packet_from_server);
    //    printf("Loop [ counter=%d ret1=%d ret2=%d ]\n", coutner++, client_ret, server_ret);
    //    if (client_ret == HC_TRANSFER_COMPELTE)
    //        break;   
    //    usleep(10000);
    //}

    fclose(client_in);
    
    // FILE* server_fd = fopen("server_output.txt", "w");  // Uncomment to use server local buffer
    // fwrite(out, sizeof(char), strlen(out), server_fd);  // Uncomment to use server local buffer
    
    fclose(server_out);

    return 0;
}