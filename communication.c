#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include "functions.h"
#include "structs.h"

// static sigset_t signalSet;

// Read incoming message
int getMessage (Message* incMessage, int incfd, int bufSize) {
    // 1st get the code
    incMessage->code = calloc(1, sizeof(char));
    if ( (incMessage->code = readBytes(incMessage->code, 1, incfd, bufSize)) == NULL) {
        return -1;
    }

    // 2nd get the header, containing the length of the actual message
    char* header = calloc((LENGTH + 1), sizeof(char));
    header = readBytes(header, LENGTH, incfd, bufSize);
    header[strlen(header)] = '\0';
    incMessage->length = atoi(header+1);    

    // 3rd get the actual message
    incMessage->body = calloc( (incMessage->length+1), sizeof(char));
    incMessage->body = readBytes(incMessage->body, incMessage->length, incfd, bufSize);
    incMessage->body[incMessage->length] = '\0';

    free(header);
    return 1;
}

// Read incoming message aux function
char* readBytes(char* msg, int length, int fd, int bufSize) {
    char buf[bufSize];
    memset(buf, 0, bufSize);
    
    // Get the message
    int receivedSoFar = 0;
    while (receivedSoFar < length) {
        int received;
        int left = length - receivedSoFar;
        // Bytes left unread < buf size => read them all
        if (left < bufSize) {
            if ( (received = read(fd, buf, left)) == -1 ) {
                perror("Didn't read message");
                return(NULL);
            }
        }
        // Bytes left unread > buf size => read buf size only
        else {
            if ( (received = read(fd, buf, bufSize)) == -1 ) {
                perror("Didn't read message");
                return(NULL);
            }
        }
        strncpy(msg + receivedSoFar, buf, received);
        // Keep receiving until whole message received
        receivedSoFar += received;
    }
    return msg;
}

// Send message
void sendBytes (char code, char* body, int fd, int bufSize) {  
    char buf[bufSize];
    memset(buf, 0, bufSize);

    // Message: [charCode] + [length of message] +  [message] + [\0]
    char* msg = malloc( sizeof(char)*(1 + LENGTH + strlen(body) + 1) );
    // First char: code
    msg[0] = code;
    // Next LENGTH chars: message length
    snprintf(msg+1, LENGTH+1, "%0*d", LENGTH, (int)strlen(body));
    // Next strlen(message) chars: message
    snprintf(msg+LENGTH+1, strlen(body)+1, "%s", body);
    // Last char: '\0'
    msg[LENGTH+strlen(body)+1] = '\0';
   
    // Send the message
    int sentSoFar = 0;
    while (sentSoFar < strlen(msg)) {
        int sent;
        int left = strlen(msg) - sentSoFar;
        // Bytes left for sending fit in buf => send them all
        if (left < bufSize) {
            strncpy(buf, msg + sentSoFar, left);
            if ( (sent = write(fd, buf, left)) == -1) {
                perror("Error with writing message");
                exit(1);
            }
        }
        // Bytes left don't fit in buf => send only bufSize bytes
        else {
            strncpy(buf, msg + sentSoFar, bufSize);
            if ( (sent = write(fd, buf, bufSize)) == -1) {
                perror("Error with writing message");
                exit(1);
            }
        }        
        // Keep sending until whole message sent
        sentSoFar += sent;
    }
    free(msg);
    return;
}

// Send previous Monitor's countries to new Monitor
void resendCountryDirs (char* dir_path, int numMonitors, int outfd, ChildMonitor childMonitor, int bufSize) {
    for (int i=0; i<childMonitor.countryCount; i++) {
        sendBytes('C', childMonitor.country[i], outfd, bufSize);
        printf("Sent country DIrs to new monitor\n");
    }
    // When mapping ready, send 'F' message
    sendBytes('F', "", outfd, bufSize);
    printf("Sent F message to new monitor\n");
}

