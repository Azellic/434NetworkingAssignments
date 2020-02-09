#Alexa Armitage
#ama043 11158883
#CMPT 434
#Assignment 1 Makefile

CC = gcc
CFLAGS = -g
CPPFLAGS = -std=gnu90 -Wall -pedantic

all: receiver sender

receiver: receiver.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o receiver receiver.o

receiver.o: receiver.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o receiver.o receiver.c

sender: sender.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o sender sender.o

sender.o: sender.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o sender.o sender.c

forwarder: forwarder.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o forwarder forwarder.o

forwarder.o: forwarder.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o forwarder.o forwarder.c

clean:
	rm -rf *.o receiver sender forwarder
