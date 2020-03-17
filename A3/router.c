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
#include <time.h>

#define MAXBUFLEN 129   //4 * 26 possible routers + 25 separators + tombstone
#define NUMROUTERS 26   //25 connections + listener
#define MILSECINSEC 1000
#define TRUE 1
#define FALSE 0
#define OFFSET 65

static const int portLowLimit = 30000; /*Minimum port specified by assignment*/
static const int portUprLimit = 40000; /*Maximum port specified by assignment*/
const char delim = ',';
const char separator[2] = ":";
const int offset = 'A';

int twoRouters = FALSE, firstIsConnected = FALSE, secondIsConnected = FALSE;
int myPort, firstOutPort, secondOutPort;
char myPortStr[6], firstOutPortStr[6], secondOutPortStr[6];
char *myName, firstOutName = '-', secondOutName = '-';
char nameOfPollfd[NUMROUTERS];
int routerTable[3][NUMROUTERS] = {
                             {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                              'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                              'U', 'V', 'W', 'X', 'Y', 'Z'},
                             {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                              -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                              -1, -1, -1, -1, -1, -1},
                             {'-', '-', '-', '-', '-', '-', '-', '-', '-', '-',
                              '-', '-', '-', '-', '-', '-', '-', '-', '-', '-',
                              '-', '-', '-', '-', '-', '-'}
                             };
//routerMessages[sender name index][destination name index]
int routerMessages[NUMROUTERS][NUMROUTERS]; //A=0, Z=25, stores dist from msgs


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


/***************************************************************************
* Sets the distance and next hop to whichever router matches first parameter
****************************************************************************/
void setDistanceAndNextHop(char router, int distance, char next){
    for(int i = 0; i < NUMROUTERS; i++){
        if(routerTable[0][i] == router){
            if(distance > 99)
                distance = 99;
            routerTable[1][i] = distance;
            routerTable[2][i] = next;
        }
    }
}


/***************************************************************************
* Add a new file descriptor to pollfds
****************************************************************************/
void addToPollfds(struct pollfd *pollfds, int newfd, int *fdCount,
    int *fdSize, char name){
    // Check if there is room in the array
    if (*fdCount == *fdSize) {
        perror("pollfds: no space left.");
        return;
    }

    nameOfPollfd[*fdCount] = name;
    pollfds[*fdCount].fd = newfd;
    pollfds[*fdCount].events = POLLIN; // Check ready-to-read
    (*fdCount)++;
}


/***************************************************************************
* Remove a file descriptor from pollfds, and its name from nameOfPollfd
****************************************************************************/
void deleteFromPollfds(struct pollfd *pollfds, int i, int *fdCount){
    // Copy the fd from the end over the one to remove
    pollfds[i] = pollfds[*fdCount-1];
    nameOfPollfd[i] = nameOfPollfd[*fdCount-1];
    (*fdCount)--;
}


/***************************************************************************
* Attempt to connect with a different router on port number provided
****************************************************************************/
int establishConnectionToRouter(char *port, struct pollfd *pfds, int *fdCount,
    int *fdSize){
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

    if(strcmp(port, firstOutPortStr)==0){
        firstOutName = namebuf[0];
    }
    if(twoRouters && strcmp(port, secondOutPortStr)==0){
        secondOutName = namebuf[0];
    }
    printf("Connection established: router %c\n", namebuf[0]);
    //set distance to router on immediate outgoing connection = 1
    setDistanceAndNextHop(namebuf[0], 1, namebuf[0]);
    addToPollfds(pfds, sockfd, fdCount, fdSize, namebuf[0]);
    return sockfd;
}


/***************************************************************************
* Sets distance to router that has disconnected to -1, same for all routers
* that used the disconnected router as next_hop
****************************************************************************/
void deleteRouterFromTable(char name){
    setDistanceAndNextHop(name, -1, '-');
    for(int i = 0; i < NUMROUTERS; i++){
        if(routerTable[2][i] == name){
            setDistanceAndNextHop(routerTable[0][i], -1, '-');
        }
    }
}


/***************************************************************************
* Prints the contents of routerTable to stdout
****************************************************************************/
void printRouterTable(){
    printf("***********Router Table:**************\n");
    for(int i = 0; i < NUMROUTERS; i++){
        if(routerTable[1][i] >= 0)
            printf("Router %c: Dist=%d, Next Hop=%c\n",
                routerTable[0][i], routerTable[1][i], routerTable[2][i]);
    }
}


/***************************************************************************
* Creates a message containing Dx(y) for all routers y
****************************************************************************/
void createMessage(char *buf){
    char letter[3], num[4];
    for(int i = 0; i < NUMROUTERS-1; i++){
        sprintf(letter, "%c:", routerTable[0][i]);
        strncat(buf, letter, 2);
        sprintf(num, "%d:", routerTable[1][i]);
        strncat(buf, num, 3);
    }
    strncat(buf, ((char*)&routerTable[0][NUMROUTERS-1]), 1);
    sprintf(num, ":%d", routerTable[1][NUMROUTERS-1]);
    strncat(buf, num, 3);
    //strcat(buf, "\0");
    //buf[MAXBUFLEN-1] = '\0';
}

/***************************************************************************
* Updates the table of distances recieved from other routers
****************************************************************************/
void updateRouterMessagesTable(char senderName, char msg[MAXBUFLEN]){
    char *token, *letter, *number;
    int letIdx, dist;


    token = strtok(msg, separator);
    while(token != NULL){
        letter = token;
        if (letter == NULL){
            printf("Malformed message");
            return;
        }
        letIdx = letter[0] - OFFSET;
        number = strtok(NULL, separator);
        if(number == NULL){
            printf("Malformed message");
            return;
        }
        dist = atoi(number);
        if(dist != -1) dist += 1; //To account for distance to sender
        routerMessages[senderName-OFFSET][letIdx] = dist;

        token = strtok(NULL, separator);

    }
}

/***************************************************************************
* Updates the router table using the values in the routerMessages table
****************************************************************************/
void updateRouterTable(){
    int min;
    char senderName, destName;

    for(int dest = 0; dest < NUMROUTERS; dest++){
        destName = dest + OFFSET;
        min = 100;

        for(int sender = 0; sender < NUMROUTERS; sender++){
            if(routerMessages[sender][dest] > -1 &&
                    routerMessages[sender][dest] < min &&
                    sender != myName[0]-65){
                min = routerMessages[sender][dest];
                senderName = sender + OFFSET;
            }

        }
        if(min == 100){
            min = -1;
            senderName = '-';
        }


        if(myName[0] != destName){
            if(destName != firstOutName && destName != secondOutName)
                setDistanceAndNextHop(destName, min , senderName);
        }

    }
}

/***************************************************************************
* Resets the router message table to default state
****************************************************************************/
void resetRouterMessagesTable(){
    for(int i = 0; i < NUMROUTERS; i++){
        for(int j = 0; j < NUMROUTERS; j ++){
            routerMessages[i][j] = -1;
        }
    }
}

/***************************************************************************
* Main method, runs a loop looking for inbound connections/messages
****************************************************************************/
int main(int argc, char *argv[]) {
    int listenSock, firstOutSock, secondOutSock, newfd, destfd, senderfd;
    char buf[MAXBUFLEN], namebuf[2], *msg = "";
    int fdCount = 0, pollCount, wait;
    time_t curTime, lastPrintedTime;
    int fdSize = NUMROUTERS;
    int nbytes, numbytes;
    socklen_t addrlen;
    struct sockaddr_storage remoteaddr;
    struct pollfd *pollfds = malloc(sizeof *pollfds * fdSize);

    /**********
    * Read in arguments
    ***********/
    if (argc < 4 || argc > 5){
        printf("Incorrect number of arguments.\n");
        printf("Usage: router [name] [myPort] [theirPort] [optionalPort]\n");
        exit(1);
    }

    myName = argv[1];
    if (myName[0] < 'A' || myName[0] > 'Z'){
        printf("Invalid router name.\n");
        exit(1);
    }
    //Set distance to self = 0
    setDistanceAndNextHop(myName[0], 0, myName[0]);

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

    resetRouterMessagesTable();

    /**********
    * Set up and get a listening socket
    ***********/
    listenSock = getListenerSocket();
    if (listenSock == -1) {
        fprintf(stderr, "Failed to establish listener socket\n");
        exit(1);
    }
    // Add the listener to set
    pollfds[0].fd = listenSock;
    pollfds[0].events = POLLIN;
    fdCount = 1;



    /**********
    * Try to connect to port #s provided
    ***********/
    if((firstOutSock = establishConnectionToRouter(firstOutPortStr, pollfds,
            &fdCount, &fdSize)) != -1)
        firstIsConnected = TRUE;


    if(twoRouters)
        if((secondOutSock = establishConnectionToRouter(secondOutPortStr,
            pollfds, &fdCount, &fdSize)) != -1)
            secondIsConnected = TRUE;

    time(&lastPrintedTime);    //Initialize to now to wait full 2 seconds

    for(;;) {
        wait = 1.0 * MILSECINSEC;

        pollCount = poll(pollfds, fdCount, wait);

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
                        if (send(newfd, myName, 1, 0) == -1)
                    		perror("send");

                        //Set distance to new connection = 1
                        setDistanceAndNextHop(namebuf[0], 1, namebuf[0]);
                        addToPollfds(pollfds, newfd, &fdCount, &fdSize,
                            namebuf[0]);

                        printf("New connection from router %c *************\n",
                            namebuf[0]);
                    }
                }
                else {// If not the listener, receiving a transmission
                    nbytes = recv(pollfds[i].fd, buf, MAXBUFLEN, 0);
                    senderfd = pollfds[i].fd;

                    if (nbytes <= 0) {
                        // Got error or connection closed by client
                        if (nbytes == 0) {
                            // Connection closed
                            printf("Router %c on socket %d hung up**********\n",
                                nameOfPollfd[i], senderfd);
                        } else {
                            perror("recv");
                        }

                        //Check if connection is one we manage
                        if(senderfd == firstOutSock){
                            firstIsConnected = FALSE;
                        }
                        else if(twoRouters && senderfd == secondOutSock){
                            secondIsConnected = FALSE;
                        }
                        close(pollfds[i].fd);
                        deleteRouterFromTable(nameOfPollfd[i]);
                        deleteFromPollfds(pollfds, i, &fdCount);


                    } else {
                        // We got some good data from a client
                        buf[nbytes] = '\0';
                        updateRouterMessagesTable(nameOfPollfd[i], buf);
                    }
                }
            }
        }

        time(&curTime);
        if (difftime(curTime, lastPrintedTime) >= 2.0){
            updateRouterTable();
            printRouterTable();
            msg = (char *)malloc(sizeof(char*)*MAXBUFLEN);
            if(!msg){
                perror("malloc msg");
            }
            memset(msg, 0, sizeof(MAXBUFLEN));
            createMessage(msg);

            for(int j = 0; j < fdCount; j++) {
                destfd = pollfds[j].fd;
                // Send to everyone, except the listener
                if (destfd != listenSock) {

                    if (send(destfd, msg, strlen(msg), 0) == -1) {
                        perror("send");
                    }

                }
            }
            memset(msg, 0, (size_t)MAXBUFLEN);
            free(msg);
            msg = NULL;


            if (!firstIsConnected){
                if((firstOutSock = establishConnectionToRouter(firstOutPortStr,
                    pollfds, &fdCount, &fdSize)) != -1)
                    firstIsConnected = TRUE;
            }

            if (twoRouters && !secondIsConnected){
                if((secondOutSock= establishConnectionToRouter(secondOutPortStr,
                        pollfds, &fdCount, &fdSize)) != -1)
                    secondIsConnected = TRUE;
            }
            resetRouterMessagesTable();
            time(&lastPrintedTime);
        }
    }
}
