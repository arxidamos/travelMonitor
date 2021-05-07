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
#include "functions.h"
#include "structs.h"

void getMessage (Message* incMessage, int incfd, int bufSize) {
    // printf("getMessage function!\n");
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
    // printf("wtf bitch %s\n", incMessage->body);
    // printf("Message received: %s\n", incMessage.body);
    // printf("Code received: %c\n", incMessage.code[0]);
    free(header);
    return;
}

char* readBytes(char* msg, int length, int fd, int bufSize) {

    char buf[bufSize];
    memset(buf, 0, bufSize);


    int receivedTotal = 0;
    // Read teh message
    while (receivedTotal < length) {
        // If bytes left are less than pipe size, read them only
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
    printf("Sending the Message: %s\n", msg);

    
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

// Wrapper cmp function to use in qsort
int compare (const void * a, const void * b) {
  return strcmp(*(char* const*)a, *(char* const*)b );
}

void analyseMessage (MonitorDir** monitorDir, Message* message, int outfd, int bufSize, char* dir_path) {
    // Message is for countries assignment
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
        char* malakia = malloc(strlen(dir_path)+1);
        strcpy(malakia, dir_path);

        char* gamwspiti = malloc(strlen(message->body)+1);
        strcpy(gamwspiti, message->body);
        

        // Get the file names
        char* files[filesCount];
        int i = 0;
        while ( (current = readdir(dir)) ) {
            char* marika = malloc(strlen(current->d_name)+1);
            strcpy(marika, current->d_name);

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

        // // Iterate through directory's files
        // for (int i=0; i<filesCount; i++) {
        //     // DO STUFF FOR BLOOM FILTERS
        // }
        return;
    }
    // Message is the parent's checking if mapping ready
    else if (message->code[0] == 'F') {
        sendBytes('F', "", outfd, bufSize);
    }
    return;
}

void analyseChildMessage(Message* message, int *readyMonitors) {
    if (message->code[0] =='F') {
        (*readyMonitors)++;
    }

}