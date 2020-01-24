#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAXBUFLEN 100 /* max number of bytes per message */
#define MAXREMLEN 83 /* max size for key + value + space*/

static const int portLowLimit = 30000; /*Minimum port specified by assignment*/
static const int portUprLimit = 40000; /*Maximum port specified by assignment*/
const char delim[2] = " ";

int theirPort;
char theirPortSt[6];
char* hostName;





void readCommand(char * buffer){
    char *token, *command, *key, *value;
    int t;
    /* Was the quit command entered? */

    if(strcmp(buffer, "getall") == 0){
        printf("command: getall\n");
    }
    else{
        token = strtok(buffer, delim);
        if (token != NULL) {
            command = token;
            //TODO:Factor out key check if there is time
            if(strcmp(command, "add") == 0){
                token = strtok(NULL, delim);
                if(token != NULL){
                    key = token;
                    t = strcspn(key, "\0");
                    token = strtok(NULL, delim);
                    if(t > 40){
                        printf("%s is longer than 40 characters\n", key);
                    }
                    else if(token != NULL){
                        value = token;
                        t = strcspn(value, "\0");
                        if(t > 40){
                            printf("%s is longer than 40 characters\n", value);
                        }
                        else{
                            if(strtok(NULL, delim) != NULL){
                                printf("Too many arguments\n");
                            }
                            else{
                                //TODO: Compose message and send it
                            }
                        }
                    }
                    else{
                        printf("Cannot add without a value\n");
                    }
                }
                else{
                    printf("Cannot add without a key\n");
                }
            }
            else if(strcmp(command, "getvalue") == 0){
                token = strtok(NULL, delim);
                if(token != NULL){
                    key = token;
                    t = strcspn(key, "\0");
                    if(t > 40){
                        printf("%s is longer than 40 characters\n", key);
                    }
                    else{
                        if(strtok(NULL, delim) != NULL){
                            printf("Too many arguments\n");
                        }
                        else{
                            //TODO: Compose message and send it
                        }
                    }
                }
                else{
                    printf("Cannot getvalue without a key\n");
                }
            }
            else if(strcmp(command, "remove") == 0){
                token = strtok(NULL, delim);
                if(token != NULL){
                    key = token;
                    t = strcspn(key, "\0");
                    if(t > 40){
                        printf("%s is longer than 40 characters\n", key);
                    }
                    else{
                        if(strtok(NULL, delim) != NULL){
                            printf("Too many arguments\n");
                        }
                        else{
                            //TODO: Compose message and send it
                        }
                    }
                }
                else{
                    printf("Cannot remove without a key\n");
                }
            }
            else{
                printf("Command not recognized\n");
            }
        }
    }
}







int main (int argc, char *argv[]) {
    char *buffer;
    int ret, rv, run = 1;
    struct addrinfo hints, *servinfo;

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

	if ((rv = getaddrinfo(argv[1], theirPortSt, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

    // loop through all the results and connect to the first we can
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

		break;
	}

    if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);

    freeaddrinfo(servinfo); 

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

    buffer = (char *)malloc(sizeof(char*)*MAXBUFLEN);
    if(!buffer){
        perror("malloc buffer");
    }
    memset(buffer, 0, sizeof(MAXBUFLEN));

    while(run){
        /* read a line */
        ret = read(0, buffer, MAXBUFLEN);
        if (ret > 0){ /* read success */
            buffer[strcspn(buffer, "\r\n")] = 0;    /*Remove \n from end*/
            if (strcmp(buffer, "quit") == 0){
                run = 0;
            }
            else{
                readCommand(buffer);
            }
        }

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

    /*cleanup*/
    memset(buffer, 0, (size_t)MAXBUFLEN);
    free(buffer);
}
