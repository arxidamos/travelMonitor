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

static volatile sig_atomic_t flagQuit = 0;
static volatile sig_atomic_t flagInt = 0;
static volatile sig_atomic_t flagUsr1 = 0;

void sigIntHandler (int sigNum) {
    printf("Caught a SIGINT\n");
    flagInt = 1;
}

void sigQuitHandler (int sigNum) {
    printf("Caught a SIGQUIT\n");
    flagQuit = 1;
}

void sigUsr1Handler (int sigNum) {
    printf("Caught a SIGUSR1\n");
    flagUsr1 = 1;
}

void checkSignalFlags (MonitorDir** monitorDir, int outfd, int bufSize, int bloomSize, char* dir_path, BloomFilter** bloomsHead, State** stateHead, Record** recordsHead, SkipList** skipVaccHead, SkipList** skipNonVaccHead, int* accepted, int* rejected) {
    if (flagQuit == 1 || flagInt == 1) {
        // Write to "log_file.pid"
        printf("TOTAL TRAVEL REQUESTS %d\n", (*accepted + *rejected));
        printf("ACCEPTED %d\nREJECTED %d\n", (*accepted), (*rejected));

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

        int sentSoFar = 0;
        // 1st write this Monitor's countries
        MonitorDir* current = (*monitorDir);
        while (current) {
            // Keep writing to file till all countries written
            if ( (sentSoFar = write(filefd, current->country, strlen(current->country))) == -1) {
                perror("Error with writing to log_file");
                exit(1);
            }
            if ( (sentSoFar = write(filefd, "\n", 1)) == -1) {
                perror("Error with writing to log_file");
                exit(1);
            }
            current = current->next;
        }

        // 2nd write total requests
        char* info = "TOTAL TRAVEL REQUESTS ";
        char* numberString = malloc((strlen(info) + LENGTH)*sizeof(char));
        sprintf(numberString, "%s%d\n", info, (*accepted + *rejected));
        if ( (sentSoFar = write(filefd, numberString, strlen(numberString))) == -1) {
            perror("Error with writing to log_file");
            exit(1);
        }

        // 3rd write accepted
        info = "ACCEPTED ";
        numberString = realloc(numberString, (strlen(info) + LENGTH)*(sizeof(char)) );
        sprintf(numberString, "%s%d\n", info, (*accepted));
        if ( (sentSoFar = write(filefd, numberString, strlen(numberString))) == -1) {
            perror("Error with writing to log_file");
            exit(1);
        }

        // 4th write accepted
        info = "REJECTED ";
        numberString = realloc(numberString, (strlen(info) + LENGTH)*(sizeof(char)) );
        sprintf(numberString, "%s%d\n", info, (*rejected));
        if ( (sentSoFar = write(filefd, numberString, strlen(numberString))) == -1) {
            perror("Error with writing to log_file");
            exit(1);
        }

        free(numberString);
        
        // Reset flags
        flagQuit = 0;
        flagInt = 0;
    }

    if (flagUsr1 == 1) {
        // Check directories for new file(s)
        
        processUsr1(monitorDir, outfd, bufSize, bloomSize, dir_path, bloomsHead, stateHead, recordsHead, skipVaccHead, skipNonVaccHead);
        // sendBytes('-', "", outfd, bufSize);
        printf("Finished actions deriving from SIGUSR1\n");
        // printMonitorDirList(*monitorDir);

        // Reset flag
        flagUsr1 = 0;
    }
}

void handleSignals (void) {
    // struct sigaction sigAct;
        // // Signal set
        // sigset_t set;
        // sigfillset(&set);

        // // Block all signals (that are present in set)
        // if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        //     perror("Error with setting signal mask");
        // }
        // memset(&sigAct, 0, sizeof(struct sigaction));
        // // Initialise flags
        // flagQuit = 0;

        // sigAct.sa_handler = sigQuitHandler;
        // sigaction(SIGQUIT, &sigAct, NULL);

        // // // Add only certain signals in signalSet, will need them (to block when receiving commands)
        // // sigemptyset(&signalSet);
        // // sigaddset(&signalSet, SIGINT);
        // // sigaddset(&signalSet, SIGQUIT);
        // // sigaddset(&signalSet, SIGUSR1);

        // // Unblock all signals
        // sigemptyset(&set);
        // if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        //     perror("Error with setting signal mask");
    // }

    static struct sigaction sigAct;

    sigfillset(&sigAct.sa_mask);
    sigAct.sa_handler = sigIntHandler;
    sigaction(SIGINT, &sigAct, NULL);
    
    sigAct.sa_handler = sigQuitHandler;
    sigaction(SIGQUIT, &sigAct, NULL);
    
    sigAct.sa_handler = sigUsr1Handler;
    sigaction(SIGUSR1, &sigAct, NULL);
    
    // sigAct.sa_handler = sigChldHandler;
    // sigaction(SIGCHLD, &sigAct, NULL);
    
    // sigAct.sa_handler = sigUsr2Handler;
    // sigaction(SIGUSR2, &sigAct, NULL);


}