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
#define PORT "32346" /* port clients will connect to */
#define BACKLOG 5

const char delim[2] = " ";
char *keys[20];
char *values[20];
int kvSize = 0;
struct sockaddr_storage their_addr;
socklen_t addr_len;





char* removeCommand(char *key){
    int i;

    if(kvSize == 0){
        return "No keys are stored";
    }
    for(i = 0; i < 20; i++){
        if(keys[i]!=NULL){
            if(strcmp(keys[i], key) == 0){
                memset(keys[i], 0, sizeof(MAXBUFLEN));
                free(keys[i]);
                keys[i] = NULL;
                memset(values[i], 0, sizeof(MAXBUFLEN));
                free(values[i]);
                values[i] = NULL;
                kvSize--;
                return "remove successful";
            }
        }
    }
    return "remove failed, key unfound";
}





char* getValueCommand(char *key){
    int i;

    for(i = 0; i < 20; i++){
        if(keys[i]!=NULL){
            if(strcmp(keys[i], key) == 0){
                return values[i];
            }
        }
    }
    return "Key not found";
}




char* addCommand(char *key, char *val){
    int i;

    if(kvSize >= 20){
        return "No space to add key value pairs";
    }

    for(i = 0; i < 20; i++){
        if(keys[i]!=NULL){
            if(strcmp(keys[i], key) == 0){
                return "Key already in use";
            }
        }
    }

    for(i = 0; i < 20; i++){
        if(keys[i]==NULL){
            keys[i] = (char *)malloc(sizeof(char*)*MAXBUFLEN);
            if(!keys[i]){
                perror("malloc message");
            }
            memset(keys[i], 0, sizeof(MAXBUFLEN));
            strcpy(keys[i], key);

            values[i] = (char *)malloc(sizeof(char*)*MAXBUFLEN);
            if(!values[i]){
                perror("malloc message");
            }
            memset(values[i], 0, sizeof(MAXBUFLEN));
            strcpy(values[i], val);
            kvSize++;
            return "add successful";
        }
    }
    return "add failed";
}







void sendKeyValue(int sockfd, int index, struct addrinfo *p){
    char *msg;
    char buf[MAXBUFLEN];
    int nb;

    msg = (char *)malloc(sizeof(char*)*MAXBUFLEN);
    if(!msg){
        perror("malloc msg");
    }
    memset(msg, 0, sizeof(MAXBUFLEN));
    /*TODO compose message*/
    sprintf(msg, "%s - %s", keys[index], values[index]);

    if (sendto(sockfd, msg, strcspn(msg, "\0"), 0,
        (struct sockaddr *)&their_addr, addr_len)  == -1){
        perror("sendto");
    }


    /*Wait for some response to properly page delivery*/
    nb = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len);
    if(nb == -1) {
        perror("recvfrom");
    }


    memset(msg, 0, (size_t)MAXBUFLEN);
    free(msg);
    msg = NULL;
}






char* readCommand(char buffer[100]){
    char *token, *command, *key, *value;
    int t;
    char *message;

    message = (char *)malloc(sizeof(char*)*MAXBUFLEN);
    if(!message){
        perror("malloc message");
    }
    memset(message, 0, sizeof(MAXBUFLEN));

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
                    message = "Key is longer than 40 characters";
                }
                else if(token != NULL){
                    value = token;
                    t = strcspn(value, "\0");
                    if(t > 40){
                        message = "Value is longer than 40 characters";
                    }
                    else{
                        if(strtok(NULL, delim) != NULL){
                            message = "Too many arguments";
                        }
                        else{
                            message = addCommand(key, value);
                        }
                    }
                }
                else{
                    message = "Cannot add without a value";
                }
            }
            else{
                message = "Cannot add without a key";
            }
        }
        else if(strcmp(command, "getvalue") == 0){
            token = strtok(NULL, delim);
            if(token != NULL){
                key = token;
                t = strcspn(key, "\0");
                if(t > 40){
                    message = "Key is longer than 40 characters";
                }
                else{
                    if(strtok(NULL, delim) != NULL){
                        message = "Too many arguments";
                    }
                    else{
                        message = getValueCommand(key);
                    }
                }
            }
            else{
                message = "Cannot getvalue without a key";
            }
        }
        else if(strcmp(command, "remove") == 0){
            token = strtok(NULL, delim);
            if(token != NULL){
                key = token;
                t = strcspn(key, "\0");
                if(t > 40){
                    message = "key is longer than 40 characters";
                }
                else{
                    if(strtok(NULL, delim) != NULL){
                        message = "Too many arguments";
                    }
                    else{
                        message = removeCommand(key);
                    }
                }
            }
            else{
                message = "Cannot remove without a key";
            }
        }
        else{
            message = "Command not recognized";
        }
    }

    return message;
}




int main(int argc, char *argv[]) {
    struct addrinfo hints, *servinfo, *p;
    int rv, numbytes, i;
    int sockfd;
    char buf[MAXBUFLEN];
    char *message, sz[3];

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("UPDserver: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("UPDserver: bind");
			continue;
		}

		break;
	}


    if (p == NULL) {
		fprintf(stderr, "UPDserver: failed to bind socket\n");
		return 2;
	}

    freeaddrinfo(servinfo);
    addr_len = sizeof their_addr;



    /*Recieve commands in loop and respond*/
    while(1){
        numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
            (struct sockaddr *)&their_addr, &addr_len);

        if(numbytes == -1) {
            perror("recvfrom");
            exit(1);
        }
        if(numbytes == 0){
            printf("Client has disconnected\n");
            close(sockfd);
            exit(0);
        }
        
        buf[strcspn(buf, "\r\n")] = 0;    /*Remove \n from end*/
        buf[numbytes] = '\0';
        printf("recieved: %s\n", buf);
        if(strcmp(buf, "getall") == 0){
            sprintf(sz, "%d", kvSize);
            if (sendto(sockfd, sz, strlen(sz), 0,
                (struct sockaddr *)&their_addr, addr_len) == -1){
                perror("sendto");
            }
            else{
                numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                    (struct sockaddr *)&their_addr, &addr_len);

                if(numbytes == -1) {
                    perror("recvfrom");
                    exit(1);
                }
                for(i = 0; i < 20; i++){
                    if(keys[i] != NULL){
                        sendKeyValue(sockfd, i, p);
                    }
                }
            }
        }
        else{
            message = readCommand(buf);
            if (sendto(sockfd, message, strcspn(message, "\0"), 0,
                (struct sockaddr *)&their_addr, addr_len)== -1){

                perror("sendto");
            }
        }


    }
    memset(message, 0, (size_t)MAXBUFLEN);
    free(message);
    return 0;
}
