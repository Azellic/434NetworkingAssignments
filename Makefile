#Alexa Armitage
#ama043 11158883
#CMPT 434
#Assignment 1 Makefile

CC = gcc
CFLAGS = -g
CPPFLAGS = -std=gnu90 -Wall -pedantic

all: server client proxy

server: server.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o server server.o

server.o: server.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o server.o server.c

client: client.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o client client.o

client.o: client.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o client.o client.c

proxy: proxy.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o proxy proxy.o

proxy.o: proxy.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o proxy.o proxy.c

clean:
	rm -rf *.o server client proxy
