/*
* Alexa Armitage ama043 11158883
* CMPT 434
* Assignment 2 Part A.2
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
const char delim[2] = ":";

/*Input Variables*/
int receiverPort, senderPort;
char receiverPortSt[6], senderPortSt[6];
char* receiverIP;
int sndWindowSz;
int usrTimeout;

/*Connection variables*/
int receiverfd, senderfd;
struct addrinfo *pRc, *pSd;
socklen_t addr_lenRc, addr_lenSd;
struct sockaddr_storage their_addrRc, their_addrSd;
struct addrinfo hints1, hints2;

/*Message queue and tracking variables*/
char **messageQue;
int msgQueSz, numMsgs = 0;
int seqNum = 0, oldestUnackedSeq = 0, newestSentSeq = 0;
int currSeqAckNum = -1;    /*Init to -1 to be less than first transmission*/

/*Poll variables*/
struct pollfd pfd[2];
time_t lastSentTime, curTime;

void sendAck(){
    char rtrn[15];
    sprintf(rtrn, "%d", currSeqAckNum);
    if (sendto(senderfd, rtrn, strlen(rtrn), 0,
        (struct sockaddr *)&their_addrSd, addr_lenSd)== -1){
        perror("sendto");
    }
}

void sendMessage(int seq){
    int msglen;
    if(messageQue[seq] == NULL){
        printf("Incorrect seq\n");
        return;
    }
    msglen = strcspn(messageQue[seq], "\0");
    if (sendto(receiverfd, messageQue[seq], msglen, 0, pRc->ai_addr,
        pRc->ai_addrlen)== -1){
        perror("sendto");
    }
    newestSentSeq = seq;
    time(&lastSentTime);
}

int isMessageUncorrupted(){
    char conf[MAXBUFLEN];
    int ret;

    printf("Was the message correct? [Y/N]\n");
    /*Read in Y/N to simulate possible corruption, and ack accordingly*/
    ret = read(0, conf, MAXBUFLEN);
    if (ret > 0){
        conf[1] = 0;
        if(strcmp(conf, "Y") == 0){
            return 0;
        }
    }
    return 1;
}

void respondToMsg(char *message){
    char *token, *msg, msgCp[MAXMSGLEN];
    int rcvdSeqNum;

    strcpy(msgCp, message);
    token = strtok(msgCp, delim);
    if( token != NULL){
        rcvdSeqNum = atoi(token);
        if(rcvdSeqNum == currSeqAckNum + 1){
            token = strtok(NULL, delim);
            if(token != NULL){
                msg = token;
                printf("recieved: %s\n", msg);
                if(isMessageUncorrupted() == 0){
                    currSeqAckNum++;
                    sendAck();

                    messageQue[seqNum] = message;
                    numMsgs++;
                    if(numMsgs <= sndWindowSz){
                        sendMessage(seqNum);
                    }
                    seqNum++;
                    if(seqNum >= msgQueSz){
                        seqNum = 0;
                    }
                }
            }
            else{
                printf("Received an improperly formatted message: %s\n",
                    message);
            }
        }
        else if(rcvdSeqNum == currSeqAckNum){
            token = strtok(NULL, delim);
            if(token != NULL){
                msg = token;
                printf("Retransmission: %d %s\n", rcvdSeqNum, msg);
                sendAck();
            }
            else{
                printf("Received an improperly formatted message: %s\n",
                    message);
            }
        }
        else if(rcvdSeqNum > currSeqAckNum + 1 || rcvdSeqNum < currSeqAckNum){
            token = strtok(NULL, delim);
            if(token != NULL){
                msg = token;
                printf("Out of order: [%d] %s\n", rcvdSeqNum, msg);
            }
            else{
                printf("Received an improperly formatted message: %s\n",
                    message);
            }
        }
    }
}

