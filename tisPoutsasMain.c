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
#include "structs.h"
#include "functions.h"

int main(int argc, char* argv[]) {

    // for (int i=0; i<argc; i++) {
    //     printf("%s ", argv[i]);
    // }
    // printf("\n");

    int bufSize = atoi(argv[1]);
    // Child's writing pipe is parent's reading pipe
    char* pipeWrite = argv[2];
    // Child's reading pipe is parent's writing pipe
    char* pipeRead = argv[3];
    printf("To GAMW WRITING PIPE TOU GAMW PARENT einai gia to paidi TO READING PIPE: %s\n", pipeRead);
    char* arithmos = argv[4];
    int x = atoi(arithmos);
    char msg[33];
    sprintf(msg, "gamw tin malakia sou tin orthia%d", x);
    

    // Open writing pipe
    int outfd;
    if ( (outfd = open(pipeWrite, O_WRONLY)) == -1 ) {
        perror("Problem opening writing pipe");
        exit(1);
    }
    // Open reading pipe
    int incfd;
    if ( (incfd = open(pipeRead, O_RDONLY)) == -1 ) {
        perror("Problem opening reading pipe");
        exit(1);
    }    

    // Send the message
    
    // sendBytes('P', msg, outfd, bufSize);



    while (1) {
        // Read the message
        printf("IN THE CHILD LOOP (from child %d)\n", getpid());
        Message* incMessage = malloc(sizeof(Message));
        
        getMessage(incMessage, incfd, bufSize);

        printf("Message IN CHILD received: %s\n", incMessage->body);
        printf("Code IN CHILD received: %c\n", incMessage->code[0]);

        if (incMessage->code[0] == 'C') {
            printf("gamieste oi pantes, bgainw\n");
            sendBytes('R', msg, outfd, bufSize);
            free(incMessage->code);
            free(incMessage->body);
            free(incMessage);        
            exit(0);
        }

        free(incMessage->code);
        free(incMessage->body);
        free(incMessage);
    }

    // printf("Child terminating\n");
    // if (close(outfd) == -1) {
    //     perror("Error closing named pipe");
    // }
    // exit(0);
}