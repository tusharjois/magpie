CC=gcc 

CFLAGS = -g -c -Wall -pedantic

all: test generate_keys

test: test.o hydrogen.o logger.o magpielib.o helper.o keys.o
	    $(CC) -o test test.o hydrogen.o logger.o magpielib.o helper.o keys.o

client: client.o  hydrogen.o logger.o magpielib.o helper.o keys.o multicast.o
	    $(CC) -o client client.o hydrogen.o logger.o magpielib.o helper.o keys.o multicast.o

server: server.o hydrogen.o logger.o magpielib.o helper.o keys.o multicast.o
	    $(CC) -o server server.o hydrogen.o logger.o magpielib.o helper.o keys.o multicast.o

generate_keys: generate_keys.o hydrogen.o logger.o 
		$(CC) -o generate_keys generate_keys.o hydrogen.o logger.o

clean:
	rm *.o
	rm test
	rm generate_keys
	
%.o:    %.c
	$(CC) $(CFLAGS) $*.c