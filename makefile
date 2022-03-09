CC=gcc 

CFLAGS = -g -c -Wall -pedantic

all: client server generate_keys

client: client.o keys.o hydrogen.o helper.o messages.o logger.o clientlib.o
	    $(CC) -o client client.o keys.o hydrogen.o helper.o messages.o logger.o clientlib.o

server: server.o keys.o hydrogen.o helper.o messages.o logger.o serverlib.o
	    $(CC) -o server server.o keys.o hydrogen.o helper.o messages.o logger.o serverlib.o

generate_keys: generate_keys.o hydrogen.o logger.o
		$(CC) -o generate_keys generate_keys.o hydrogen.o logger.o

clean:
	rm *.o
	rm client
	rm server
	rm generate_keys
	
%.o:    %.c
	$(CC) $(CFLAGS) $*.c