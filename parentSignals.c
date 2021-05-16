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

static volatile sig_atomic_t flagQuitParent = 0;
static volatile sig_atomic_t flagIntParent = 0;
static volatile sig_atomic_t flagUsr1Parent = 0;

void sigUsr1HandlerParent (int sigNum) {
    printf("sigUsr1 handler caught a SIGUSR1 in parent\n");
    flagUsr1Parent = 1;
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


int checkSignalFlagsParent (int* readyMonitors) {
    // SIGUSR1 received by child, await its response
    if (flagUsr1Parent == 1) {
        printf("checkSIgnalParent prints: Child is gonna send message\n");
        (*readyMonitors)--;

        // Reset the flag
        flagUsr1Parent = 0;
        return 1;
    }
    return 0;
}

void handleSignalsParent (void) {
    static struct sigaction sigAct;

    sigfillset(&sigAct.sa_mask);
    sigAct.sa_handler = sigUsr1HandlerParent;
    sigaction(SIGUSR1, &sigAct, NULL);

}