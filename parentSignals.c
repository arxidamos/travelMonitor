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

// Wait for forked childs (didn't use it)
void waitChildMonitors (Stats* stats, DIR* input_dir, char* dir_path, int bufSize, int bloomSize, int* readyMonitors, int numMonitors, int* readfd, int* writefd, ChildMonitor* childMonitor, int* accepted, int* rejected, BloomFilter* bloomsHead) {
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
            printf("...reaching parent - %lu with return code %d \n",(long)pid, status);
            // Create log file
            createLogFileParent (numMonitors, childMonitor, accepted, rejected);
            // Deallocate memory
            exitApp(stats, input_dir, dir_path, bufSize, bloomSize, readyMonitors, numMonitors, readfd, writefd, childMonitor, accepted, rejected, bloomsHead);
            return;
        }
    }
}

// Install signal handlers
void handleSignalsParent (void) {
    static struct sigaction sigAct;

    sigfillset(&sigAct.sa_mask);
    sigAct.sa_handler = sigUsr1HandlerParent;
    sigaction(SIGUSR1, &sigAct, NULL);

    sigAct.sa_handler = sigIntHandlerParent;
    sigaction(SIGINT, &sigAct, NULL);

    sigAct.sa_handler = sigQuitHandlerParent;
    sigaction(SIGQUIT, &sigAct, NULL);

    sigAct.sa_handler = sigChldHandlerParent;
    sigaction(SIGCHLD, &sigAct, NULL);
}

// Set USR1 flag
void sigUsr1HandlerParent (int sigNum) {
    printf("Parent caught a SIGUSR1\n");
    flagUsr1Parent = 1;
}

// Set INT flag
void sigIntHandlerParent (int sigNum) {
    printf("Parent caught a SIGINT\n");
    flagIntParent = 1;
}

// Set QUIT flag
void sigQuitHandlerParent (int sigNum) {
    printf("Parent caught a SIGQUIT\n");
    flagQuitParent = 1;
}

// Set CHLD flag
void sigChldHandlerParent (int sigNum) {
    printf("Parent caught a SIGCHLD\n");
    flagChldParent = 1;
}

// Check if signal flags set, act accordingly
int checkSignalFlagsParent (Stats* stats, DIR* input_dir, char* dir_path, int bufSize, int bloomSize, int* readyMonitors, int numMonitors, int* readfd, int* writefd, ChildMonitor* childMonitor, int* accepted, int* rejected, BloomFilter* bloomsHead) {
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
            printf("SIGKILL sent to child %d.\n", (int)childMonitor[i].pid);
            kill(childMonitor[i].pid, SIGKILL);
        }

        int status;
        pid_t pid;
        while (1) {
            for (int i = 0; i < numMonitors; ++i) {
                // -1: wait any child to end, status: child's return status, 0: suspend caller
                pid = waitpid(-1, &status, 0);
                if (pid == 0) {
                    return 0;
                }
                else if (pid == -1) {
                    return 0;
                }
                else {
                    printf("Monitor reaching parent - %lu, return code %d \n",(long)pid, status);
                }
            }
            // Create log file
            createLogFileParent (numMonitors, childMonitor, accepted, rejected);
            // Deallocate memory
            exitApp(stats, input_dir, dir_path, bufSize, bloomSize, readyMonitors, numMonitors, readfd, writefd, childMonitor, accepted, rejected, bloomsHead);
            printf("Goodbye!\n");
            return 1;
            
        }
    }
    return 0;
}

// Block signals
void blockSignalsParent (void) {
    sigset_t set;

    // Fill with SIGUSR1, SIGINT, SIGQUIT
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    // sigaddset(&set, SIGCHLD);

    // Block them
    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        perror("Error with setting signal blocks");
        exit(1);
    }
}

// Unblock signals
void unblockSignalsParent (void) {
    sigset_t set;

    // Fill with SIGUSR1, SIGINT, SIGQUIT
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    // sigaddset(&set, SIGCHLD);

    // Unblock them
    if (sigprocmask(SIG_UNBLOCK, &set, NULL) == -1) {
        perror("Error with lifting signal blocks");
        exit(1);
    }
}