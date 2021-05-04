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

    // Create a diretory for the named pipes
    char* fifoPath = "./named_pipes";
    if (mkdir(fifoPath, 0777) == -1) {
        perror("Error creating named_pipes directory");
        exit(1);
    }
    
    // Store the child processes' pids
    pid_t childpids[numMonitors];
    // Parent process' fds for read and write
    int readfd[numMonitors];
    int writefd[numMonitors];
    // Named pipes for read and write
    char pipeParentReads[25];
    char pipeParentWrites[25];
    // Store pids with respective country dirs
    ChildMonitor childMonitor[numMonitors];

    sigchldHandler();

    // Create named pipes and child processes
    for (int i=0; i<numMonitors; i++) {
        // Name them
        sprintf(pipeParentReads, "./named_pipes/readPipe%d", i);
        sprintf(pipeParentWrites, "./named_pipes/writePipe%d", i);
        
        // Create named pipes for read and write
        if (mkfifo(pipeParentReads, 0666) == -1) {
            perror("Error creating named pipe");
            exit(1);
        }
        if (mkfifo(pipeParentWrites, 0666) == -1) {
            perror("Error creating named pipe");
            exit(1);
        }
        printf ("Forking-------------------------------\n");
        // Create child processes
        if ((childpids[i] = fork()) == -1) {
            perror("Error with fork");
            exit(1);
        }
        if (childpids[i] == 0) {
            printf ("Child with ID: %lu and parent ID: %lu.\n", (long)getpid(), (long)getppid());
            // Convert bufSize to string to pass it as arg
            char bufSizeString[15];
            sprintf(bufSizeString, "%d", bufferSize);
            char arithmos[4];
            sprintf(arithmos, "%d", i);
            execl("./child", "child", bufSizeString, pipeParentReads, pipeParentWrites, arithmos, NULL);
            perror("Error with execl");
        }

        // Open reading fifo for each child process
        if ((readfd[i] = open(pipeParentReads, O_RDONLY | O_NONBLOCK)) == -1) {
            perror("Error opening named pipe for reading");
            exit(1);
        }
        if ((writefd[i] = open(pipeParentWrites, O_WRONLY)) == -1) { // O_RDWR | O_NONBLOCK
            perror("Error opening named pipe for writing");
            exit(1);
        }

        // Store pids with respective country dirs
        childMonitor[i].pid = childpids[i];
        childMonitor[i].countryCount = 0;
        printf ("Parent with ID: %lu and child ID: %lu.\n", (long)getpid(), (long)childpids[i]);
    }

    int readyMonitors = 0;
    mapCountryDirs(input_dir, numMonitors, writefd, childMonitor, bufferSize);
    // sendBytes('F', "xesio", writefd[2], bufferSize);
    // while(readyMonitors<numMonitors) {
    //     for (int i=0; i<numMonitors; i++) {
    //         Message* incMessage =
    //         if (read(readfd[i], ))
    //     }
    // }



    // Signal handler for when a child process exits
    pid_t pid;
    // -1: wait for any child process to end, NULL: ignore child's return status, WNOHANG: avoid suspending the caller
    // while ( (pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        //     printf("...reaching the father %lu process \n",(long)pid);
        //     // int index;
        //         // for (int i=0; i<size; i++) {
        //         //     if (childpids[i] == pid) {
        //         //         index = i;
        //         //         break;
        //         //     }
        //         // }

        //         // if ( (childpids[index] = fork()) == -1) {
        //         //     perror("ton poulo");
        //         //     exit(1);
        //         // }
        //         // else if (childpids[index] == 0) {
        //         //     printf ("I am the child process with ID: %lu \n", ( long ) getpid ());
        //         //     exit(0);
        //     // }
    // }
    
    // for (int i=0; i<numMonitors; i++) {
    //     printf("Pid %d: %d\n", i, (int)childMonitor[i].pid);
    // }

    


    fd_set readFds;
    while (1) {


        // Zero the fd_set
        FD_ZERO(&readFds);
        for (int i=0; i<numMonitors; i++) {
            FD_SET(readfd[i], &readFds);
            FD_SET(writefd[i], &readFds);
        }
    
        int retVal = select(FD_SETSIZE, &readFds, NULL, NULL, NULL);
        if (retVal == -1) {
            perror("Error with select");
        }
        if (retVal == 0) {
            // No child process' state has changed
            continue;
        }
        // Iterate over file descriptors to check if inside readFds
        for (int i=0; i<numMonitors; i++) {
            // True if available data in this fd
            if (FD_ISSET(readfd[i], &readFds)) {
                printf("Processing message\n");

                // Read the message
                Message* incMessage = malloc(sizeof(Message));
                getMessage(incMessage, readfd[i], bufferSize);
                printf("parent SELECT FTANOUME???????\n");
                printf("Message received: %s\n", incMessage->body);
                printf("Code received: %c\n", incMessage->code[0]);

                if (incMessage->code[0] == 'R') {
                    readyMonitors++;
                    printf("readymonitors=%d\n", readyMonitors);
                }

                if (readyMonitors == numMonitors){
                    printf("ton poylo ki apo ton pater\n");
                    exit(0);
                }


                free(incMessage->code);
                free(incMessage->body);
                free(incMessage);
            }
            if (FD_ISSET(writefd[i], &readFds)) {
                printf("To parent paei na grapsei se child\n");
            }

        }
    

        // free(dir_path);
        // closedir(input_dir);
        // for (int i=0; i<numMonitors; i++) {
        //     if (close(readfd[i]) == -1) {
        //         perror("Error closing named pipes");
        //     }
        // }

        // for (int i=0; i<numMonitors; i++) {
        //     remove(pipeParentReads);
        //     remove(pipeParentWrites);
        // }
        // fflush(stdout);


        // exit(0);
    }


    // // Unblock signals INT, QUIT, CHLD, USR2    
    // sigprocmask(SIG_UNBLOCK, &cmd_set, NULL);

    // if (WIFEXITED(status)) {
    //         exit_status = WEXITSTATUS(status);
    //         printf("Exit status from %lu was %d\n", (long) childPid, exit_status);
    // }



    // return 0;
}