// Map countries to Monitors, round robin
void mapCountryDirs (char* dir_path, int numMonitors, int outfd[], ChildMonitor childMonitor[], int bufSize) {
    struct dirent** directory;
    int fileCount;
    // Scan directory in alphabetical order
    fileCount = scandir(dir_path, &directory, NULL, alphasort);
    
    // Send each directory to the right Monitor, round robin
    int i=0;
    for (int j=0; j<fileCount; j++) {
       char* dirName = directory[j]->d_name;
        // Avoid assigning dirs . and ..
        if (strcmp(dirName, ".") && strcmp(dirName, "..")) {     
            // Send country assignment command
            sendBytes('C', dirName, outfd[i], bufSize);
            // Store each country in its childMonitor structure
            if (childMonitor[i].countryCount == 0) {
                childMonitor[i].countryCount++;
                childMonitor[i].country = malloc(sizeof(char*)*(childMonitor[i].countryCount));
                childMonitor[i].country[0] = malloc(strlen(dirName)+1);
                strcpy(childMonitor[i].country[0], dirName);
            }
            else {
                childMonitor[i].countryCount++;
                childMonitor[i].country = realloc(childMonitor[i].country, sizeof(char*)*(childMonitor[i].countryCount));
                childMonitor[i].country[childMonitor[i].countryCount-1] = malloc(strlen(dirName)+1);
                strcpy(childMonitor[i].country[childMonitor[i].countryCount-1], dirName);
            }
            i = (i+1) % numMonitors;
        }
    }
    for (int i = 0; i < fileCount; i++) {
        free(directory[i]);
    }
    free(directory);

    // When mapping ready, send 'F' message
    for (int i=0; i<numMonitors; i++) {
        sendBytes('F', "", outfd[i], bufSize);
    }
    printf("FINISHED MAPPING COUNTRIES\n");
}

void processUsr1(MonitorDir** monitorDir, int outfd, int bufSize, int bloomSize, char* dir_path, BloomFilter** bloomsHead, State** stateHead, Record** recordsHead, SkipList** skipVaccHead, SkipList** skipNonVaccHead) {
    // Iterate through both MonitorDir's directories and the actual directories    
    MonitorDir* current = (*monitorDir);
    while(current) {
        // Get each dir's files alphabetically
        struct dirent** directory;
        int filesCount = 0;
        char* currentPath = malloc( (strlen(dir_path) + 1 + strlen(current->country) + 1)*sizeof(char) );
        strcpy(currentPath, dir_path);
        strcat(currentPath, "/");
        strcat(currentPath, current->country);
        filesCount = scandir(currentPath , &directory, NULL, alphasort);
        
        // Iterate through files
        for (int i=0; i<filesCount; i++) {
            if ( strcmp(directory[i]->d_name, ".") && strcmp(directory[i]->d_name, "..") ) {
                // Adjust file name to compare with struct's files
                char* file = malloc( (strlen(current->country) + 1 + strlen(directory[i]->d_name) + 1)*sizeof(char) );
                strcpy(file, current->country);
                strcat(file, "/");
                strcat(file, directory[i]->d_name);

                // File is not included in struct's files
                if ( !(fileInDir(current, file)) ) {
                    // Add file to MonitorDir struct
                    insertFile(&current, file);

                    // Add file contents to Monitor's data
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
                    State* state;
                    Record* record;
                    int k =16;
                    
                    // Open this file
                    char* currentFile = malloc(strlen(dir_path) + strlen(file)+1);
                    strcpy(currentFile, dir_path);
                    strcat(currentFile, file);
                    inputFile = fopen(currentFile, "r");
                    if(inputFile == NULL) {
                        fprintf(stderr, "Cannot open file: %s\n", currentFile);
                        exit(1);
                    }
                    // Read from file and add to structs
                    while (getline(&text, &textSize, inputFile) != -1) {
                        // Record to be inserted in structs, unless faulty
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
                            if ( (token = strtok(NULL, " ")) ) {
                                printf("ERROR IN RECORD %s %s %s %s %d %s - Has 'NO' with date %s", citizenID, fName, lName, country, age, virus, token);
                                // recordAccepted = 0;
                            }
                            else {
                                vaccDate.day=0;
                                vaccDate.month=0;
                                vaccDate.year=0;
                            }
                        }
                        int check = insertCitizenCheck((*recordsHead), citizenID, fName, lName, country, age, virus);
                        // Check if new record is inconsistent
                        if (check == 1) {
                            printf("CitizenID %s already in use for another record. Please enter a different one.\n", citizenID);
                        }
                        // New record is consistent
                        else {
                            SkipList* vList;
                            SkipList* nonVList;
                            SkipNode* node;

                            // Add country in State linked list
                            state = stateExists((*stateHead), country);
                            if (!state) {
                                state = insertState(stateHead, country);
                            }

                            // Record already exists. Add new virus for record
                            if (check == 2) {
                                record = insertVirusOnly(recordsHead, citizenID, virus);
                            }
                            // Record is new. Add new record in record linked list
                            else if (!check) {
                                record = insertSortedRecord(recordsHead, citizenID, fName, lName, state, age, virus);
                            }

                            // Vaccinated Skip List for this virus exists
                            if ( (vList = virusSkipExists((*skipVaccHead), virus)) ) {
                                // CitizenID already vaccinated
                                if ( (node = searchSkipList(vList, citizenID)) ) {
                                    printf("ERROR: CITIZEN %s ALREADY VACCINATED ON %d-%d-%d\n", citizenID, node->vaccDate.day, node->vaccDate.month, node->vaccDate.year);
                                }
                                // CitizenID not yet vaccinated
                                else {
                                    // Add record in Bloom Filter
                                    insertInBloom((*bloomsHead), citizenID, virus);
                                    // Add record in Skip List
                                    insertInSkip((*skipVaccHead), record, virus, vaccDate);
                                }
                            }
                            // Vaccinated Skip List for this virus doesn't exist
                            else {
                                // Create Skip List for this virus
                                (*skipVaccHead) = createList((*skipVaccHead), virus);
                                // Add record in Skip List
                                insertInSkip((*skipVaccHead), record, virus, vaccDate);
                            
                                // Create Bloom Filter for this virus
                                (*bloomsHead) = createBloom((*bloomsHead), virus, bloomSize, k);
                                // Add record in Bloom Filter
                                insertInBloom((*bloomsHead), citizenID, virus);
                            }

                            // Non vaccinated Skip List for this virus exists
                            if ( (nonVList = virusSkipExists((*skipNonVaccHead), virus)) ) {
                            
                                // CitizenID in this Skip List exists
                                if ( (node = searchSkipList(nonVList, citizenID)) ) {
                                    // Remove node from non vaccinated Skip List
                                    removeFromSkip (nonVList, node);
                                }
                            }
                        }
                    }

                    // Send signal to Parent to make him anticipate message
                    if (kill(getppid(), SIGUSR1) == -1) {
                        perror("Error with sending SIGUSR1 to parent");
                        exit(1);
                    }
                    // Initialization finished, send Blooms to parent
                    updateParentBlooms(*bloomsHead, outfd, bufSize);
                    // Report setup finished to Parent
                    sendBytes('F', "", outfd, bufSize);
                    // printf("Sent 'f' message ston parent sto telos tis processUsr1()\n");
                }
            }
        }
        free(currentPath);
        current = current->next;
    }
}

