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

static volatile sig_atomic_t flagQuitParent = 0;
static volatile sig_atomic_t flagIntParent = 0;
static volatile sig_atomic_t flagUsr1Parent = 0;
static volatile sig_atomic_t flagChldParent = 0;

// Wait for forked childs
void waitChildMonitors (void) {
    int status;
    pid_t pid;
    while (1) {
    // -1: wait any child to end, WNOHANG: don't suspend caller
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

// Install signal handlers
void handleSignalsParent (void) {
    static struct sigaction sigAct;

    sigfillset(&sigAct.sa_mask);
    sigAct.sa_handler = sigUsr1HandlerParent;
    sigaction(SIGUSR1, &sigAct, NULL);

    sigAct.sa_handler = sigChldHandlerParent;
    sigaction(SIGCHLD, &sigAct, NULL);

    sigAct.sa_handler = sigIntHandlerParent;
    sigaction(SIGINT, &sigAct, NULL);

    sigAct.sa_handler = sigQuitHandlerParent;
    sigaction(SIGQUIT, &sigAct, NULL);        

}

// Set USR1 flag
void sigUsr1HandlerParent (int sigNum) {
    printf("Caught a SIGUSR1 in parent\n");
    flagUsr1Parent = 1;
}

// Set INT flag
void sigIntHandlerParent (int sigNum) {
    printf("Caught a SIGINT in parent\n");
    flagIntParent = 1;
}

// Set QUIT flag
void sigQuitHandlerParent (int sigNum) {
    printf("Caught a SIGQUIT in parent\n");
    flagQuitParent = 1;
}

// Set CHLD flag
void sigChldHandlerParent(int sigNum) {
    printf("Caught a SIGCHLD in parent\n");
    flagChldParent = 1;
}

// Check if signal flags set, act accordingly
int checkSignalFlagsParent (DIR* input_dir, char* dir_path, int bufSize, int bloomSize, int* readyMonitors, int numMonitors, int* readfd, int* writefd, ChildMonitor* childMonitor, int* accepted, int* rejected, BloomFilter* bloomsHead) {
    // SIGUSR1: Sent by child, await its response
    if (flagUsr1Parent == 1) {
        // Wait for child's message
        (*readyMonitors)--;
        // Reset flag
        flagUsr1Parent = 0;
    }
    // SIGCHLD: Replace child
    if (flagChldParent == 1) {
        int status;
        pid_t pid;
        while (1) {
        // -1: wait any child to end, status: child's return status, WNOHANG: don't suspend caller
            pid = waitpid(-1, &status, WNOHANG);
            if (pid == 0) {
                return 0;
            }
            else if (pid == -1) {
                return 0;
            }
            else {
                // Wait for (new) child's message
                (*readyMonitors)--;
                printf("...reaching parent - %lu  with return code %d \n",(long)pid, status);
                // Create new child Monitor
                replaceChild (pid, dir_path, bufSize, bloomSize, numMonitors, readfd, writefd, childMonitor);
            }
        }
        // Reset flag
        flagChldParent = 0;
    }
    // SIGINT or SIGQUIT: Kill childs, print log file
    if (flagQuitParent == 1 || flagIntParent == 1) {
        // Send SIGKILL signal to every child
        for (int i = 0; i < numMonitors; ++i) {
            printf("SIGKILL sent to child\n");
            kill(childMonitor[i].pid, SIGKILL);
        }

        int status;
        pid_t pid;
        while (1) {
        // -1: wait any child to end, status: child's return status, WNOHANG: don't suspend caller
            pid = waitpid(-1, &status, WNOHANG);
            if (pid == 0) {
                return 0;
            }
            else if (pid == -1) {
                return 0;
            }
            else {
                printf("...reaching parent - %lu  with return code %d \n",(long)pid, status);
                // Create log file
                createLogFileParent (numMonitors, childMonitor, accepted, rejected);
                // Deallocate memory
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
        }
    }
    return 0;
}
