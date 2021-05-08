#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include "structs.h"
#include "functions.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>
#include "functions.h"
#include "structs.h"
#include <errno.h>
static volatile sig_atomic_t flagQuit;
static sigset_t signalSet;

void getMessage (Message* incMessage, int incfd, int bufSize) {
    // First get the code
    incMessage->code = calloc(1, sizeof(char));
    incMessage->code = readBytes(incMessage->code, 1, incfd, bufSize);

    // Fist get the header, containing the length of the actual message
    char* header = calloc((DIGITS + 1), sizeof(char));
    header = readBytes(header, DIGITS, incfd, bufSize);
    header[strlen(header)] = '\0';
    incMessage->length = atoi(header+1);    

    // Then get the actual message
    incMessage->body = calloc( (incMessage->length+1), sizeof(char));
    incMessage->body = readBytes(incMessage->body, incMessage->length, incfd, bufSize);
    incMessage->body[incMessage->length] = '\0';
    // printf("Message received: %s\n", incMessage.body);
    // printf("Code received: %c\n", incMessage.code[0]);
    free(header);
    return;
}

char* readBytes(char* msg, int length, int fd, int bufSize) {
    char buf[bufSize];
    memset(buf, 0, bufSize);

    int receivedTotal = 0;
    // Read the message
    while (receivedTotal < length) {
        // If bytes left are less than pipe size, read those only
        int left = length - receivedTotal;
        int bytesToRead = left < bufSize ? left : bufSize;

        int received;
        if ( (received = read(fd, buf, bytesToRead)) == -1 ) {
            perror("Error with reading message");
            exit(1);
        }
        // printf("Reading...");
        // for (int k=0; k<received; k++) {
        //     printf("%c", buf[k]);
        // }
        // printf("\n");
        strncpy(msg + receivedTotal, buf, received);
        receivedTotal += received;
    }
    return msg;
}

void sendBytes (char code, char* body, int fd, int bufSize) {
    
    char buf[bufSize];
    memset(buf, 0, bufSize);

    // code + header (length of message and leading zeros) + actual message + \0
    char* msg = malloc( sizeof(char)*(1 + DIGITS + strlen(body) + 1) );
    msg[0] = code;
    snprintf(msg+1, DIGITS+1, "%0*d", DIGITS, (int)strlen(body));
    snprintf(msg+DIGITS+1, strlen(body)+1, "%s", body);
    msg[DIGITS+strlen(body)+1] = '\0';
    // printf("Sending the Message: %s\n", msg);
   
    // Send the message
    int sentTotal = 0;
    while ( sentTotal < strlen(msg)) {

        // If bytes left to send can fit in pipe, send them all. Else send only bufSize bytes (which fit)
        int left = strlen(msg) - sentTotal;
        int bytesToWrite = left < bufSize ? left : bufSize;

        strncpy(buf, msg + sentTotal, bytesToWrite);
        int sent;
        // printf("Sending: ");
        // for (int k=0; k<bytesToWrite; k++) {
        //     printf("%c", buf[k]);
        // }
        // printf("\n");
        
        if ( (sent = write(fd, buf, bytesToWrite)) == -1) {
            perror("Error with writing message");
            exit(1);
        }
        sentTotal += sent;
    }
    free(msg);
    return;
}

void mapCountryDirs (char* dir_path, int numMonitors, int outfd[], ChildMonitor childMonitor[], int bufSize) {
    int i=0;

    struct dirent** directory;
    int fileCount;

    fileCount = scandir(dir_path, &directory, NULL, alphasort);
    
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

    for (int i=0; i<numMonitors; i++) {
        sendBytes('F', "", outfd[i], bufSize);
    }
    printf("FINISHED MAPPING COUNTRIES\n");
}

void sigchldHandler () {
    int status;
    pid_t pid;
    while (1) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0) {
            return;
        }
        else if (pid == -1) {
            return;
        }
        else {
            printf("...reaching parent - %lu  with return code %d \n",(long)pid, status);
        }
    }
}

void sigQuitHandler (int sigNum) {
    printf("Caught a SIGQUIT\n");
    flagQuit = 1;
}

void signalHandler(void) {
    struct sigaction sigAct;
    // Signal set
    sigset_t set;
    sigfillset(&set);

    // Block all signals (that are present in set)
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        perror("Error with setting signal mask");
    }
    memset(&sigAct, 0, sizeof(struct sigaction));
    // Initialise flags
    flagQuit = 0;

    sigAct.sa_handler = sigQuitHandler;
    sigaction(SIGQUIT, &sigAct, NULL);

    // // Add only certain signals in signalSet, will need them (to block when receiving commands)
    // sigemptyset(&signalSet);
    // sigaddset(&signalSet, SIGINT);
    // sigaddset(&signalSet, SIGQUIT);
    // sigaddset(&signalSet, SIGUSR1);

    // Unblock all signals
    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        perror("Error with setting signal mask");
    }
}

