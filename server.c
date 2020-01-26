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
#define PORT "32345" /* port clients will connect to */
#define BACKLOG 5

const char delim[2] = " ";
char *keys[20];
char *values[20];
int kvSize = 0;





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







void sendKeyValue(int sockfd, int index){
    char *msg;
    char buf[MAXBUFLEN];

    msg = (char *)malloc(sizeof(char*)*MAXBUFLEN);
    if(!msg){
        perror("malloc msg");
    }
    memset(msg, 0, sizeof(MAXBUFLEN));
    /*TODO compose message*/
    sprintf(msg, "%s - %s", keys[index], values[index]);

    if (send(sockfd, msg, strcspn(msg, "\0"), 0) == -1)
        perror("send");

    recv(sockfd, buf, MAXBUFLEN-1, 0);





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
    int rv, yes=1, numbytes, i;
    int sockfd, newfd;
    char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char *message, sz[3];

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
    newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	if (newfd == -1) {
		perror("accept");
	}

    /*Recieve commands in loop and respond*/
    while(1){
        numbytes = recv(newfd, buf, MAXBUFLEN-1, 0);

        if(numbytes == -1) {
            perror("recv");
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
            if (send(newfd, sz, strlen(sz), 0) == -1){
                perror("send");
            }
            else{
                numbytes = recv(newfd, buf, MAXBUFLEN-1, 0);

                if(numbytes == -1) {
                    perror("recv");
                    exit(1);
                }
                for(i = 0; i < 20; i++){
                    if(keys[i] != NULL){
                        sendKeyValue(newfd, i);
                    }
                }
            }
        }
        else{
            message = readCommand(buf);
            /*TODO:send message*/
            if (send(newfd, message, strcspn(message, "\0"), 0) == -1){
                perror("send");
            }
        }


    }
    memset(message, 0, (size_t)MAXBUFLEN);
    free(message);
    return 0;
}
