CC=gcc 

CFLAGS = -g -c -Wall -pedantic

all: test client server generate_keys

test: test.o hydrogen.o logger.o magpielib.o
	    $(CC) -o test test.o hydrogen.o logger.o magpielib.o

client: client.o  hydrogen.o logger.o magpielib.o
	    $(CC) -o client client.o hydrogen.o logger.o magpielib.o

server: server.o hydrogen.o logger.o magpielib.o
	    $(CC) -o server server.o hydrogen.o logger.o magpielib.o

generate_keys: generate_keys.o hydrogen.o logger.o
		$(CC) -o generate_keys generate_keys.o hydrogen.o logger.o

clean:
	rm *.o
	rm test
	rm client
	rm server
	rm generate_keys
	
%.o:    %.c
	$(CC) $(CFLAGS) $*.c