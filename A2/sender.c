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
#include <time.h>

#define MAXBUFLEN 100 /* max number of bytes per message */
#define HEADERSZ 11 /* max number of characters in header plus 1*/
#define MAXMSGLEN MAXBUFLEN+HEADERSZ
#define MAXSZ 2147483647 /*Maximum int, also max sequence number*/
#define MILSECINSEC 1000

static const int portLowLimit = 30000; /*Minimum port specified by assignment*/
static const int portUprLimit = 40000; /*Maximum port specified by assignment*/
const char delim[2] = " ";

char* ipAddress;
int theirPort;
char theirPortSt[6];
int sndWindowSz, msgQueSz, numMsgs = 0;
int usrTimeout;
socklen_t addr_len;
int seqNum = 0, oldestUnackedSeq = 0, newestSentSeq = 0;
struct pollfd pfd[2];
char **messageQue;
struct addrinfo *p;
time_t lastSentTime, curTime;
int sockfd;

/*Acknowledges acks by removing older messages from queue ********************/
int freeAckedMessages(int ack){
    int i, count = 0;

    if(ack > oldestUnackedSeq){
        for(i = oldestUnackedSeq; i < ack; i ++){
            memset(messageQue[i], 0, (size_t)MAXBUFLEN);
            free(messageQue[i]);
            messageQue[i] = NULL;
            numMsgs--;
            count++;
        }
    }
    free(messageQue[ack]);
    messageQue[ack] = NULL;
    numMsgs--;
    count++;
    oldestUnackedSeq = ack + 1;
    if (oldestUnackedSeq == msgQueSz) {oldestUnackedSeq = 0;}
    return count;
}

void sendMessage(int seq){
    int msglen;
    if(messageQue[seq] == NULL){return;}
    msglen = strcspn(messageQue[seq], "\0");
    if (sendto(sockfd, messageQue[seq], msglen, 0, p->ai_addr,
        p->ai_addrlen)== -1){

        perror("sendto");
    }
    newestSentSeq = seq;
    time(&lastSentTime);
}

void resendUnackedMessages(){
    int i, j;

    for(i = 0; i < sndWindowSz; i++){
        j = oldestUnackedSeq + i;
        if(j >= msgQueSz){
            j = j - msgQueSz;
        }
        if(messageQue[j] != NULL){
            sendMessage(j);
        }
        else{
            break;  /*Have reached an unused part of messageQue, stop*/
        }
    }
}

void sendQueuedMessages(int num){
    int i;

    for(i = 0; i < num; i++){
        if(messageQue[newestSentSeq+1] != NULL){
            sendMessage(newestSentSeq+1);
        }
        else{
            break;
        }
    }
}

void cleanup(){
    int i;
    printf("Cleaning up (this can take a few seconds)\n");
    for(i = 0; i < msgQueSz; i++){
        if(messageQue[i] != NULL){
            memset(messageQue[i], 0, (size_t)MAXBUFLEN);
            free(messageQue[i]);
            messageQue[i] = NULL;
        }
    }

    numMsgs = 0;
    free(messageQue);
}

