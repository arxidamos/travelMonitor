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
    // 1st get the code
    incMessage->code = calloc(1, sizeof(char));
    incMessage->code = readBytes(incMessage->code, 1, incfd, bufSize);

    // 2nd get the header, containing the length of the actual message
    char* header = calloc((DIGITS + 1), sizeof(char));
    header = readBytes(header, DIGITS, incfd, bufSize);
    header[strlen(header)] = '\0';
    incMessage->length = atoi(header+1);    

    // 3rd get the actual message
    incMessage->body = calloc( (incMessage->length+1), sizeof(char));
    incMessage->body = readBytes(incMessage->body, incMessage->length, incfd, bufSize);
    incMessage->body[incMessage->length] = '\0';

    free(header);
    return;
}

char* readBytes(char* msg, int length, int fd, int bufSize) {
    char buf[bufSize];
    memset(buf, 0, bufSize);
    int receivedTotal = 0;

    // Read the message
    while (receivedTotal < length) {
        // If bytes left unread < pipe size, read those only
        int left = length - receivedTotal;
        int bytesToRead = left < bufSize ? left : bufSize;

        int received;
        if ( (received = read(fd, buf, bytesToRead)) == -1 ) {
            perror("Error with reading message");
            exit(1);
        }
        strncpy(msg + receivedTotal, buf, received);
        receivedTotal += received;
    }
    return msg;
}

void sendBytes (char code, char* body, int fd, int bufSize) {  
    char buf[bufSize];
    memset(buf, 0, bufSize);

    // Message: charCode + length of message +  message + \0
    char* msg = malloc( sizeof(char)*(1 + DIGITS + strlen(body) + 1) );
    // First char: code
    msg[0] = code;
    // Next DIGITS chars: message length
    snprintf(msg+1, DIGITS+1, "%0*d", DIGITS, (int)strlen(body));
    // Next strlen(message) chars: message
    snprintf(msg+DIGITS+1, strlen(body)+1, "%s", body);
    // Last char: '\0'
    msg[DIGITS+strlen(body)+1] = '\0';
   
    // Send the message
    int sentTotal = 0;
    while ( sentTotal < strlen(msg)) {
        int left = strlen(msg) - sentTotal;
        int bytesToWrite; //= left < bufSize ? left : bufSize;
        // Bytes left to send can't fit in pipe => send only bufSize bytes
        if (left < bufSize) {
            bytesToWrite = left;
        }
        // Bytes left fit in pipe => send them all
        else {
            bytesToWrite = bufSize;
        }

        strncpy(buf, msg + sentTotal, bytesToWrite);
        
        // Keep sending until whole message sent
        int sent;       
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
    struct dirent** directory;
    int fileCount;
    // Scan directory in an alphabetical order
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

void analyseMessage (MonitorDir** monitorDir, Message* message, int outfd, int* bufSize, char* dir_path, BloomFilter* bloomsHead) {
    // Message 'C': Parent assigned countries to Monitor
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
        // Sort file names alphabetically
        qsort(files, filesCount, sizeof(char*), compare);
        // Reset directory's stream
        rewinddir(dir);

        // Store all info in a structure
        *monitorDir = insertDir(monitorDir, dir, message->body, files, filesCount);
        return;
    }

    if (message->code[0] == '1') {
        *bufSize = atoi(message->body);
    }
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
    
    char* citizenID;
    char* virus;
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
            printf("SQIGUIT sent to childs\n");
            kill(outfd[i], SIGQUIT);
        }

        return 1;
    }
    if (!strcmp(command, "/vaccineStatusBloom")) {
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
    }    
    return 0;
}
