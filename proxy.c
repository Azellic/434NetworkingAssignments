/*
* Alexa Armitage ama043 11158883
* CMPT 434
* Assignment 1 Part A.2
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXBUFLEN 101 /* max number of bytes per message */
#define PORT "34565" /* port clients will connect to */
#define BACKLOG 5

static const int portLowLimit = 30000; /*Minimum port specified by assignment*/
static const int portUprLimit = 40000; /*Maximum port specified by assignment*/
const char delim[2] = " ";

int theirPort;
char theirPortSt[6];
char* hostName;

int main(int argc, char *argv[]) {
    char *buffer, *t;/*, *message;*/
    struct addrinfo hints, *servinfo, *p;
    int servfd; /*server*/
    int sockfd, clntfd; /*client*/
    int rv, yes=1, numbytes, c;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN], rbuf[MAXBUFLEN];

    buffer = (char *)malloc(sizeof(char*)*2*MAXBUFLEN);
    if(!buffer){
        perror("malloc buffer");
    }
    memset(buffer, 0, sizeof(MAXBUFLEN));

    /*Perform argument checks*/
    if (argc != 3){
        printf("Incorrect number of arguments.\n");
        exit(1);
    }
    hostName = argv[1];
    theirPort = atoi(argv[2]);
    if (theirPort > portUprLimit || theirPort < portLowLimit){
        printf("Remote port %d out of range.\n", theirPort);
        exit(1);
    }
    printf("Arguments: %s %d\n",  hostName, theirPort);
    strcpy(theirPortSt, argv[2]);


    /*Setup to act as client to the server*/
    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostName, theirPortSt, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

    for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((servfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(servfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(servfd);
			continue;
		}
        printf("Connection found\n");
		break;
	}

    if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

    freeaddrinfo(servinfo);
    printf("Connected to server\n");

    /*setup to act as server to the client(s)*/
    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

    for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

    freeaddrinfo(servinfo);

    if(p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

    if(listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
    sin_size = sizeof their_addr;
    clntfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	if (clntfd == -1) {
		perror("accept");
	}
    printf("Connected to client\n");

    while(1){
        numbytes = recv(clntfd, buf, MAXBUFLEN-1, 0);

        if(numbytes == -1) {
            perror("recv");
            exit(1);
        }
        if(numbytes == 0){
            printf("Client has disconnected\n");
            close(clntfd);
            exit(0);
        }
        printf("Client->Server: %s\n", buf);

        if (send(servfd, buf, numbytes, 0) == -1)
            perror("send");

        numbytes = recv(servfd, rbuf, MAXBUFLEN-1, 0);

        if(numbytes == -1) {
            perror("recv");
            exit(1);
        }
        if(numbytes == 0){
            printf("Client has disconnected\n");
            close(clntfd);
            close(servfd);
            close(sockfd);
            exit(0);
        }
        rbuf[numbytes] = '\0';

        c = 0;
        for (t = rbuf; *t != '\0'; t++){
            buffer[c] = t[0];
            c++;
            /*printf("%c\n", t[0]);*/
            if(t[0] == 'c' || t[0] == 'm' || t[0] == 'p' || t[0] == 't'){
                /*printf("%c\n", t[0]);*/
                buffer[c] = t[0];
                c++;
            }
        }

        printf("Server->Client: %s\n", buffer);
        if (send(clntfd, buffer, c, 0) == -1)
            perror("send");

        /* reset read buffer variables */
        memset(buffer, 0, (size_t)MAXBUFLEN);
        free(buffer);
        buffer = NULL;
        buffer = (char *)malloc(sizeof(char*)*MAXBUFLEN);
        if(!buffer){
            perror("(char *)malloc buffer");
        }
        memset(buffer, 0, (size_t)MAXBUFLEN);
    }

    exit(0);
}