/*Acknowledges acks by removing older messages from queue ********************/
int freeAckedMessages(int ack){
    int i, count = 0;

    if(ack > oldestUnackedSeq){
        for(i = oldestUnackedSeq; i < ack; i ++){
            memset(messageQue[i], 0, (size_t)MAXMSGLEN);
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


int connectToSender(){
    struct addrinfo *servinfo;
    int rv;

    /*Open connection to receive messages**************************************/
    memset(&hints2, 0, sizeof hints2);
	hints2.ai_family = AF_INET;
	hints2.ai_socktype = SOCK_DGRAM;
	hints2.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, senderPortSt, &hints2, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(pSd = servinfo; pSd != NULL; pSd = pSd->ai_next) {
		if ((senderfd = socket(pSd->ai_family, pSd->ai_socktype,
				pSd->ai_protocol)) == -1) {
			perror("receiver: socket");
			continue;
		}

		if (bind(senderfd, pSd->ai_addr, pSd->ai_addrlen) == -1) {
			close(senderfd);
			perror("receiver: bind");
			continue;
		}

		break;
	}


    if (pSd == NULL) {
		fprintf(stderr, "receiver: failed to bind socket\n");
		return 2;
	}
    freeaddrinfo(servinfo);
    addr_lenSd = sizeof their_addrSd;
    return 0;
}

int connectToReceiver(){
    struct addrinfo *servinfo;
    int rv;

    /*Setup connection to send messages********************************/
    memset(&hints1, 0, sizeof hints1);
	hints1.ai_family = AF_INET;
	hints1.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(receiverIP, receiverPortSt, &hints1, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(pRc = servinfo; pRc != NULL; pRc = pRc->ai_next) {
		if ((receiverfd = socket(pRc->ai_family, pRc->ai_socktype,
				pRc->ai_protocol)) == -1) {
			perror("sender: socket");
			continue;
		}
		break;
	}

    if (pRc == NULL) {
		fprintf(stderr, "sender: failed to create socket\n");
		return 2;
	}
    addr_lenRc = sizeof their_addrRc;
    freeaddrinfo(servinfo);
    return 0;
}

void cleanup(){
    int i;
    printf("Cleaning up (this can take a few seconds)\n");
    close(senderfd);
    close(receiverfd);
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

int main(int argc, char *argv[]) {
    char *message, ack[20];
    int numbytes, ackVal, wait, numFreed;

    srand(time(0));

    /*Perform argument checks*/
    /*Perform argument checks *************************************/
    if (argc != 6){
        printf("Incorrect number of arguments.\n");
        printf("Usage: ./forwarder [senderPort] [receiverIP] %s\n",
            "[receiverPort] [sendWindow] [timeout]");
        exit(1);
    }
    senderPort = atoi(argv[1]);
    if (senderPort > portUprLimit || senderPort < portLowLimit){
        printf("Remote port %d out of range.\n", senderPort);
        exit(1);
    }
    strcpy(senderPortSt, argv[1]);

    receiverIP = argv[2];

    receiverPort = atoi(argv[3]);
    if (receiverPort > portUprLimit || receiverPort < portLowLimit){
        printf("Remote port %d out of range.\n", receiverPort);
        exit(1);
    }
    strcpy(receiverPortSt, argv[3]);

    sndWindowSz = atoi(argv[4]);
    if (sndWindowSz < 0){
        printf("sendWindow cannot be negative.\n");
        exit(1);
    }

    usrTimeout = atoi(argv[5]);
    if (usrTimeout < 0){
        printf("Timeout cannot be negative.\n");
        exit(1);
    }

    printf("Arguments: ./forwarder %s %s %s %d %d\n", senderPortSt, receiverIP,
        receiverPortSt, sndWindowSz, usrTimeout);

    if(connectToSender()!= 0){
        exit(1);
    }

    if(connectToReceiver()!= 0){
        exit(1);
    }


    /*Set up poll variables */
    pfd[0].fd = receiverfd;
    pfd[0].events = POLLIN;

    pfd[1].fd = senderfd;
    pfd[1].events = POLLIN;

    /*Change this to change maximum number of messages ************************/
    msgQueSz = MAXSZ;

    /*Memory Allocate for message buffers *************************/
    message = (char *)malloc(sizeof(char*)*MAXMSGLEN);
    if(!message){
        perror("malloc message");
    }
    memset(message, 0, sizeof(MAXMSGLEN));

    messageQue = (char**)malloc(msgQueSz * sizeof(char*));
    if(!messageQue)
        perror("malloc messageQue");
    memset(messageQue, 0, msgQueSz);

    while(1){
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
            /*Received input from receiver (ack)*/
            /*Receive from server*/
            numbytes = recvfrom(receiverfd, ack, 19, 0,
                (struct sockaddr *)&their_addrRc, &addr_lenRc);

            if(numbytes == -1) {
                perror("recv");
                exit(1);
            }
            ack[numbytes] = '\0';
            ackVal = atoi(ack);
            numFreed = freeAckedMessages(ackVal);

            sendQueuedMessages(numFreed);
        }
        else if(pfd[1].revents & POLLIN){
            /*Received input from sender (message)*/
            numbytes = recvfrom(senderfd, message, MAXMSGLEN-1 , 0,
                (struct sockaddr *)&their_addrSd, &addr_lenSd);

            if(numbytes == -1) {
                perror("recvfrom");
                cleanup();
                memset(message, 0, (size_t)MAXMSGLEN);
                free(message);
                exit(1);
            }
            message[numbytes] = '\0';
            if(numMsgs == msgQueSz){
                printf("Message queue full\n");
            }
            else{
                respondToMsg(message);  /*Don't respond unless there is space*/
            }
        }
        else{
            resendUnackedMessages();
        }


        /* reset read message variables */
        message = NULL;
        message = (char *)malloc(sizeof(char*)*MAXMSGLEN);
        if(!message){
            perror("(char *)malloc message");
        }
        memset(message, 0, (size_t)MAXMSGLEN);
    }

    exit(0);
}
