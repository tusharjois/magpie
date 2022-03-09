# magpie
a medical device secure communications library

## About

The purpose of this library is to provide a framework for secure communication between medical devices. 
Magpie makes use of [libhydrogen](https://github.com/jedisct1/libhydrogen) for secure encryption and key 
exchange using [Noise](https://noiseprotocol.org).

## Usage

The main functions are found in clientlib.c and serverlib.c. Supporting functions are found in helper.c, messages.c, and keys.c
For an overview of the structs defined and used in this library, see structs.h. 

An example of how this library can be used is implemented in client.c and server.c, modeling a client-server system.

### Key Exchange and Generation

This library implements the xx protocol for key exchange.

All clients and server must have a stored public and private key. You can generate keys locally using [this](https://github.com/tusharjois/magpie/blob/xx_handshake/generate_keys.c) script. Keys can be loaded in the client and server main function using the load_local_kp() function found in keys.c. 

For example, after generating the keypair stored in keys/keypair_0, your client can load the keypair for use and then print it out so that you can see the keypair:

    char buffer[1024];
    load_local_kp("keys/keypair_0", &context.local_kp);
    format_keypair(buffer, &context.local_kp);
    logger(DEBUG, "Client keypair:\n%s", buffer);


### Encryption and Decryption


### Send and Receive


