#include <stdio.h>

#define buflen 2000
#define MAXBUFLEN 240 /* max number of bytes per message */

int main (int argc, char *argv[]) {
    char *buffer;
    int ret, run = 1;

    buffer = (char *)Malloc(sizeof(char*)*MAXBUFLEN);
    if(!buffer)
    {
        perror("Malloc");
    }
    memset(buffer, 0, sizeof(MAXBUFLEN));

    while(run){
        /* read a line */
        ret = read(0, buffer, MAXBUFLEN);
        //select(STDIN+1, &readfds, NULL, NULL, &tv);
        if (ret > 0) /* read success */
        {
            printf("threads created\n");
        }

        /* Was the quit command entered? */
        if (strcmp(buffer, "quit\n") == 0)
        {
            running = 0;
        }

        /* reset read buffer */
        Free(buffer);
        buffer = NULL;
        buffer = (char *)Malloc(sizeof(char*)*MAXBUFLEN);
        if(!buffer)
        {
            perror("(char *)Malloc");
        }
        memset(buffer, 0, (size_t)MAXBUFLEN);
    }

}
