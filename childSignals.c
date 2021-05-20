#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include "functions.h"
#include "structs.h"

static volatile sig_atomic_t flagQuit = 0;
static volatile sig_atomic_t flagInt = 0;
static volatile sig_atomic_t flagUsr1 = 0;

// Install signal handlers
void handleSignals (void) {
    static struct sigaction sigAct;

    sigfillset(&sigAct.sa_mask);
    sigAct.sa_handler = sigIntHandler;
    sigaction(SIGINT, &sigAct, NULL);
    
    sigAct.sa_handler = sigQuitHandler;
    sigaction(SIGQUIT, &sigAct, NULL);
    
    sigAct.sa_handler = sigUsr1Handler;
    sigaction(SIGUSR1, &sigAct, NULL);
}

// Set INT flag
void sigIntHandler (int sigNum) {
    printf("Caught a SIGINT\n");
    flagInt = 1;
}

// Set QUIT flag
void sigQuitHandler (int sigNum) {
    printf("Caught a SIGQUIT\n");
    flagQuit = 1;
}

// Set USR1 flag
void sigUsr1Handler (int sigNum) {
    printf("Caught a SIGUSR1\n");
    flagUsr1 = 1;
}

// Check if signal flags set
void checkSignalFlags (MonitorDir** monitorDir, int outfd, int bufSize, int bloomSize, char* dir_path, BloomFilter** bloomsHead, State** stateHead, Record** recordsHead, SkipList** skipVaccHead, SkipList** skipNonVaccHead, int* accepted, int* rejected) {
    // SIGINT or SIGQUIT: Print log file
    if (flagQuit == 1 || flagInt == 1) {
        // Create file's name, inside "log_files" dir
        char* dirName = "log_files/";
        char* fileName = "log_file.";
        char* fullName = malloc(sizeof(char)*(strlen(dirName) + strlen(fileName) + LENGTH + 1));
        sprintf(fullName, "%s%s%d", dirName, fileName, (int)getpid());

        // Create file
        int filefd;
        if ( (filefd = creat(fullName, RWE)) == -1) {
            perror("Error with creating log_file");
            exit(1);
        }
        free(fullName);

        // 1st print: this Monitor's countries
        MonitorDir* current = (*monitorDir);
        while (current) {
            // Keep printing to file till all countries written
            if ( (write(filefd, current->country, strlen(current->country)) == -1) ) {
                perror("Error with writing to log_file");
                exit(1);
            }
            if ( (write(filefd, "\n", 1) == -1) ) {
                perror("Error with writing to log_file");
                exit(1);
            }
            current = current->next;
        }

        // 2nd print: total requests
        char* info = "TOTAL TRAVEL REQUESTS ";
        char* numberString = malloc((strlen(info) + LENGTH)*sizeof(char));
        sprintf(numberString, "%s%d\n", info, (*accepted + *rejected));
        if ( (write(filefd, numberString, strlen(numberString)) == -1) ) {
            perror("Error with writing to log_file");
            exit(1);
        }

        // 3rd print: accepted requests
        info = "ACCEPTED ";
        numberString = realloc(numberString, (strlen(info) + LENGTH)*(sizeof(char)) );
        sprintf(numberString, "%s%d\n", info, (*accepted));
        if ( (write(filefd, numberString, strlen(numberString)) == -1) ) {
            perror("Error with writing to log_file");
            exit(1);
        }

        // 4th print: rejected requests
        info = "REJECTED ";
        numberString = realloc(numberString, (strlen(info) + LENGTH)*(sizeof(char)) );
        sprintf(numberString, "%s%d\n", info, (*rejected));
        if ( write(filefd, numberString, strlen(numberString)) == -1 ) {
            perror("Error with writing to log_file");
            exit(1);
        }
        free(numberString);
        
        // Reset flags
        flagQuit = 0;
        flagInt = 0;
    }
    // SIGUSR1: Check for new file
    if (flagUsr1 == 1) {
        // Check directories for new file(s)
        processUsr1(monitorDir, outfd, bufSize, bloomSize, dir_path, bloomsHead, stateHead, recordsHead, skipVaccHead, skipNonVaccHead);
        // Reset flag
        flagUsr1 = 0;
    }
}

// Block signals
void blockSignals (void) {
    sigset_t set;

    // Fill with SIGUSR1, SIGINT, SIGQUIT
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);

    // Block them
    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        perror("Error with setting signal blocks");
        exit(1);
    }
}

// Unblock signals
void unblockSignals (void) {
    sigset_t set;

    // Fill with SIGUSR1, SIGINT, SIGQUIT
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);

    // Unblock them
    if (sigprocmask(SIG_UNBLOCK, &set, NULL) == -1) {
        perror("Error with lifting signal blocks");
        exit(1);
    }
}
