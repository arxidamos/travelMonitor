#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
# define MAX_DIGITS 10


int main(int argc, char* argv[]) {

    printf("I m in the child\n");
    // Writing fd for childs (Reading for parent)

    // Get the pipe from args
    // char* pipeWrite = malloc(strlen(argv[1]) + 1);
    // strcpy(pipeWrite, argv[1]);
    
    for (int i=0; i<argc; i++) {
        printf("%s\t", argv[i]);
    }
    printf("\n");
    int bufSize = atoi(argv[1]);
    // Child's writing pipe is parent's reading pipe
    char* pipeWrite = argv[2];

     char* arithmos = argv[3];
    int x = atoi(arithmos);
    char msg[33];
    sprintf(msg, "gamw tin malakia sou tin orthia%d", x);
    int msglen = 32;
    // char* msg = "gamw tin malakia sou tin orthia bastardo";
    // int msglen = strlen(msg);
    

    
    char buf[bufSize];
    memset(buf, 0, bufSize);

    // for (int i=0; i<strlen(pipeWrite); i++) {

    // Open the pipe
    int out_fd;
    if ( (out_fd = open(pipeWrite, O_RDWR | O_NONBLOCK)) == -1 ) {
        perror("Problem opening pipe");
    }


    // header (length of message and leading zeros) + actual message + \0
    char* fullMessage = malloc( sizeof(char)*(MAX_DIGITS + msglen + 1) );
    snprintf(fullMessage, MAX_DIGITS+1, "%0*d", MAX_DIGITS, msglen);
    snprintf(fullMessage+MAX_DIGITS, msglen+1, "%s", msg);
    fullMessage[MAX_DIGITS+msglen+1] = '\0';
    printf("Sending the full Message: %s\n", fullMessage);

    // Send the message
    int sentTotal = 0;
    while ( sentTotal < strlen(fullMessage)) {

        // If bytes left to send can fit in pipe, send them all. Else send only bufSize bytes (which fit)
        int left = strlen(fullMessage) - sentTotal;
        int bytesToWrite = left < bufSize ? left : bufSize;

        strncpy(buf, fullMessage + sentTotal, bytesToWrite);
        int sent;
        printf("Sending: ");
        for (int k=0; k<bytesToWrite; k++) {
            printf("%c", buf[k]);
        }
        printf("\n");
        
        if ( (sent = write(out_fd, buf, bytesToWrite)) == -1) {
            perror("Error with writing message");
            exit(1);
        }
        sentTotal += sent;
    }
 
    free(fullMessage);
    printf("Child terminating\n");
    if (close(out_fd) == -1) {
        perror("Error closing named pipe");
    }

    exit(0);

}