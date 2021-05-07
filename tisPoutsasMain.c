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

    int bufSize = atoi(argv[1]);
    // Child's writing pipe is parent's reading pipe
    char* pipeWrite = argv[2];
    // Child's reading pipe is parent's writing pipe
    char* pipeRead = argv[3];
    // char* dir_path = argv[4];
    char* dir_path = malloc(strlen(argv[4]) + 1);
    strcpy(dir_path, argv[4]);

    int bloomSize = atoi(argv[5]);
    // char* arithmos = argv[4];
    // int x = atoi(arithmos);
    // char msg[33];
    // sprintf(msg, "gamw tin malakia sou tin orthia%d", x);
    

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

    MonitorDir* monitorDir = NULL;

    while (1) {
        // Read the message
        // printf("IN THE CHILD LOOP (from child %d)\n", getpid());
        Message* incMessage = malloc(sizeof(Message));
        
        getMessage(incMessage, incfd, bufSize);

        printf("Message IN CHILD received: %s ", incMessage->body);
        printf("Code IN CHILD received: %c\n", incMessage->code[0]);

        analyseMessage(&monitorDir, incMessage, outfd, bufSize, dir_path);
        
        if (incMessage->code[0] == 'F') {

            // as diavasw ola ta arxeia kai kanoume send ta bloom filters
////////////////////////////////////////////////////////////////////////////////////////////////////////////
            FILE* inputFile;

            // Initialize variables
            size_t textSize;
            char* text = NULL;
            char* token = NULL;
            char* citizenID;
            char* fName;
            char* lName;
            char* country;
            int age;
            char* virus;
            Date vaccDate;
            State* stateHead = NULL;
            State* state;
            Record* recordsHead = NULL;
            Record* record;
            BloomFilter* bloomsHead = NULL;
            int k =16;
            int recordAccepted = 1;
            SkipList* skipVaccHead = NULL;
            SkipList* skipNonVaccHead = NULL;
            // Seed the time generator
            srand(time(NULL));
            printf("bika sto palaio\n");
            // Read from inputFile and create structs
            while (monitorDir) {
                for (int i=0; i<monitorDir->fileCount; i++) {
                    char* currentFile = malloc(strlen(dir_path) + strlen(monitorDir->files[i])+1);
                    strcpy(currentFile, dir_path);
                    strcat(currentFile, monitorDir->files[i]);
                    inputFile = fopen(currentFile, "r");
                    if(inputFile == NULL) {
                        fprintf(stderr, "Cannot open file: %s\n", currentFile);
                        return 1;
                    }

                    while (getline(&text, &textSize, inputFile) != -1) {

                        // Record to be inserted in structs, unless faulty
                        recordAccepted = 1;
                        // Get citizenID
                        token = strtok(text, " ");
                        citizenID = malloc(strlen(token)+1);
                        strcpy(citizenID, token);

                        // Get firstName
                        token = strtok(NULL, " ");
                        fName = malloc(strlen(token)+1);
                        strcpy(fName, token);

                        // Get lastName
                        token = strtok(NULL, " ");
                        lName = malloc(strlen(token)+1);
                        strcpy(lName, token);

                        // Get country
                        token = strtok(NULL, " ");
                        country = malloc(strlen(token)+1);
                        strcpy(country, token);

                        // Get age
                        token = strtok(NULL, " ");
                        sscanf(token, "%d", &age);

                        // Get virus
                        token = strtok(NULL, " ");
                        virus = malloc(strlen(token)+1);
                        strcpy(virus, token);

                        // Check if YES/NO
                        token = strtok(NULL, " ");

                        // Get vaccine date, if YES
                        if(!strcmp("YES", token)){
                            token = strtok(NULL, " ");
                            sscanf(token, "%d-%d-%d", &vaccDate.day, &vaccDate.month, &vaccDate.year);
                        }
                        else {
                            // If NO, then record should have no date
                            if (token = strtok(NULL, " ")) {
                                printf("ERROR IN RECORD %s %s %s %s %d %s - Has 'NO' with date %s", citizenID, fName, lName, country, age, virus, token);
                                recordAccepted = 0;
                            }
                            else {
                                vaccDate.day=0;
                                vaccDate.month=0;
                                vaccDate.year=0;
                            }
                        }
                        // Insert records into structures
                        if (recordAccepted) {
                            // Add country in State linked list
                            // Add each country once only
                            if (!stateExists(stateHead, country)) {
                                state = insertState(&stateHead, country);
                            }
                            else {
                                state = stateExists(stateHead, country);
                            }

                            // Add record in Record linked list
                            // Check if record is unique
                            int check = checkDuplicate(recordsHead, citizenID, fName, lName, state, age, virus);
                            // Record is unique
                            if (!check) {
                                record = insertSortedRecord(&recordsHead, citizenID, fName, lName, state, age, virus);
                                recordAccepted = 1;
                            }
                            // Record exists for different virus
                            else if (check == 2) {
                                record = insertVirusOnly(&recordsHead, citizenID, virus);
                                recordAccepted = 1;
                            }
                            // Record exists for the same virus
                            else if (check == 1) {
                                recordAccepted = 0;
                            }
                        }

                        if (recordAccepted) {
                            // Add vaccinated record in Bloom Filter
                            if (vaccDate.year != 0) {
                                // Check if we have Bloom Filter for this virus
                                if (virusBloomExists(bloomsHead, virus)) {
                                    insertInBloom(bloomsHead, citizenID, virus);
                                }
                                // Create new Bloom Filter for this virus
                                else {
                                    bloomsHead = createBloom(bloomsHead, virus, bloomSize, k);
                                    insertInBloom(bloomsHead, citizenID, virus);
                                }
                            }      

                            // Add record in Skip List
                            // Separate structure for vaccined Skip Lists
                            if (vaccDate.year != 0) {
                                if (virusSkipExists(skipVaccHead, virus)) {
                                    insertInSkip(skipVaccHead, record, virus, vaccDate);
                                }
                                else {
                                    skipVaccHead = createList(skipVaccHead, virus);
                                    insertInSkip(skipVaccHead, record, virus, vaccDate);
                                }
                            }
                            // Separate structure for non-vaccined Skip Lists
                            else {
                                if (virusSkipExists(skipNonVaccHead, virus)) {
                                    insertInSkip(skipNonVaccHead, record, virus, vaccDate);
                                }
                                else {
                                    skipNonVaccHead = createList(skipNonVaccHead, virus);
                                    insertInSkip(skipNonVaccHead, record, virus, vaccDate);
                                }
                                
                            }
                        }
                        free(citizenID);
                        free(fName);
                        free(lName);
                        free(country);
                        free(virus);
                    }
                }
                monitorDir = monitorDir->next;
            }
////////////////////////////////////////////////////////////////////////////////////////////////////////////
            printStateList(stateHead);
            // printRecordsList(recordsHead);
            // printBloomsList(bloomsHead);
            // printSkipLists(skipVaccHead);
            // printf("---------------------\n");
            // printSkipLists(skipNonVaccHead);
            
            printMonitorDirList(monitorDir);
            freeMonitorDirList(monitorDir);
            free(incMessage->code);
            free(incMessage->body);
            free(incMessage);
            exit(0);
        }
        // if (incMessage->code[0] == 'C') {           
        //     printf("gamieste oi pantes, bgainw\n");
            // sendBytes('R', msg, outfd, bufSize);
        //     free(incMessage->code);
        //     free(incMessage->body);
        //     free(incMessage);        
        //     exit(0);
        // }
        // printMonitorDirList(monitorDir);   
        // freeMonitorDirList(monitorDir);  
        // free(incMessage->code);
        // free(incMessage->body);
        // free(incMessage);
    }

    // printf("Child terminating\n");
    // if (close(outfd) == -1) {
    //     perror("Error closing named pipe");
    // }
    // exit(0);
}