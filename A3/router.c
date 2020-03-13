/*
* Alexa Armitage ama043 11158883
* CMPT 434
* Assignment 3 Part A
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define MAXBUFLEN 104   //3 * 26 possible routers + 25 separators + tombstone
#define NUMROUTERS 26   //25 connections + listener
#define MILSECINSEC 1000

static const int portLowLimit = 30000; /*Minimum port specified by assignment*/
static const int portUprLimit = 40000; /*Maximum port specified by assignment*/

int twoRouters = 0;
int myPort, firstOutPort, secondOutPort;
char myPortStr[6], firstOutPortStr[6], secondOutPortStr[6];
char *myName;
int routerCount = 0;
int routerTable[2][26] = {
                             {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                              'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                              'U', 'V', 'W', 'X', 'Y', 'Z',},
                             {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                              -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                              -1, -1, -1, -1, -1, -1}
                             };





/***************************************************************************
* Return a listening socket
****************************************************************************/
int getListenerSocket(void){
    struct addrinfo hints, *ai, *p;
    int listener;
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, myPortStr, &hints, &ai)) != 0) {
        fprintf(stderr, "router: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // Remove "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }
    freeaddrinfo(ai);

    if (p == NULL) {
        return -1;
    }

    if (listen(listener, 10) == -1) {
        return -1;
    }
    return listener;
}







void setDistanceToRouter(char router, int distance){
    for(int i = 0; i < 26; i++){
        if(routerTable[0][i] == router){
            routerTable[1][i] = distance;
        }
    }
}


/***************************************************************************
* Attempt to connect with a different router on port number provided
****************************************************************************/
int establishConnectionToRouter(char *port){
    int rv, numbytes;
    int sockfd;
    char namebuf[2];
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

    for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("router: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			continue;
		}
        printf("Connection found\n");
		break;
	}

    freeaddrinfo(servinfo);

    if (p == NULL) {
        //We did not find the router, this is fine
		return -1;
	}

    //Perform a handshake with the names
    if (send(sockfd, myName, 1, 0) == -1)
        perror("send");

    numbytes = recv(sockfd, namebuf, 1, 0);

    if(numbytes == -1) {
        //Found it, but failed to perform handshake
        perror("recv");
        close(sockfd);
        return -1;
    }

    namebuf[numbytes] = '\0';
    setDistanceToRouter(namebuf[0], 1);


    return sockfd;
}





/***************************************************************************
* Add a new file descriptor to pollfds
****************************************************************************/
void addToPollfds(struct pollfd *pollfds, int newfd, int *fdCount, int *fdSize)
{
    // Check if there is room in the array
    if (*fdCount == *fdSize) {
        perror("pollfds: no space left.");
        return;
    }

    pollfds[*fdCount].fd = newfd;
    pollfds[*fdCount].events = POLLIN; // Check ready-to-read

    (*fdCount)++;
}

/***************************************************************************
* Remove an file descriptor from pollfds
****************************************************************************/
void deleteFromPollfds(struct pollfd *pollfds, int i, int *fdCount){
    // Copy the fd from the end over the one to remove
    pollfds[i] = pollfds[*fdCount-1];
    (*fdCount)--;
}


