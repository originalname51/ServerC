CFLAGS = -Wall

server: server.c server.h
	gcc -o server -g  server.c $(CFLAGS)

all: server

clean:
	rm -f *.o server
