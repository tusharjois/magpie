# magpie
a medical device secure communications library

## About

The purpose of this library is to provide a framework for secure communication between medical devices. 
Magpie makes use of [libhydrogen](https://github.com/jedisct1/libhydrogen) for secure encryption and key 
exchange using [Noise](https://noiseprotocol.org).

## Functions

The main functions are found in magpie.c. Supporting functions are found in helper.c and keys.c
For an overview of the structs defined and used in this library, see structs.h. 

An example of how this library can be used is implemented in client.c and server.c, modeling a client-server system.

### Forward Facing Functions
These should be the only functions that you need to call in your code. Everything else is taken care of behind the scenes.

#### generate_keys.c

This library implements the xx protocol for key exchange.

All clients and server must have a stored public and private key. You can generate keys locally using [this](https://github.com/tusharjois/magpie/blob/xx_handshake/generate_keys.c) script, which makes use of the Libhydrogen key generation function. Keypairs are loaded when the setup_context() function is called.

Usage: 
    ` ./generate_keypair <optional: number of keypairs to generate>`
    
Keys will be automatically stored in a file at the location keys/keypair_x where x is the index of the keypair stored. The default behavior is for the function to only generate one keypair, stored at keys/keypair_0.

#### int setup_context(struct magpie_context* context, char* key_filepath, int is_server)

***Description:***
* setup_context() initializes the context to be used throughout the program. It initializes the context struct, populates the local keypair, and sets the intial state to server or client based on who initiated the communication.
	
***Parameters:***
* context: the magpie_context struct to populate
* key_filepath: the path to the file where the local keypair is stored
* is_server: boolean true if server, false if client

***Return Value:***
* returns int = 0 if successful

#### int generate_packet(struct magpie_context* context, struct magpie_packet* packet) {

***Description:***
* generate_packet() will construct a magpie_message struct based on the current state that is automatically populated in context. If the context state is AWAITING_BEGIN, it will generate the first xx handshake packet. If it is AWAITING_XX_1, it will generate the second xx packet, and so on. If the state is READY, it will populate the packet with the data in the send_buffer. This function also calles encrypt_packet(), which takes care of the security considerations such as encryption and the addition of a probe. 
	
***Parameters:***
* context: the magpie_context struct 
* packet: the magpie_packet struct that will be populated

***Return Value:***
* returns an enum magpie_handler_code indicating the status

## Sample Usage

### Client side

### Server side

#### Example: After running ./generate_keys and generating the keypair now stored in keys/keypair_0, your client can load the keypair for use:

    struct magpie_context client_context;
    setup_context(&client_context, "keys/keypair_0", false);
    


### Encryption and Decryption


### Send and Receive





