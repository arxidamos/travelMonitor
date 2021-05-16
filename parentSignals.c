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

void sigUsr1HandlerParent (int sigNum) {
    printf("sigUsr1 handler caught a SIGUSR1 in parent\n");
    flagUsr1Parent = 1;
}

void sigChldHandlerParent(int sigNum) {
    printf("sigChld handler caught a SIGCHLD in parent\n");
    flagChldParent = 1;
}

void sigchldHandler () {
    int status;
    pid_t pid;
    while (1) {
    // -1: wait any child to end, NULL: ignore child's return status, WNOHANG: don't suspend caller
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

// Check if signal flags set
int checkSignalFlagsParent (char* dir_path, int bufSize, int bloomSize, int* readyMonitors, int numMonitors, int* readfd, int* writefd, ChildMonitor* childMonitor) {
    // SIGUSR1: Sent by child, await its response
    if (flagUsr1Parent == 1) {
        // Wait for child's message
        (*readyMonitors)--;
        // Reset flag
        flagUsr1Parent = 0;
        return 1;
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

                replaceChild (pid, dir_path, bufSize, bloomSize, numMonitors, readfd, writefd, childMonitor);

            }
        }

        // Reset flag
        flagChldParent = 0;
    }
    return 0;
}

// Instal signal handlers
void handleSignalsParent (void) {
    static struct sigaction sigAct;

    sigfillset(&sigAct.sa_mask);
    sigAct.sa_handler = sigUsr1HandlerParent;
    sigaction(SIGUSR1, &sigAct, NULL);

    sigAct.sa_handler = sigChldHandlerParent;
    sigaction(SIGCHLD, &sigAct, NULL);

}