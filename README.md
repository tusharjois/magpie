# magpie
a medical device secure communications library

## About

The purpose of this library is to provide a framework for secure communication between medical devices. 
Magpie makes use of [libhydrogen](https://github.com/jedisct1/libhydrogen) for secure encryption and key 
exchange using the Noise Protocol.

## Usage

The main functions are found in clientlib.c and serverlib.c. Supporting functions are found in helper.c, messages.c, and keys.c
For an overview of the structs defined and used in this library, see structs.h. 

An example of how this library can be used is implemented in client.c and server.c, modeling a client-server system.

### Key Exchange

This library implements the xx protocol for key exchange. 

### Encryption and Decryption


### Send and Receive


