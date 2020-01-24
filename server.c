#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAXBUFLEN 100 /* max number of bytes per message */

static const int portLowLimit = 30000; /*Minimum port specified by assignment*/
static const int portUprLimit = 40000; /*Maximum port specified by assignment*/

int theirPort;
char theirPortSt[6];
char* hostName;

int main (int argc, char *argv[]) {
    struct addrinfo hints, *servinfo;
    int rv;

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
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, theirPortSt, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
}