/***************************************************************************
* Main method, runs a loop looking for inbound connections/messages
****************************************************************************/
int main(int argc, char *argv[]) {
    int listenSock, firstOutSock, secondOutSock, newfd, destfd, senderfd;
    char buf[MAXBUFLEN], namebuf[2];
    int fdCount = 0, pollCount;
    int fdSize = NUMROUTERS;
    int nbytes, numbytes;
    socklen_t addrlen;
    struct sockaddr_storage remoteaddr;
    struct pollfd *pollfds = malloc(sizeof *pollfds * fdSize);

    if (argc < 4 || argc > 5){
        printf("Incorrect number of arguments.\n");
        printf("Usage: ./router [name] [myPort] [theirPort] [optionalPort]\n");
        exit(1);
    }

    if(argv == NULL){
        printf("huh?\n");
    }

    myName = argv[1];
    if (myName[0] < 'A' || myName[0] > 'Z'){
        printf("Invalid router name.\n");
        exit(1);
    }
    setDistanceToRouter(myName[0], 0);

    myPort = atoi(argv[2]);
    if (myPort > portUprLimit || myPort < portLowLimit){
        printf("My port %d out of range.\n", myPort);
        exit(1);
    }
    strcpy(myPortStr, argv[2]);

    firstOutPort = atoi(argv[3]);
    if (firstOutPort > portUprLimit || firstOutPort < portLowLimit){
        printf("Remote port %d out of range.\n", firstOutPort);
        exit(1);
    }
    strcpy(firstOutPortStr, argv[3]);

    if (argc == 5){
        twoRouters = 1;
        secondOutPort = atoi(argv[4]);
        if (secondOutPort > portUprLimit || secondOutPort < portLowLimit){
            printf("Remote port %d out of range.\n", secondOutPort);
            exit(1);
        }
        strcpy(secondOutPortStr, argv[4]);
        printf("Arguments: %s %s %s %s\n", myName, myPortStr, firstOutPortStr,
            secondOutPortStr);
    }
    else{
        printf("Arguments: %s %s %s\n", myName, myPortStr, firstOutPortStr);
    }

    // Set up and get a listening socket
    listenSock = getListenerSocket();
    if (listenSock == -1) {
        fprintf(stderr, "Failed to establish listener socket\n");
        exit(1);
    }
    // Add the listener to set
    pollfds[0].fd = listenSock;
    pollfds[0].events = POLLIN;
    fdCount = 1;

    //Try to connect to first port # provided
    firstOutSock = establishConnectionToRouter(firstOutPortStr);
    if(firstOutSock != -1){
        addToPollfds(pollfds, firstOutSock, &fdCount, &fdSize);
    }

    if(twoRouters){
        secondOutSock = establishConnectionToRouter(secondOutPortStr);
        if(secondOutSock != -1){
            addToPollfds(pollfds, secondOutSock, &fdCount, &fdSize);
        }
    }


    for(;;) {
        printf("About to wait for something to happen\n" );
        pollCount = poll(pollfds, fdCount, 2 * MILSECINSEC);

        if (pollCount == -1) {
            perror("poll");
            exit(1);
        }

        // Run through the existing connections looking for data to read
        for(int i = 0; i < fdCount; i++) {
            if (pollfds[i].revents & POLLIN) { // Connection i sent something
                // If listener is ready to read, handle new connection
                if (pollfds[i].fd == listenSock) {
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listenSock, (struct sockaddr *)&remoteaddr,
                                    &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    }
                    else {
                        if((numbytes = recv(newfd, namebuf, 1, 0)) == -1) {
                            perror("recv");
                            continue;
                        }
                        namebuf[numbytes] = '\0';
                        setDistanceToRouter(namebuf[0], 1);
                        addToPollfds(pollfds, newfd, &fdCount, &fdSize);

                        printf("router: new connection on file descriptor %d\n",
                            newfd);
                    }
                }
                else {

                    // If not the listener, we're just a regular client
                    nbytes = recv(pollfds[i].fd, buf, sizeof buf, 0);
                    senderfd = pollfds[i].fd;

                    if (nbytes <= 0) {
                        // Got error or connection closed by client
                        if (nbytes == 0) {
                            // Connection closed
                            printf("pollserver: socket %d hung up\n", senderfd);
                        } else {
                            perror("recv");
                        }
                        //Cleanup dead connections
                        close(pollfds[i].fd);
                        deleteFromPollfds(pollfds, i, &fdCount);
                        //TODO:remove from paths

                    } else {
                        // We got some good data from a client

                        printf("%s\n", buf);

                        for(int j = 0; j < fdCount; j++) {
                            // Send to everyone!
                            destfd = pollfds[j].fd;

                            // Except the listener and ourselves
                            if (destfd != listenSock && destfd != senderfd) {
                                if (send(destfd, buf, nbytes, 0) == -1) {
                                    perror("send");
                                }
                            }
                        }
                    }

                } // END handle data from client
            } // END got ready-to-read from poll()
        } // END looping through file descriptors
    }
}
