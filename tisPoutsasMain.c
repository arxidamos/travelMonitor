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
    // Install signal handlers
    handleSignals();

    // Scan command line arguments
    // Child's writing pipe is parent's reading pipe
    char* pipeWrite = argv[1];
    // Child's reading pipe is parent's writing pipe
    char* pipeRead = argv[2];
    char* dir_path = malloc(strlen(argv[3]) + 1);
    strcpy(dir_path, argv[3]);

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
   
    // Structure to store Monitor's directory info
    MonitorDir* monitorDir = NULL;
    // Structures to store records data
    BloomFilter* bloomsHead = NULL;
    State* stateHead = NULL;
    // State* state;
    Record* recordsHead = NULL;
    // Record* record;
    SkipList* skipVaccHead = NULL;
    SkipList* skipNonVaccHead = NULL;
    // Seed the time generator
    srand(time(NULL));
    int accepted = 0;
    int rejected = 0;

   // Initialize bufSize and bloomSize
    int bufSize = 10;
    int bloomSize = 0;

    // Get 1st message for bufSize
    Message* firstMessage = malloc(sizeof(Message));
    getMessage(firstMessage, incfd, bufSize);
    analyseMessage(&monitorDir, firstMessage, outfd, &bufSize, &bloomSize, dir_path, &bloomsHead, &stateHead, &recordsHead, &skipVaccHead, &skipNonVaccHead, &accepted, &rejected);
 
    // Get 2nd message for bloomSize
    getMessage(firstMessage, incfd, bufSize);
    analyseMessage(&monitorDir, firstMessage, outfd, &bufSize, &bloomSize, dir_path, &bloomsHead, &stateHead, &recordsHead, &skipVaccHead, &skipNonVaccHead, &accepted, &rejected);

    free(firstMessage->code);
    free(firstMessage->body);
    free(firstMessage);

    while (1) {
        // Keep checking signal flags
        checkSignalFlags(&monitorDir, &accepted, &rejected);

        // Get incoming messages
        Message* incMessage = malloc(sizeof(Message));
        // getMessage(incMessage, incfd, bufSize);
        if (getMessage(incMessage, incfd, bufSize) == -1) {
            continue;
        }

        // printf("Message IN CHILD received: %s ", incMessage->body);
        // printf("Code IN CHILD received: %c\n", incMessage->code[0]);
        
        // Decode incoming messages
        analyseMessage(&monitorDir, incMessage, outfd, &bufSize, &bloomSize, dir_path, &bloomsHead, &stateHead, &recordsHead, &skipVaccHead, &skipNonVaccHead, &accepted, &rejected);
        
        
        // printStateList(stateHead);
        // printRecordsList(recordsHead);
        // printBloomsList(bloomsHead);
        // printSkipLists(skipVaccHead);
        // printf("---------------------\n");
        // printSkipLists(skipNonVaccHead);

        // Signal handler for SIGINT and SIGQUIT
        // checkSigQuit(&stateHead, &recordsHead, &bloomsHead, &skipVaccHead, &skipNonVaccHead, monitorDir, dir_path);

    }
}