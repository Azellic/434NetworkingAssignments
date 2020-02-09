/*
* Alexa Armitage ama043 11158883
* CMPT 434
* Assignment 1 Part A.3
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
#define PORT "35790" /* port clients will connect to */
#define BACKLOG 5

static const int portLowLimit = 30000; /*Minimum port specified by assignment*/
static const int portUprLimit = 40000; /*Maximum port specified by assignment*/
const char delim[2] = " ";

int theirPort;
char theirPortSt[6];
char* hostName;

struct sockaddr_storage their_addr2;
socklen_t addr_len;

int main(int argc, char *argv[]) {
    char *buffer, *t;/*, *message;*/
    struct addrinfo hints, hints2, *servinfo, *servinfo2, *p, *p1;
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


    /*Setup connection from proxy to server*/
    memset(&hints2, 0, sizeof hints2);
	hints2.ai_family = AF_UNSPEC;
	hints2.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(hostName, theirPortSt, &hints2, &servinfo2)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p1 = servinfo2; p1 != NULL; p1 = p1->ai_next) {
		if ((servfd = socket(p1->ai_family, p1->ai_socktype,
				p1->ai_protocol)) == -1) {
			perror("UDPproxy: socket");
			continue;
		}

		break;
	}

    if (p1 == NULL) {
		fprintf(stderr, "UDPproxy: failed to create socket\n");
		return 2;
	}
    addr_len = sizeof their_addr;
    printf("Connected to server\n");




    /*open connection from proxy to client(s)*/
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


    /*Continuously pass messages from client to server and back*/
    while(1){
        /*recieve from client*/
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
        buf[numbytes] = '\0';
        printf("Client->Server: %s\n", buf);

        /*Send to server*/
        if (sendto(servfd, buf, numbytes, 0, p1->ai_addr,
            p1->ai_addrlen)== -1){

            perror("sendto");
        }

        /*Receive from server*/
        numbytes = recvfrom(servfd, rbuf, MAXBUFLEN-1, 0,
            (struct sockaddr *)&their_addr, &addr_len);

        if(numbytes == -1) {
            perror("recv");
            exit(1);
        }
        if(numbytes == 0){
            printf("Client has disconnected\n");
            close(clntfd);
            freeaddrinfo(servinfo2);
            exit(0);
        }
        rbuf[numbytes] = '\0';

        c = 0;
        for (t = rbuf; *t != '\0'; t++){
            buffer[c] = t[0];
            c++;
            if(t[0] == 'c' || t[0] == 'm' || t[0] == 'p' || t[0] == 't'){
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
