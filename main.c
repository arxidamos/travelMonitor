#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "structs.h"
#include "functions.h"

int main(int argc, char **argv) {
    int numMonitors;
    int bufferSize;
    int bloomSize;
    char* dir_path;
    DIR* input_dir;

    // Install signal handler
    blockSignalsParent();
    handleSignalsParent();

	// Scan command line arguments
    if (argc != 9) {
        fprintf(stderr, "Error: 8 parameters are required.\n");
        exit(EXIT_FAILURE);
    }
	for (int i=0; i<argc; ++i) {
        if (!strcmp("-m", argv[i])) {
            numMonitors = atoi(argv[i+1]);
            if (numMonitors <= 0) {
				fprintf(stderr, "Error: Number of Monitors must be a positive number.\n");
				exit(EXIT_FAILURE);
            }
        }
        else if (!strcmp("-b", argv[i])) {
            bufferSize = atoi(argv[i+1]);
            if (bufferSize <= 0) {
				fprintf(stderr, "Error: Size of buffer must be a positive number.\n");
				exit(EXIT_FAILURE);
            }
        }
		else if (!strcmp("-s", argv[i])) {
			bloomSize = atoi(argv[i+1]);
			if (bloomSize <= 0) {
				fprintf(stderr, "Error: Bloom filter size must be a positive number.\n");
				exit(EXIT_FAILURE);
			}
		}
		else if (!strcmp("-i", argv[i])) {
            dir_path = malloc(strlen(argv[i+1]) + 1);
            strcpy(dir_path, argv[i+1]);
            input_dir = opendir(dir_path);
            if(input_dir == NULL) {
                fprintf(stderr, "Cannot open directory: %s\n", argv[i+1]);
                return 1;
            }
        }
	}

    // Create directory for the named pipes
    if (mkdir("./named_pipes", RWE) == -1) {
        perror("Error creating named_pipes directory");
        exit(1);
    }

    // Create directory for the log file
    if (mkdir("./log_files", RWE) == -1) {
        perror("Error creating log_files directory");
        exit(1);
    }

    // Child processes' pids
    pid_t childpids[numMonitors];
    // Parent process' fds for read and write
    int readfd[numMonitors];
    int writefd[numMonitors];
    // Named pipes for read and write
    char pipeParentReads[25];
    char pipeParentWrites[25];
    // Store pids with respective country dirs
    ChildMonitor childMonitor[numMonitors];

    // Create named pipes and child processes
    for (int i=0; i<numMonitors; i++) {
        // Name and create named pipes for read and write
        sprintf(pipeParentReads, "./named_pipes/readPipe%d", i);
        sprintf(pipeParentWrites, "./named_pipes/writePipe%d", i);
        if (mkfifo(pipeParentReads, RW) == -1) {
            perror("Error creating named pipe");
            exit(1);
        }
        if (mkfifo(pipeParentWrites, RW) == -1) {
            perror("Error creating named pipe");
            exit(1);
        }
        // Create child processes
        if ((childpids[i] = fork()) == -1) {
            perror("Error with fork");
            exit(1);
        }
        // Child executes "child" program
        if (childpids[i] == 0) {
            execl("./child", "child", pipeParentReads, pipeParentWrites, dir_path, NULL);
            perror("Error with execl");
        }
        // Open reading & writing fds for each child process
        if ((readfd[i] = open(pipeParentReads, O_RDONLY)) == -1) {
            perror("Error opening named pipe for reading");
            exit(1);
        }
        if ((writefd[i] = open(pipeParentWrites, O_WRONLY)) == -1) {
            perror("Error opening named pipe for writing");
            exit(1);
        }
        // Store pids with respective country dirs
        childMonitor[i].pid = childpids[i];
        childMonitor[i].countryCount = 0;
    }

    // Var to monitor if child is about to send message
    int readyMonitors = 0;
    // Vars for stats
    int accepted =0;
    int rejected = 0;
    Stats stats;
    initStats(&stats);
    // Initialise structure
    BloomFilter* bloomsHead = NULL;

    // Convert bufSize and bloomSize to strings
    char bufSizeString[15];
    sprintf(bufSizeString, "%d", bufferSize);
    char bloomSizeString[15];
    sprintf(bloomSizeString, "%d", bloomSize);
    // Send bufSize and bloomSize as first two messages
    for (int i=0; i<numMonitors; i++) {
        sendMessage ('1', bufSizeString, writefd[i], bufferSize);
        sendMessage ('2', bloomSizeString, writefd[i], bufferSize);
    }

    // Assign countries to each Monitor, round-robin
    mapCountryDirs(dir_path, numMonitors, writefd, childMonitor, bufferSize);
    for (int i=0; i<numMonitors; i++) {
        printf("Monitor %d with countries:", i);
        for (int j=0; j<childMonitor[i].countryCount; j++) {
            printf(" %s,", childMonitor[i].country[j]);
        }
        printf("\n");
    }

    // Check any blocked messages
    unblockSignalsParent();
    if (checkSignalFlagsParent(&stats, input_dir, dir_path, bufferSize, bloomSize, &readyMonitors, numMonitors, readfd, writefd, childMonitor, &accepted, &rejected, bloomsHead) == 1) {
        exit(0);
    }

    fd_set incfds;
    
    while (1) {
        // Monitor(s) about to send message
        if (readyMonitors < numMonitors) {
            // Zero the fd_set
            FD_ZERO(&incfds);
            for (int i=0; i<numMonitors; i++) {
                FD_SET(readfd[i], &incfds);
            }
            // Select() on incfds
            int retVal;
            if ( (retVal = select(FD_SETSIZE, &incfds, NULL, NULL, NULL)) == -1) {
                perror("Error with select");
            }
            if (retVal == 0) {
                // No child process' state has changed
                continue;
            }
            // Iterate over fds to check if inside readFds
            for (int i=0; i<numMonitors; i++) {
                // Check if available data in this fd
                if (FD_ISSET(readfd[i], &incfds)) {
                    // Read incoming messages
                    Message* incMessage = malloc(sizeof(Message));
                    getMessage(incMessage, readfd[i], bufferSize);
                    
                    // Decode incoming messages
                    analyseChildMessage(readfd, incMessage, childMonitor, numMonitors, &readyMonitors, writefd, bufferSize, &bloomsHead, bloomSize, &accepted, &rejected, &stats);

                    FD_CLR(readfd[i], &incfds);
                    free(incMessage->code);
                    free(incMessage->body);
                    free(incMessage);
                }
            }
        }
        // Monitors ready. Receive user queries
        else {
            printf("Type a command:\n");
            int userCommand = getUserCommand(&stats, &readyMonitors, numMonitors, childMonitor, bloomsHead, dir_path, input_dir, readfd, writefd, bufferSize, bloomSize, &accepted, &rejected);
            // Command is NULL
            if (userCommand == -1) {
                fflush(stdin);
                continue;
            }
            // Command is /exit
            else if (userCommand == 1) {
                exit(0);
            }
        }
    }
}