void checkSigQuit (State** stateHead, Record** recordsHead, BloomFilter** bloomsHead, SkipList** skipVaccHead, SkipList** skipNonVaccHead, MonitorDir* monitorDir, char* dir_path) {
    // Check for SIGQUIT
    printf("energopoiisi tou checkSiqQUit\n");
    if (flagQuit) {
        // Deallocate memory
        free(dir_path);
        freeMonitorDirList(monitorDir);
        freeStateList(*stateHead);
        freeRecordList(*recordsHead);
        freeBlooms(*bloomsHead);
        freeSkipLists(*skipVaccHead);
        freeSkipLists(*skipNonVaccHead);
        printf("exiting from child\n");
        exit(1);
    }
    flagQuit = 0;
}


// Wrapper cmp function to use in qsort
int compare (const void * a, const void * b) {
  return strcmp(*(char* const*)a, *(char* const*)b );
}

void analyseMessage (MonitorDir** monitorDir, Message* message, int outfd, int bufSize, char* dir_path, BloomFilter* bloomsHead) {
    // Message 'C' is for countries assignment
    if (message->code[0] == 'C') {
        // Open the directory
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
            if ( strcmp(current->d_name, ".") && strcmp(current->d_name, "..") ) {
                filesCount++;
            }
        }
        // Reset directory's stream
        rewinddir(dir);

        // Get the file names
        char* files[filesCount];
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

        qsort(files, filesCount, sizeof(char*), compare);
        // Reset directory's stream
        rewinddir(dir);

        // Store all info in a structure
        *monitorDir = insertDir(monitorDir, dir, message->body, files, filesCount);
        // sendBytes('C', "", outfd, bufSize);
        return;
    }
    // Message 'F' is the parent's checking if mapping ready
    // else if (message->code[0] == 'F') {
    //     // initialize all the shit, borei na ta ferw edw
    //     // send the bloom filters
    //     BloomFilter* current = bloomsHead;
    //     while (current) {
    //         printf("sending\n");
    //         sendBytes('v', current->virus, outfd, bufSize);
    //         char bitArrayString[current->size];
    //         sprintf(bitArrayString, "%d", current->size);
    //         sendBytes('b', bitArrayString, outfd, bufSize);
    //         current = current->next;
    //     }
    //     sendBytes('F', "", outfd, bufSize);
    // }
    return;
}

void analyseChildMessage(Message* message, int *readyMonitors, int outfd, int bufSize) {
    // Monitor sent ready message
    if (message->code[0] =='F') {
        (*readyMonitors)++;
        // sendBytes('f', "", outfd, bufSize);
    }
    else if (message->code[0] == 'C') {
        sendBytes('F', "", outfd, bufSize);
    }
    // Monitor 
    // else if (message->code[0] =='F')

}

int getUserCommand(char* input, size_t inputSize, char* command, int* readyMonitors, int numMonitors, BloomFilter* bloomsHead, ChildMonitor* childMonitor, char* dir_path, DIR* input_dir, int* outfd, int bufSize) {
    getline(&input, &inputSize, stdin);
    input[strlen(input)-1] = '\0'; // Cut terminating '\n' from string
    
    // Get the command
    command = strtok(input, " ");
    if (!strcmp(command, "/exit")) {
        free(input);
        free(dir_path);
        closedir(input_dir);
        for (int i=0; i<numMonitors; i++) {
            for (int j=0; j<childMonitor[i].countryCount; j++) {
                free(childMonitor[i].country[j]);
            }
            free(childMonitor[i].country);
        }
        freeBlooms(bloomsHead);
        // Send SIGQUIT signal to childs
        for (int i = 0; i < numMonitors; ++i) {
            printf("esteila to sigquit sta childs\n");
            kill(outfd[i], SIGQUIT);
        }

        return 1;
    }
    return 0;
}




// void serializeData (BloomFilter* bloomsHead, Buffer* buffer) {
    //     serialiseVirus(bloomsHead->virus, buffer);
    //     serialiseInt(bloomsHead->size, buffer);
    //     serialiseInt(bloomsHead->k, buffer);
    //     serialiseBitArray(bloomsHead->bitArray, buffer);
    //     serialiseNext(bloomsHead->next, buffer);
    // }

    // void createBuffer (int bufSize) {
    //     Buffer* buffer = malloc(sizeof(Buffer));
    //     buffer->data = malloc(bufSize);
    //     buffer->size = bufSize;
    //     buffer->next = 0;
    // }

    // void allocSpace (Buffer* buffer, int bytes) {
    //     // if (buffer->next + bytes > buffer->size) 
    // }

    // void serialiseInt(int x, Buffer* buffer) {
    //     allocSpace(buffer, sizeof(int));
// }