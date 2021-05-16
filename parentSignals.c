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
        // -1: wait any child to end, NULL: ignore child's return status, WNOHANG: don't suspend caller
            pid = waitpid(-1, &status, WNOHANG);
            if (pid == 0) {
                return 0;
            }
            else if (pid == -1) {
                return 0;
            }
            else {
                (*readyMonitors)--;
                printf("...reaching parent - %lu  with return code %d \n",(long)pid, status);
                // Find the terminated child in parent's structure
                int i=0;
                for (i=0; i<numMonitors; i++) {
                    if (pid == childMonitor[i].pid) {
                        break;
                    }
                }

                // Close reading & writing fds for child
                if ( close(readfd[i]) == -1 ) {
                    perror("Error closing named pipe for reading");
                    exit(1);
                }
                if ( close(writefd[i]) == -1 ) {
                    perror("Error closing named pipe for writing");
                    exit(1);
                }
            

                // Name and create named pipes for read and write
                char pipeParentReads[25];
                char pipeParentWrites[25];
                sprintf(pipeParentReads, "./named_pipes/readPipe%d", i);
                sprintf(pipeParentWrites, "./named_pipes/writePipe%d", i);

                // Remove reading & writing named pipes for child
                if ( unlink(pipeParentReads) == -1 ) {
                    perror("Error deleting named pipe for reading");
                    exit(1);
                }
                if ( unlink(pipeParentWrites) == -1 ) {
                    perror("Error deleting named pipe for writing");
                    exit(1);
                }

                if (mkfifo(pipeParentReads, RW) == -1) {
                    perror("Error creating named pipe");
                    exit(1);
                }
                if (mkfifo(pipeParentWrites, RW) == -1) {
                    perror("Error creating named pipe");
                    exit(1);
                }

                // Create new child process
                if ((pid = fork()) == -1) {
                    perror("Error with fork");
                    exit(1);
                }
                // Child executes "child" program
                if (pid == 0) {
                    execl("./child", "child", pipeParentReads, pipeParentWrites, dir_path, NULL);
                    perror("Error with execl");
                }
                // Open reading & writing named pipes for child
                if ((readfd[i] = open(pipeParentReads, O_RDONLY)) == -1) {
                    perror("Error opening named pipe for reading");
                    exit(1);
                }
                if ((writefd[i] = open(pipeParentWrites, O_WRONLY)) == -1) {
                    perror("Error opening named pipe for writing");
                    exit(1);
                }

                // Update pid in parent's structure
                childMonitor[i].pid = pid;

                // Convert bufSize and bloomSize to strings
                char bufSizeString[15];
                sprintf(bufSizeString, "%d", bufSize);
                char bloomSizeString[15];
                sprintf(bloomSizeString, "%d", bloomSize);
                // Send bufSize and bloomSize as first two messages
                sendBytes ('1', bufSizeString, writefd[i], bufSize);
                sendBytes ('2', bloomSizeString, writefd[i], bufSize);

                // Assign the same countires to new Monitor
                resendCountryDirs(dir_path, numMonitors, writefd[i], childMonitor[i], bufSize);
                for (int i=0; i<numMonitors; i++) {
                    printf(" New=> Monitor %d with countries:", i);
                    for (int j=0; j<childMonitor[i].countryCount; j++) {
                        printf(" %s,", childMonitor[i].country[j]);
                    }
                    printf("\n");
                }

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