// void checkSigQuit (State** stateHead, Record** recordsHead, BloomFilter** bloomsHead, SkipList** skipVaccHead, SkipList** skipNonVaccHead, MonitorDir* monitorDir, char* dir_path) {
    //     // Check for SIGQUIT
    //     if (flagQuit) {
    //         // Deallocate memory
    //         free(dir_path);
    //         freeMonitorDirList(monitorDir);
    //         freeStateList(*stateHead);
    //         freeRecordList(*recordsHead);
    //         freeBlooms(*bloomsHead);
    //         freeSkipLists(*skipVaccHead);
    //         freeSkipLists(*skipNonVaccHead);
    //         printf("exiting from child\n");
    //         exit(1);
    //     }
    //     flagQuit = 0;
// }

// Compare two files (aux function for qsort)
int compare (const void * a, const void * b) {
  return strcmp(*(char* const*)a, *(char* const*)b );
}

// Analyse incoming message in Monitor
void analyseMessage (MonitorDir** monitorDir, Message* message, int outfd, int* bufSize, int* bloomSize, char* dir_path, BloomFilter** bloomsHead, State** stateHead, Record** recordsHead, SkipList** skipVaccHead, SkipList** skipNonVaccHead, int* accepted, int* rejected) {
    // Message 'C': Parent assigns countries to Monitor
    if (message->code[0] == 'C') {
        // Open directory
        char full_path[_POSIX_PATH_MAX];
        snprintf(full_path, _POSIX_PATH_MAX, "%s/%s", dir_path, message->body);
        DIR* dir;
        if (!(dir = opendir(full_path))) {
            perror("Error with opening directory");
            exit(1);
        }

        // Get number of directory's files 
        int filesCount = 0;
        struct dirent* current;
        while ( (current = readdir(dir)) ) {
            // Count all files except for "." and ".."
            if ( strcmp(current->d_name, ".") && strcmp(current->d_name, "..") ) {
                filesCount++;
            }
        }
        // Reset directory's stream
        rewinddir(dir);

        // Get the file names
        char** files = malloc(sizeof(char*)*filesCount);
        int i = 0;
        while ( (current = readdir(dir)) ) {
            if ( strcmp(current->d_name, ".") && strcmp(current->d_name, "..") ) {
                files[i] = malloc(strlen(message->body) + 1 + strlen(current->d_name) + 1);
                strcpy(files[i], message->body);
                strcat(files[i], "/");
                strcat(files[i], current->d_name);
                i++;
            }
        }
        // Sort file names alphabetically
        qsort(files, filesCount, sizeof(char*), compare);
        // Reset directory's stream
        rewinddir(dir);

        // Store all info in a structure
        *monitorDir = insertDir(monitorDir, dir, message->body, files, filesCount);
        free(files);
        return;
    }
    // Message '1': Parent's 1st message with bufSize
    else if (message->code[0] == '1') {
        *bufSize = atoi(message->body);
        return;
    }
    // Message '2': Parent's 2nd message with bloomSize
    else if (message->code[0] == '2') {
        *bloomSize = atoi(message->body);
        return;
    }
    // Message 'F': Parent finishes assigning. Create structures - send Bloom Filters. 
    else if (message->code[0] == 'F') {
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
        State* state;
        Record* record;
        int k =16;
        int recordAccepted = 1;
        // Iterate through Monitor's directories and files
        MonitorDir* curDir = *monitorDir;
        // Read from each directory
        while (curDir) {
            for (int i=0; i<curDir->fileCount; i++) {
                char* currentFile = malloc(strlen(dir_path) + strlen(curDir->files[i])+1);
                strcpy(currentFile, dir_path);
                strcat(currentFile, curDir->files[i]);
                inputFile = fopen(currentFile, "r");
                if(inputFile == NULL) {
                    fprintf(stderr, "Cannot open file: %s\n", currentFile);
                    exit(1);
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
                        if ( (token = strtok(NULL, " ")) ) {
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
                        if (!stateExists(*stateHead, country)) {
                            state = insertState(stateHead, country);
                        }
                        else {
                            state = stateExists(*stateHead, country);
                        }

                        // Add record in Record linked list
                        // Check if record is unique
                        int check = checkDuplicate(*recordsHead, citizenID, fName, lName, state, age, virus);
                        // Record is unique
                        if (!check) {
                            record = insertSortedRecord(recordsHead, citizenID, fName, lName, state, age, virus);
                            recordAccepted = 1;
                        }
                        // Record exists for different virus
                        else if (check == 2) {
                            record = insertVirusOnly(recordsHead, citizenID, virus);
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
                            if (virusBloomExists(*bloomsHead, virus)) {
                                insertInBloom(*bloomsHead, citizenID, virus);
                            }
                            // Create new Bloom Filter for this virus
                            else {
                                *bloomsHead = createBloom(*bloomsHead, virus, *bloomSize, k);
                                insertInBloom(*bloomsHead, citizenID, virus);
                            }
                        }      

                        // Add record in Skip List
                        // Separate structure for vaccined Skip Lists
                        if (vaccDate.year != 0) {
                            if (virusSkipExists(*skipVaccHead, virus)) {
                                insertInSkip(*skipVaccHead, record, virus, vaccDate);
                            }
                            else {
                                *skipVaccHead = createList(*skipVaccHead, virus);
                                insertInSkip(*skipVaccHead, record, virus, vaccDate);
                            }
                        }
                        // Separate structure for non-vaccined Skip Lists
                        else {
                            if (virusSkipExists(*skipNonVaccHead, virus)) {
                                insertInSkip(*skipNonVaccHead, record, virus, vaccDate);
                            }
                            else {
                                *skipNonVaccHead = createList(*skipNonVaccHead, virus);
                                insertInSkip(*skipNonVaccHead, record, virus, vaccDate);
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

        // Initialization finished, send Blooms to parent
        updateParentBlooms(*bloomsHead, outfd, *bufSize);

        // Report setup finished to Parent
        sendBytes('F', "", outfd, *bufSize);
        free(message->code);
        free(message->body);
        free(message);

    }
    // Message 't': Parent sends travelRequest query
    else if (message->code[0] == 't') {
        char* citizenID;
        char* token2;
        char* token3;
        char* virus;
        char* countryTo;
        char* dateString;
        Date date;
        // 1st token: citizenID
        citizenID = strtok_r(message->body, ";", &token2);
        // 2nd token: virus
        virus = strtok_r(token2, ";", &token3);
        // 3rd token: countryTo
        countryTo = strtok_r(token3, ";", &dateString);
        // 4th token: dateString
        sscanf(dateString, "%d-%d-%d", &date.day, &date.month, &date.year);

        // Check in structures
        char* answer = processTravelRequest(*skipVaccHead, citizenID, virus, date);

        // Answer to Parent, also pass 'countryTo'
        char* fullString = malloc((strlen(countryTo) + 1 + strlen(answer) + 1)*sizeof(char));
        strcpy(fullString, countryTo);
        strcat(fullString, ";");
        strcat(fullString, answer);
        sendBytes('t', fullString, outfd, *bufSize);
        free(fullString);
    }
    // Message '+': Parent informs to increment counters
    else if (message->code[0] == '+') {
        // pritnf("Why don't i get it?\n");
        if (!strcmp(message->body, "YES")) {
            (*accepted)++;
        }
        else if (!strcmp(message->body, "NO") || !strcmp(message->body, "BUT")) {
            (*rejected)++;
        }
    }
    // free(message->code);
    // free(message->body);`
    // free(message);
    return;
}

// Analyse incoming message in Parent
void analyseChildMessage(Message* message, ChildMonitor* childMonitor, int numMonitors, int *readyMonitors, int* outfd, int bufSize, BloomFilter** bloomsHead, int bloomSize, int* accepted, int* rejected) {
    // Message 'F': Monitor reports setup finished
    if (message->code[0] =='F') {
        printf("PArent received f message\n");
        (*readyMonitors)++;
    }
    // Message 'B': Monitor sends Bloom Filter
    if (message->code[0] == 'B') {    
        BloomFilter* bloomFilter = NULL;
        int k = 16;

        char* token1;
        char* token2;
        token1 = strtok_r(message->body, ";", &token2);

        if ( (bloomFilter = insertBloomInParent(bloomsHead, token1, bloomSize, k)) ) {
            updateBitArray(bloomFilter, token2);
        }
        else {
            *bloomsHead = createBloom(*bloomsHead, token1, bloomSize, k);
            updateBitArray(*bloomsHead, token2);
        }

        // bloomsHead = createBloom(bloomsHead, incMessage->body, bloomSize, k);
        // printf("Pira mes ston parent: bloom virus %s\n", message->body);
    }
    // Message 't': Monitor answers travelRequest query
    if (message->code[0] == 't') {
        char* countryTo;
        char* answer;

        // 1st token: countryTo, 2nd token: answer string
        countryTo = strtok_r(message->body, ";", &answer);

        if (!strcmp(answer, "YES")) {
            printf("REQUEST ACCEPTED – HAPPY TRAVELS\n");
            (*accepted)++;
        }        
        else if (!strcmp(answer, "NO")) {
            printf("REQUEST REJECTED – YOU ARE NOT VACCINATED\n");
            (*rejected)++;
        }
        else if (!strcmp(answer, "BUT")) {
            printf("REQUEST REJECTED – YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n");
            (*rejected)++;            
        }
        (*readyMonitors)++;
        
        // Inform the Monitor in charge of countryTo
        for (int i=0; i<numMonitors; i++) {
            for (int j=0; j<childMonitor[i].countryCount; j++) {
                if ( !strcmp(childMonitor[i].country[j], countryTo) ) {
                    // Send increment counter message
                    sendBytes('+', answer, outfd[i], bufSize);
                    break;
                }
            }
        }
    }
    
}

// Receive commands from user
int getUserCommand(int* readyMonitors, int numMonitors, ChildMonitor* childMonitor, BloomFilter* bloomsHead,
 char* dir_path, DIR* input_dir, int* incfd, int* outfd, int bufSize, int bloomSize, int* accepted, int* rejected) {
    // size_t inputSize;
    // char* input = NULL;
    char* command = NULL;

    int size = 512;
    char input[size];


    // Get user commands
    if ( (fgets(input, size, stdin) == NULL) ) {
        // Check if some signal's flag is on
        checkSignalFlagsParent(dir_path, bufSize, bloomSize, readyMonitors, numMonitors, incfd, outfd, childMonitor);
        return -1;
    }

    input[strlen(input)-1] = '\0'; // Cut terminating '\n' from string
    
    char* citizenID;
    char* virus;
    Date date;

    // Get the command
    command = strtok(input, " ");
    if (!strcmp(command, "/exit")) {

        printf("parent:\naccepted: %d\nrejected: %d\n", (*accepted), (*rejected));

        // Send SIGKILL signal to every child
        for (int i = 0; i < numMonitors; ++i) {
            printf("SIGKILL sent to child\n");
            kill(childMonitor[i].pid, SIGKILL);
        }

        // Wait till every child finishes
        for (int i = 0; i < numMonitors; ++i) {
            waitpid(-1, NULL, 0);
            printf("Child listened SIGKILL, terminated\n");

        }
        // free(input);
        free(dir_path);
        closedir(input_dir);
        for (int i=0; i<numMonitors; i++) {
            for (int j=0; j<childMonitor[i].countryCount; j++) {
                free(childMonitor[i].country[j]);
            }
            free(childMonitor[i].country);
        }
        freeBlooms(bloomsHead);

     
        return 1;
    }
    else if (!strcmp(command, "/vaccineStatusBloom")) {
        // Get citizenID
        command = strtok(NULL, " ");
        if (command) {
            citizenID = malloc(strlen(command)+1);
            strcpy(citizenID, command);

            // Get virusName
            command = strtok(NULL, " ");
            if(command) {
                virus = malloc(strlen(command)+1);
                strcpy(virus, command);
                vaccineStatusBloom(bloomsHead, citizenID, virus);
                free(virus);
            }
            else {
                printf("Please enter a virus name\n");
            }
            free(citizenID);
        }
        else {
            printf("Please enter a citizenID and a virus name\n");
        }
        // free(input);
    }
    else if (!strcmp(command, "/travelRequest")) {
        // Get citizenID
        command = strtok(NULL, " ");
        if (command) {
            citizenID = malloc(strlen(command)+1);
            strcpy(citizenID, command);

            // Get date
            command = strtok(NULL, " ");
            if (command) {
                sscanf(command, "%d-%d-%d", &date.day, &date.month, &date.year);

                // Get countryFrom
                command = strtok(NULL, " ");
                if (command) {
                    char* countryFrom = malloc(strlen(command)+1);
                    strcpy(countryFrom, command);

                    // Get countryTo
                    command = strtok(NULL, " ");
                    if (command) {
                        char* countryTo = malloc(strlen(command)+1);
                        strcpy(countryTo, command);

                        // Get virusName
                        command = strtok(NULL, " ");
                        if (command) {
                            virus = malloc(strlen(command)+1);
                            strcpy(virus, command);

                            // vaccineStatusBloom(bloomsHead, citizenID, virus);
                            travelRequest(readyMonitors, bloomsHead, childMonitor, numMonitors, incfd, outfd, bufSize, accepted, rejected, citizenID, countryFrom, countryTo, virus, date);

                            free(virus);
                        }
                        else {
                            printf("Please also enter the following parameter: virusName.\n");
                        }
                        free(countryTo);
                    }
                    else {
                        printf("Please also enter the following parameters: countryTo virusName.\n");
                    }
                    free(countryFrom);
                }
                else {
                    printf("Please also enter the following parameters: countryFrom countryTo virusName.\n");
                }
            }
            else {
                printf("Please also enter the following parameters: date countryFrom countryTo virusName.\n");
            }
            free(citizenID);
        }
        else {
            printf("Please enter the following parameters: citizenID date countryFrom countryTo virusName.\n");
        }
        // free(input);
    }
    else {
        printf("Command '%s' is unknown\n", command);
        printf("Please type a known command:\n");
        // free(input);
    }
    return 0;
}

// Send Blooms to Parent
void updateParentBlooms(BloomFilter* bloomsHead, int outfd, int bufSize) {
    BloomFilter* current = bloomsHead;
    // Iterate through all Bloom Filters' bit arrays
    while (current) {
        // Store every non-zero number and its index
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

        // Stringify the 'indices-content' array
        char* bArray = malloc(current->size);
        int pos = 0;
        for (int i=0; i<length; i++) {
            pos += sprintf(bArray+pos, "%d-", indices[i]);
        }

        // Final message: BF's virus + ';' + BF's bit array's indices-content
        char* charArray = malloc( (strlen(current->virus) + 1 + strlen(bArray) + 1) * sizeof(char));
        strcpy(charArray, current->virus);
        strcat(charArray, ";");
        strcat(charArray, bArray);

        // Send BF info message to Parent
        sendBytes('B', charArray, outfd, bufSize);
        free(indices);
        free(charArray);

        current = current->next;
    }
}