int main (int argc, char *argv[]) {
    char *buffer, *message, seq[11], ack[20];
    int ret, rv, msglen, run = 1, numbytes, ackVal;
    int wait, numFreed;
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage their_addr;

    /*Perform argument checks *************************************/
    if (argc != 5){
        printf("Incorrect number of arguments.\n");
        printf("Usage: ./sender [hostIP] [port] [sendWindow] [timeout]\n");
        exit(1);
    }
    ipAddress = argv[1];
    theirPort = atoi(argv[2]);
    if (theirPort > portUprLimit || theirPort < portLowLimit){
        printf("Remote port %d out of range.\n", theirPort);
        exit(1);
    }
    strcpy(theirPortSt, argv[2]);

    sndWindowSz = atoi(argv[3]);
    if (sndWindowSz < 0){
        printf("sendWindow cannot be negative.\n");
        exit(1);
    }

    usrTimeout = atoi(argv[4]);
    if (usrTimeout < 0){
        printf("Timeout cannot be negative.\n");
        exit(1);
    }

    printf("Arguments: ./sender %s %s %d %d\n",  ipAddress, theirPortSt,
        sndWindowSz, usrTimeout);

    /*Change this to change maximum number of messages ************************/
    msgQueSz = MAXSZ;

    /*Memory Allocate for message buffers *************************/
    buffer = (char *)malloc(sizeof(char*)*MAXBUFLEN);
    if(!buffer){
        perror("malloc buffer");
    }
    memset(buffer, 0, sizeof(MAXBUFLEN));

    message = (char *)malloc(sizeof(char*)*MAXMSGLEN);
    if(!message){
        perror("malloc message");
    }
    memset(message, 0, sizeof(MAXMSGLEN));

    messageQue = (char**)malloc(msgQueSz * sizeof(char*));
    if(!messageQue)
        perror("malloc messageQue");
    memset(messageQue, 0, msgQueSz);

    /*Setup connection with server ********************************/
    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(ipAddress, theirPortSt, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("sender: socket");
			continue;
		}
		break;
	}

    if (p == NULL) {
		fprintf(stderr, "sender: failed to create socket\n");
		return 2;
	}
    addr_len = sizeof their_addr;
    freeaddrinfo(servinfo);




    /*Main loop ***************************************************************/
    printf("Enter messages to send (100 character limit):\n");
    while(run){
        /*Set up poll variables */
        pfd[0].fd = 0;
        pfd[0].events = POLLIN;

        pfd[1].fd = sockfd;
        pfd[1].events = POLLIN;
        /* poll stdin and socket */

        /*Adjust poll timeout*/
        if(numMsgs == 0){
            wait = -1;
        }
        else{
            time(&curTime);
            wait = (usrTimeout-difftime(lastSentTime, curTime))*MILSECINSEC;
        }

        poll(pfd, 2, wait);

        if(pfd[0].revents & POLLIN){
            ret = read(0, buffer, MAXBUFLEN);
            if (ret > 0){
                msglen = strcspn(buffer, "\r\n");
                if(msglen == 0){
                    continue;   /*Ignore blank lines*/
                }
                buffer[msglen] = 0;   /*Remove \n\r from end*/
                /* Was the quit command entered? */
                if (strcmp(buffer, "quit") == 0){
                    run = 0;
                }
                else if(numMsgs == msgQueSz){
                    printf("Message queue full\n");
                }
                else{
                    /*Build the message*/
                    sprintf(seq, "%d:", seqNum);
                    strcpy(message, seq);
                    strcat(message, buffer);
                    /*Add message to queue*/
                    messageQue[seqNum] = message;
                    numMsgs++;
                    /*If sending window still has space, send now*/
                    if(numMsgs <= sndWindowSz){
                        sendMessage(seqNum);
                    }

                    seqNum++;
                    if(seqNum >= msgQueSz){
                        seqNum = 0;
                    }
                }
            }
        }
        else if(pfd[1].revents & POLLIN){
            /*Receive from server*/
            numbytes = recvfrom(sockfd, ack, 19, 0,
                (struct sockaddr *)&their_addr, &addr_len);

            if(numbytes == -1) {
                perror("recv");
                cleanup();
                exit(1);
            }
            ack[numbytes] = '\0';
            ackVal = atoi(ack);
            numFreed = freeAckedMessages(ackVal);
            sendQueuedMessages(numFreed);
        }
        else{
            resendUnackedMessages();
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
        message = NULL;
        message = (char *)malloc(sizeof(char*)*MAXMSGLEN);
        if(!message){
            perror("(char *)malloc message");
        }
        memset(message, 0, (size_t)MAXMSGLEN);
    }

    /*cleanup*/
    memset(buffer, 0, (size_t)MAXBUFLEN);
    free(buffer);
    memset(message, 0, (size_t)MAXBUFLEN);
    free(message);
    cleanup();

    close(sockfd);

    return 0;
}
