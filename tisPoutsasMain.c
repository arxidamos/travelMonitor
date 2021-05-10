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

    // Scan command line arguments
    // Child's writing pipe is parent's reading pipe
    char* pipeWrite = argv[1];
    // Child's reading pipe is parent's writing pipe
    char* pipeRead = argv[2];
    char* dir_path = malloc(strlen(argv[3]) + 1);
    strcpy(dir_path, argv[3]);

    // Initialize bufSize and bloomSize, before parent sends them through pipes
    int bufSize = 10;
    int bloomSize = 0;

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

    signalHandler();

    // Get first two messages for bufSize and bloomSize
    Message* firstMessage = malloc(sizeof(Message));
    getMessage(firstMessage, incfd, bufSize);
    bufSize = atoi(firstMessage->body);
    getMessage(firstMessage, incfd, bufSize);
    bloomSize = atoi(firstMessage->body);
    free(firstMessage->code);
    free(firstMessage->body);
    free(firstMessage);

    // Structure to store Monitor's directory info
    MonitorDir* monitorDir = NULL;
    // BloomFilter head
    BloomFilter* bloomsHead = NULL;
    State* stateHead = NULL;
    State* state;
    Record* recordsHead = NULL;
    Record* record;
    SkipList* skipVaccHead = NULL;
    SkipList* skipNonVaccHead = NULL;


    while (1) {
        // Read the message
        Message* incMessage = malloc(sizeof(Message));        
        getMessage(incMessage, incfd, bufSize);
        // printf("Message IN CHILD received: %s ", incMessage->body);
        // printf("Code IN CHILD received: %c\n", incMessage->code[0]);
        // Decode the message
        analyseMessage(&monitorDir, incMessage, outfd, &bufSize, dir_path, bloomsHead);
        
        // If 'F' received, country mapping just finished
        if (incMessage->code[0] == 'F') {
////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Initialize variables
            FILE* inputFile;
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
            // BloomFilter* bloomsHead = NULL;
            int k =16;
            int recordAccepted = 1;
            // Seed the time generator
            srand(time(NULL));

            // Iterate through Monitor's directories and files
            MonitorDir* curDir = monitorDir;
            // Read from each directory
            while (curDir) {
                for (int i=0; i<curDir->fileCount; i++) {
                    char* currentFile = malloc(strlen(dir_path) + strlen(curDir->files[i])+1);
                    strcpy(currentFile, dir_path);
                    strcat(currentFile, curDir->files[i]);
                    inputFile = fopen(currentFile, "r");
                    if(inputFile == NULL) {
                        fprintf(stderr, "Cannot open file: %s\n", currentFile);
                        return 1;
                    }
                    // Read from each file and create structs
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
                    fclose(inputFile);
                    free(currentFile);
                }
                curDir = curDir->next;
            }
////////////////////////////////////////////////////////////////////////////////////////////////////////////
            printStateList(stateHead);
            // printRecordsList(recordsHead);
            // printBloomsList(bloomsHead);
            // printSkipLists(skipVaccHead);
            // printf("---------------------\n");
            // printSkipLists(skipNonVaccHead);

            // Initialization finished, send Blooms to parent
            // initialize all the shit, borei na ta ferw edw
            // send the bloom filters
            BloomFilter* current = bloomsHead;
            while (current) {
                // printf("sending Bloom Filter\n");
                // sendBytes('v', current->virus, outfd, bufSize);
                
                // int* array = current->bitArray;
                // char* bitArrayString = calloc(current->size,1);
                // int pos = 0;
                // for (int i=0; i<(current->size/sizeof(int)); i++) {
                //     pos += sprintf(bitArrayString+pos, "%d", array[i]);
                // }
                // for (int i=90; i<100; i++) {
                //     printf("-%c-",bitArrayString[i]);
                // }
                // printf("\n");

                // Get indices and respective content of BF's bitArray
                int length = 0;
                int* indices = malloc(sizeof(int));
                for (int i=0; i<(current->size/sizeof(int)); i++) {
                    // Store only non zero indices
                    if ( current->bitArray[i] != 0) {
                        indices = realloc(indices, (length+2)*sizeof(int));
                        indices[length] = i;    // Save indexSize
                        indices[length+1] = current->bitArray[i];   // Save content
                        length += 2;
                    }
                }
                // for (int i=0; i<length; i++) {
                //     printf("%d ", indices[i]);
                // }
                // printf("\n");
                // Stringify the indices-content array
                char* bArray = malloc(current->size);
                int pos = 0;
                for (int i=0; i<length; i++) {
                    pos += sprintf(bArray+pos, "%d-", indices[i]);
                }
                // printf("charArray: %s\n", charArray);

                char* charArray = malloc( (strlen(current->virus) + 1 + strlen(bArray) + 1) * sizeof(char));
                strcpy(charArray, current->virus);
                strcat(charArray, ";");
                strcat(charArray, bArray);

                // Send the stringified array
                sendBytes('v', charArray, outfd, bufSize);
                free(indices);
                free(charArray);

                current = current->next;
            }
            // printf("Child %d\n", (int)getpid());
            // vaccineStatusBloom(bloomsHead, "1958", "SARS-1");
            // printf("Child %d\n", (int)getpid());
            printf("o child %d eimai, esteila ta Bloom Filter\n", (int)getpid());
            sendBytes('F', "", outfd, bufSize);
            free(incMessage->code);
            free(incMessage->body);
            free(incMessage);

        }
        
        // Signal handler for SIGINT and SIGQUIT
        checkSigQuit(&stateHead, &recordsHead, &bloomsHead, &skipVaccHead, &skipNonVaccHead, monitorDir, dir_path);

        // // Wait for other commands
        // if (incMessage->code[0] == 'f') {
        //     // free(dir_path);
        //     // free(incMessage->code);
        //     // free(incMessage->body);
        //     // free(incMessage);
        //     // freeMonitorDirList(monitorDir);

        //     freeStateList(stateHead);
        //     freeRecordList(recordsHead);
        //     freeBlooms(bloomsHead);
        //     freeSkipLists(skipVaccHead);
        //     freeSkipLists(skipNonVaccHead);

        //     printf("o child %d eimai, eida to f, exiting...\n", (int)getpid());
        //     exit(0);
        // }


    }
}