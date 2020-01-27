/*
* Alexa Armitage ama043 11158883
* CMPT 434
* Assignment 1 Part A.1
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
#define MAXREMLEN 83 /* max size for key + value + space*/

static const int portLowLimit = 30000; /*Minimum port specified by assignment*/
static const int portUprLimit = 40000; /*Maximum port specified by assignment*/
const char delim[2] = " ";

int theirPort;
char theirPortSt[6];
char* hostName;


int checkCommand(char * buffer){
    char *token, *command, *key, *value;
    int t;

    token = strtok(buffer, delim);
    if (token != NULL) {
        command = token;
        if(strcmp(command, "add") == 0){
            token = strtok(NULL, delim);
            if(token != NULL){
                key = token;
                t = strcspn(key, "\0");
                token = strtok(NULL, delim);
                if(t > 40){
                    printf("%s is longer than 40 characters\n", key);
                    return 1;
                }
                else if(token != NULL){
                    value = token;
                    t = strcspn(value, "\0");
                    if(t > 40){
                        printf("%s is longer than 40 characters\n", value);
                        return 1;
                    }

                    if(strtok(NULL, delim) != NULL){
                        printf("Too many arguments\n");
                        return 1;
                    }

                }
                else{
                    printf("Cannot add without a value\n");
                    return 1;
                }
            }
            else{
                printf("Cannot add without a key\n");
                return 1;
            }
        }
        else if(strcmp(command, "getvalue") == 0){
            token = strtok(NULL, delim);
            if(token != NULL){
                key = token;
                t = strcspn(key, "\0");
                if(t > 40){
                    printf("%s is longer than 40 characters\n", key);
                    return 1;
                }

                if(strtok(NULL, delim) != NULL){
                    printf("Too many arguments\n");
                    return 1;
                }
            }
            else{
                printf("Cannot getvalue without a key\n");
                return 1;
            }
        }
        else if(strcmp(command, "remove") == 0){
            token = strtok(NULL, delim);
            if(token != NULL){
                key = token;
                t = strcspn(key, "\0");
                if(t > 40){
                    printf("%s is longer than 40 characters\n", key);
                    return 1;
                }

                if(strtok(NULL, delim) != NULL){
                    printf("Too many arguments\n");
                    return 1;
                }

            }
            else{
                printf("Cannot remove without a key\n");
                return 1;
            }
        }
        else{
            printf("Command not recognized\n");
            return 1;
        }
    }
    return 0;
}







int main (int argc, char *argv[]) {
    char *buffer, *message;
    int ret, rv, msglen, numbytes, sz, i, run = 1;
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    char recvbuf[2*MAXBUFLEN];


    buffer = (char *)malloc(sizeof(char*)*MAXBUFLEN);
    if(!buffer){
        perror("malloc buffer");
    }
    memset(buffer, 0, sizeof(MAXBUFLEN));

    message = (char *)malloc(sizeof(char*)*MAXBUFLEN);
    if(!message){
        perror("malloc message");
    }
    memset(message, 0, sizeof(MAXBUFLEN));


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

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostName, theirPortSt, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
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

    while(run){
        /* read a line */
        ret = read(0, buffer, MAXBUFLEN);
        if (ret > 0){ /* read success */
            msglen = strcspn(buffer, "\r\n");
            if(msglen == 0){
                continue;   /*Ignore blank lines*/
            }
            buffer[strcspn(buffer, "\r\n")] = 0;    /*Remove \n from end*/
            /* Was the quit command entered? */
            if (strcmp(buffer, "quit") == 0){
                run = 0;
            }
            /* The getall command will need multiple recv, check for it here*/
            else if(strcmp(buffer, "getall") == 0){
                strcpy(message, buffer); /*Make a copy of the message to pass */

                if (send(sockfd, message, msglen, 0) == -1)
            		perror("send");

                numbytes = recv(sockfd, recvbuf, MAXBUFLEN-1, 0);

                if(numbytes == -1) {
                    perror("recv");
                    exit(1);
                }
                recvbuf[numbytes] = '\0';
                printf("recieved: %s\n", recvbuf);
                if (send(sockfd, "good", 4, 0) == -1)
            		perror("send");
                if(strcmp(recvbuf, "0") == 0){
                    printf("No key-value pairs\n");
                }
                else{
                    sz = atoi(recvbuf);
                    if(sz > 0 && sz <=20){
                        printf("server: All Key-Value Pairs:\n");
                        for(i = 0; i< sz; i++){
                            numbytes = recv(sockfd, recvbuf, (2*MAXBUFLEN)-1, 0);

                            if(numbytes == -1) {
                                perror("recv");
                                exit(1);
                            }
                            if (send(sockfd, "good", 4, 0) == -1)
                        		perror("send");
                            recvbuf[numbytes] = '\0';
                            printf("%s\n", recvbuf);
                        }
                    }
                    /*TODO loop recieve*/
                }

            }
            else{
                strcpy(message, buffer); /*Make a copy of the message to pass */
                if(checkCommand(buffer) == 0){
                    if (send(sockfd, message, msglen, 0) == -1)
                		perror("send");

                    numbytes = recv(sockfd, recvbuf, (2*MAXBUFLEN)-1, 0);

                    if(numbytes == -1) {
                        perror("recv");
                        exit(1);
                    }
                    recvbuf[numbytes] = '\0';
                    printf("server: %s\n", recvbuf);
                }
            }
        }

        /* reset read buffer variable */
        memset(buffer, 0, (size_t)MAXBUFLEN);
        free(buffer);
        buffer = NULL;
        buffer = (char *)malloc(sizeof(char*)*MAXBUFLEN);
        if(!buffer){
            perror("(char *)malloc buffer");
        }
        memset(buffer, 0, (size_t)MAXBUFLEN);

        /* reset message variable */
        memset(message, 0, (size_t)MAXBUFLEN);
        free(message);
        message = NULL;
        message = (char *)malloc(sizeof(char*)*MAXBUFLEN);
        if(!message){
            perror("(char *)malloc message");
        }
        memset(message, 0, (size_t)MAXBUFLEN);
    }

    /*cleanup*/
    memset(buffer, 0, (size_t)MAXBUFLEN);
    free(buffer);
    memset(message, 0, (size_t)MAXBUFLEN);
    free(message);

    return 0;
}
