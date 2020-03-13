#Alexa Armitage
#ama043 11158883
#CMPT 434
#Assignment 3 Makefile

CC = gcc
CFLAGS = -g
CPPFLAGS = -Wall -pedantic

all: router

router: router.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o router router.o

router.o: router.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o router.o router.c

clean:
	rm -rf *.o router
