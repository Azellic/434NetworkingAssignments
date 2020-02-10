/*
* Alexa Armitage ama043 11158883
* CMPT 434
* Assignment 2 Part A.1
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
#include <sys/poll.h>

#define MAXBUFLEN 112 /* max number of bytes per message */
#define BACKLOG 5
#define MAXSZ 2147483647 /*Maximum int, also max sequence number*/

static const int portLowLimit = 30000; /*Minimum port specified by assignment*/
static const int portUprLimit = 40000; /*Maximum port specified by assignment*/

const char delim[2] = ":";
struct sockaddr_storage their_addr;
socklen_t addr_len;
int myPort, failRate;
char myPortSt[6];
int currSeqNum = -1;    /*Init to -1 to be less than first transmission*/
struct pollfd pfd[2];
int sockfd;

void sendAck(){
    int failRoll;
    char rtrn[15];

    failRoll = (rand() % 100) + 1;
    printf("FailRoll: %d\n", failRoll);
    if(failRoll >= failRate){
        sprintf(rtrn, "%d", currSeqNum);
        if (sendto(sockfd, rtrn, strlen(rtrn), 0,
            (struct sockaddr *)&their_addr, addr_len)== -1){
            perror("sendto");
        }
    }
}

void ackUncorruptedMessage(){
    char conf[MAXBUFLEN];
    int ret;

    /*Read in Y/N to simulate possible corruption, and ack accordingly*/
    ret = read(0, conf, MAXBUFLEN);
    if (ret > 0){
        fflush(0);
        conf[1] = 0;
        if(strcmp(conf, "Y") == 0){
            currSeqNum++;
            sendAck();
        }
    }
}

void respondToMsg(char *message){
    char *token, *msg;
    int rcvdSeqNum;

    token = strtok(message, delim);
    if( token != NULL){
        rcvdSeqNum = atoi(token);
        if(rcvdSeqNum == currSeqNum + 1){
            token = strtok(NULL, delim);
            if(token != NULL){
                msg = token;
                printf("recieved: %s\n", msg);
                printf("Was the message correct? [Y/N]\n");
                ackUncorruptedMessage();
            }
            else{
                printf("Received an improperly formatted message\n");
            }
        }
        else if(rcvdSeqNum == currSeqNum){
            token = strtok(NULL, delim);
            if(token != NULL){
                msg = token;
                printf("Retransmission: %d %s\n", rcvdSeqNum, msg);
                sendAck();
            }
            else{
                printf("Received an improperly formatted message\n");
            }
        }
        else if(rcvdSeqNum > currSeqNum + 1 || rcvdSeqNum < currSeqNum){
            token = strtok(NULL, delim);
            if(token != NULL){
                msg = token;
                printf("Out of order: [%d] %s\n", rcvdSeqNum, msg);
            }
            else{
                printf("Received an improperly formatted message\n");
            }
        }
    }
}

int main(int argc, char *argv[]) {
    struct addrinfo hints, *servinfo, *p;
    int rv, numbytes, run = 1;
    char *message;

    /*Perform argument checks*/
    if (argc != 3){
        printf("Incorrect number of arguments!\n");
        printf("Correct usage ./receiver [port] [failRate]\n");
        exit(1);
    }
    myPort = atoi(argv[1]);
    if (myPort > portUprLimit || myPort < portLowLimit){
        printf("Remote port %d out of range.\n", myPort);
        exit(1);
    }
    strcpy(myPortSt, argv[1]);
    failRate = atoi(argv[2]);
    if(failRate > 100 || failRate < 0){
        printf("Invalid fail rate %d.\n", failRate);
        exit(1);
    }
    printf("Arguments: ./receiver %s %d\n", myPortSt, failRate);

    /*Memory Allocate *********************************************************/
    message = (char *)malloc(sizeof(char*)*MAXBUFLEN);
    if(!message){
        perror("malloc message");
    }
    memset(message, 0, sizeof(MAXBUFLEN));

    /*Setup connection to listen **********************************************/
    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, myPortSt, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("receiver: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("receiver: bind");
			continue;
		}

		break;
	}


    if (p == NULL) {
		fprintf(stderr, "receiver: failed to bind socket\n");
		return 2;
	}
    freeaddrinfo(servinfo);
    addr_len = sizeof their_addr;


    /*Main loop ***************************************************************/
    while(run){
        /*Set up poll variables ***********************************************/


        /*Message recieved over port **************************************/
        numbytes = recvfrom(sockfd, message, MAXBUFLEN-1 , 0,
            (struct sockaddr *)&their_addr, &addr_len);

        if(numbytes == -1) {
            perror("recvfrom");
            close(sockfd);
            memset(message, 0, (size_t)MAXBUFLEN);
            free(message);
            exit(1);
        }
        message[numbytes] = '\0';
        respondToMsg(message);


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

    close(sockfd);
    memset(message, 0, (size_t)MAXBUFLEN);
    free(message);
    return 0;
}
