CC=gcc 

CFLAGS = -g -c -Wall -pedantic

all: client server 

client: client.o keys.o hydrogen.o helper.o messages.o logger.o clientlib.o
	    $(CC) -o client client.o keys.o hydrogen.o helper.o messages.o logger.o clientlib.o

server: server.o keys.o hydrogen.o helper.o messages.o logger.o serverlib.o
	    $(CC) -o server server.o keys.o hydrogen.o helper.o messages.o logger.o serverlib.o

clean:
	rm *.o
	rm client
	rm server
	
%.o:    %.c
	$(CC) $(CFLAGS) $*.c