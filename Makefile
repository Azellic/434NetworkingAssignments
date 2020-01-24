#Alexa Armitage
#ama043 11158883
#CMPT 434
#Assignment 1 Makefile

CC = gcc
CFLAGS = -g
CPPFLAGS = -std=gnu90 -Wall -pedantic

all: server client

server: server.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o server server.o

server.o: server.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o server.o server.c

client: client.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o client client.o

client.o: client.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o client.o client.c

clean:
	rm -rf *.o server client
