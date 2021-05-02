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
    // INSTALL SIGNAL HANDLER, px Signal(SIGCHLD, sigchd_handler);
    // TO-DO block signals during command process, block signals during setup?
    // TO-DO Structures for Monitors?

    // Create a diretory for the named pipes
    char* fifoPath = "./named_pipes";
    // if (access(dir_path, F_OK) == 0)  // If dir already exists (due to abnormal previous termination, eg: SIGKILL)
    // delete_flat_dir(dir_path);      // remove it
    if (mkdir(fifoPath, 0777) == -1) {
        perror("Error creating named_pipes directory");
        exit(1);
    }
    
    // Store the child processes' pids
    pid_t childpids[numMonitors];
    // Parent process' fds for read and write
    int readfd[numMonitors];
    int writefd[numMonitors];
    char pipeParentReads[25];
    char pipeParentWrites[25];

    // Create named pipes and child processes
    for (int i=0; i<numMonitors; i++) {

        // Parent process' read and write ends

        // Name them
        sprintf(pipeParentReads, "./named_pipes/readPipe%d", i);
        sprintf(pipeParentWrites, "./named_pipes/writePipe%d", i);
        
        printf ("Making named pipes %s and %s\n", pipeParentReads, pipeParentWrites);

        // Create named pipes for read and write
        if (mkfifo(pipeParentReads, 0666) == -1) {
            perror("Error creating named pipe");
            exit(1);
        }
        if (mkfifo(pipeParentWrites, 0666) == -1) {
            perror("Error creating named pipe");
            exit(1);
        }

        // Open reading fifo for each child process
        if ((readfd[i] = open(pipeParentReads, O_RDWR)) == -1) {
            perror("Error opening named pipe for reading");
            exit(1);
        }
        if ((writefd[i] = open(pipeParentWrites, O_RDWR | O_NONBLOCK)) == -1) {
            perror("Error opening named pipe for writing");
            exit(1);
        }

        printf ("Forking...\n------------------\n");
        // Create child processes
        if ((childpids[i] = fork()) == -1) {
            perror("Error with fork");
            exit(1);
        }
        if (childpids[i] == 0) {
            printf ("I am the child process with ID: %lu and parent ID: %lu.\n", (long)getpid(), (long)getppid());
            // Convert bufSize to string to pass it as arg
            char bufSizeString[15];
            // char* bufSizeString = malloc (sizeof(char)* )
            sprintf(bufSizeString, "%d", bufferSize);
            char arithmos[4];
            sprintf(arithmos, "%d", i);
            execl("./child", "child", bufSizeString, pipeParentReads, arithmos, NULL);
            // execl("./child", "child", bufSizeString, pipeParentReads, NULL);

            perror("Error with execl");
        }
        printf ("I am the parent process with ID: %lu and child ID: %lu.\n", (long)getpid(), (long)childpids[i]);
    }

    
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
    while ((pid = waitpid(-1, NULL, 0)) > 0) {

            printf("...reaching the father %lu process \n",(long)pid);
    }


    fd_set readFds;
    while (1) {
        // Zero the fd_set
        FD_ZERO(&readFds);
        for (int i=0; i<numMonitors; i++) {
            FD_SET(readfd[i], &readFds);
        }

        // int maxfd;
        // for (int i=0; i<numMonitors-1; i++) {
        //     maxfd = parentRead_fd[i] > parentRead_fd[i+1] ?  parentRead_fd[i] : parentRead_fd[i+1];
        // }
        
        int retVal = select(FD_SETSIZE, &readFds, NULL, NULL, NULL);
        if (retVal == -1) {
            perror("Error with select");
        }
        if (retVal == 0) {
            // No child process' state has changed
            continue;
        }
        for (int i=0; i<numMonitors; i++) {
            // True if available data in this fd
            if (FD_ISSET(readfd[i], &readFds)) {
                printf("Processing message\n");
                // Read the message
                char inbuf[bufferSize];
                memset(inbuf, 0, bufferSize);
                // Fist get the header, containing the length of the actual message
                char* header = calloc((MAX_DIGITS + 1), sizeof(char));
                header = readBytes(header, MAX_DIGITS, readfd[i], inbuf, bufferSize);
                header[strlen(header)] = '\0';
                printf("Length as a string is:%s\n", header);
                int length = atoi(header+1);
                printf("Length as an int is:%d\n", length);
                // Then get the actual message
                char* message = calloc( (length+1), sizeof(char));
                message = readBytes(message, length, readfd[i], inbuf, bufferSize);
                message[length] = '\0';

                printf("Message received: %s\n", message);
                free(header);
                free(message);
            }
        }
        free(dir_path);
        closedir(input_dir);
        for (int i=0; i<numMonitors; i++) {
            if (close(readfd[i]) == -1) {
                perror("Error closing named pipes");
            }
        }

        for (int i=0; i<numMonitors; i++) {
            remove(pipeParentReads);
            remove(pipeParentWrites);
        }
        fflush(stdout);


        exit(0);
    }


    // // Unblock signals INT, QUIT, CHLD, USR2    
    // sigprocmask(SIG_UNBLOCK, &cmd_set, NULL);

    // if (WIFEXITED(status)) {
    //         exit_status = WEXITSTATUS(status);
    //         printf("Exit status from %lu was %d\n", (long) childPid, exit_status);
    // }

    // Initialize variables



    // size_t textSize;
    // char* text = NULL;
    // char* token = NULL;
    // char* citizenID;
    // char* fName;
    // char* lName;
    // char* country;
    // int age;
    // char* virus;
    // Date vaccDate;
    // State* stateHead = NULL;
    // State* state;
    // Record* recordsHead = NULL;
    // Record* record;
    // BloomFilter* bloomsHead = NULL;
    // int k =16;
    // int recordAccepted = 1;
    // SkipList* skipVaccHead = NULL;
    // SkipList* skipNonVaccHead = NULL;
    // // Seed the time generator
    // srand(time(NULL));

    // Read from inputFile and create structs
    // while (getline(&text, &textSize, inputFile) != -1) {
        //     // Record to be inserted in structs, unless faulty
        //     recordAccepted = 1;

        //     // Get citizenID
        //     token = strtok(text, " ");
        //     citizenID = malloc(strlen(token)+1);
        //     strcpy(citizenID, token);

        //     // Get firstName
        //     token = strtok(NULL, " ");
        //     fName = malloc(strlen(token)+1);
        //     strcpy(fName, token);

        //     // Get lastName
        //     token = strtok(NULL, " ");
        //     lName = malloc(strlen(token)+1);
        //     strcpy(lName, token);

        //     // Get country
        //     token = strtok(NULL, " ");
        //     country = malloc(strlen(token)+1);
        //     strcpy(country, token);

        //     // Get age
        //     token = strtok(NULL, " ");
        //     sscanf(token, "%d", &age);

        //     // Get virus
        //     token = strtok(NULL, " ");
        //     virus = malloc(strlen(token)+1);
        //     strcpy(virus, token);

        //     // Check if YES/NO
        //     token = strtok(NULL, " ");

        //     // Get vaccine date, if YES
        //     if(!strcmp("YES", token)){
        //         token = strtok(NULL, " ");
        //         sscanf(token, "%d-%d-%d", &vaccDate.day, &vaccDate.month, &vaccDate.year);
        //     }
        //     else {
        //         // If NO, then record should have no date
        //         if (token = strtok(NULL, " ")) {
        //             printf("ERROR IN RECORD %s %s %s %s %d %s - Has 'NO' with date %s", citizenID, fName, lName, country, age, virus, token);
        //             recordAccepted = 0;
        //         }
        //         else {
        //             vaccDate.day=0;
        //             vaccDate.month=0;
        //             vaccDate.year=0;
        //         }
        //     }

        //     // Insert records into structures
        //     if (recordAccepted) {
        //         // Add country in State linked list
        //         // Add each country once only
        //         if (!stateExists(stateHead, country)) {
        //             state = insertState(&stateHead, country);
        //         }
        //         else {
        //             state = stateExists(stateHead, country);
        //         }

        //         // Add record in Record linked list
        //         // Check if record is unique
        //         int check = checkDuplicate(recordsHead, citizenID, fName, lName, state, age, virus);
        //         // Record is unique
        //         if (!check) {
        //             record = insertSortedRecord(&recordsHead, citizenID, fName, lName, state, age, virus);
        //             recordAccepted = 1;
        //         }
        //         // Record exists for different virus
        //         else if (check == 2) {
        //             record = insertVirusOnly(&recordsHead, citizenID, virus);
        //             recordAccepted = 1;
        //         }
        //         // Record exists for the same virus
        //         else if (check == 1) {
        //             recordAccepted = 0;
        //         }
        //     }

        //     if (recordAccepted) {
        //         // Add vaccinated record in Bloom Filter
        //         if (vaccDate.year != 0) {
        //             // Check if we have Bloom Filter for this virus
        //             if (virusBloomExists(bloomsHead, virus)) {
        //                 insertInBloom(bloomsHead, citizenID, virus);
        //             }
        //             // Create new Bloom Filter for this virus
        //             else {
        //                 bloomsHead = createBloom(bloomsHead, virus, bloomSize, k);
        //                 insertInBloom(bloomsHead, citizenID, virus);
        //             }
        //         }      

        //         // Add record in Skip List
        //         // Separate structure for vaccined Skip Lists
        //         if (vaccDate.year != 0) {
        //             if (virusSkipExists(skipVaccHead, virus)) {
        //                 insertInSkip(skipVaccHead, record, virus, vaccDate);
        //             }
        //             else {
        //                 skipVaccHead = createList(skipVaccHead, virus);
        //                 insertInSkip(skipVaccHead, record, virus, vaccDate);
        //             }
        //         }
        //         // Separate structure for non-vaccined Skip Lists
        //         else {
        //             if (virusSkipExists(skipNonVaccHead, virus)) {
        //                 insertInSkip(skipNonVaccHead, record, virus, vaccDate);
        //             }
        //             else {
        //                 skipNonVaccHead = createList(skipNonVaccHead, virus);
        //                 insertInSkip(skipNonVaccHead, record, virus, vaccDate);
        //             }
                    
        //         }
        //     }
        //     free(citizenID);
        //     free(fName);
        //     free(lName);
        //     free(country);
        //     free(virus);
    // }

    // printStateList(stateHead);
    // printRecordsList(recordsHead);
    // printBloomsList(bloomsHead);
    // printSkipLists(skipVaccHead);
    // printf("---------------------\n");
    // printSkipLists(skipNonVaccHead);

    // Prepare for receiving commands 
    // size_t inputSize;
    // char* input = NULL;
    // char* command = NULL;
    // printf("Structs have been constructed. Type a command:\n");

    // while (1) {
        //     getline(&input, &inputSize, stdin);
        //     input[strlen(input)-1] = '\0'; // Cut terminating '\n' from string
            
        //     // Get the command
        //     command = strtok(input, " ");

        //     // /vaccineStatusBloom citizenID virusName 
        //     if (!strcmp(command, "/vaccineStatusBloom")) {
        //         // Get citizenID
        //         command = strtok(NULL, " ");
        //         if (command) {
        //             citizenID = malloc(strlen(command)+1);
        //             strcpy(citizenID, command);

        //             // Get virusName
        //             command = strtok(NULL, " ");
        //             if(command) {
        //                 // Check that virus exists in our data
        //                 if (virusSkipExists(skipVaccHead, command)) {
        //                     virus = malloc(strlen(command)+1);
        //                     strcpy(virus, command);
        //                     vaccineStatusBloom(bloomsHead, citizenID, virus);
        //                     free(virus);
        //                 }
        //                 else {
        //                     printf("Please enter an existing virus name\n");
        //                 }
        //             }
        //             else {
        //                 printf("Please enter a virus name\n");
        //             }
        //             free(citizenID);
        //         }
        //         else {
        //             printf("Please enter a citizenID and a virus name\n");
        //         }
        //     }
        //     // /vaccineStatus citizenID [virusName]
        //     else if (!strcmp(command, "/vaccineStatus")) { 
        //         // Get cizitenID
        //         command = strtok(NULL, " ");
        //         if (command) {
        //             citizenID = malloc(strlen(command)+1);
        //             strcpy(citizenID, command);

        //             // Check that citizenID is valid
        //             if (checkExistence(recordsHead, citizenID)) {
        //                 // Get virusName
        //                 command = strtok(NULL, " ");
        //                 // Call vaccineStatus function
        //                 if (command) {
        //                     // Check that virus exists in our data
        //                     if (virusSkipExists(skipVaccHead, command)) {
        //                         virus = malloc(strlen(command)+1);
        //                         strcpy(virus, command);
        //                         vaccineStatus(skipVaccHead, citizenID, virus);
        //                         free(virus);
        //                     }
        //                     else {
        //                         printf("Please enter an existing virus name\n");
        //                     }
        //                 }
        //                 // Call vaccineStatusAll function
        //                 else {
        //                     vaccineStatusAll(skipVaccHead, citizenID);
        //                     vaccineStatusAll(skipNonVaccHead, citizenID);
        //                 }
        //             }
        //             else {
        //                 printf("Please enter a valid citizenID\n");
        //             }
        //             free(citizenID);
        //         }
        //         else {
        //             printf("Please enter a citizenID\n");
        //         }
        //     }
        //     // /populationStatus [country] virusName date1 date2
        //     else if (!strcmp(command, "/populationStatus")) {
        //         // Get country OR virusName
        //         command = strtok(NULL, " ");
        //         if (command) {
        //             // 1st argument: existing country
        //             if (stateExists(stateHead, command)){
        //                 country = malloc(strlen(command)+1);
        //                 strcpy(country, command);

        //                 // Get virusName
        //                 command = strtok(NULL, " ");
        //                 if (command) {
        //                     // 2nd argument: existing virusName
        //                     if (virusSkipExists(skipVaccHead, command) || virusSkipExists(skipNonVaccHead, command)) {
        //                         virus = malloc(strlen(command)+1);
        //                         strcpy(virus, command);
                        
        //                         Date date1;
        //                         Date date2;

        //                         // Get date1
        //                         command = strtok(NULL, " ");
        //                         if (command) {
        //                             // 3rd argument: date1
        //                             sscanf(command, "%d-%d-%d", &date1.day, &date1.month, &date1.year);

        //                             // Get date2
        //                             command = strtok(NULL, " ");
        //                             if (command) {
        //                                 // 4rth argument: date2
        //                                 sscanf(command, "%d-%d-%d", &date2.day, &date2.month, &date2.year);

        //                                 // Call function
        //                                 SkipList* vaccSkipList = virusSkipExists(skipVaccHead, virus);
        //                                 SkipList* nonVaccSkipList = virusSkipExists(skipNonVaccHead, virus);
        //                                 populationStatus(vaccSkipList, nonVaccSkipList, country, stateHead, date1, date2);
        //                             }
        //                             // No date2, exit command
        //                             else {
        //                                 printf("ERROR:\nNo second date typed. Please submit the command again.\n");
        //                             }
        //                         }
        //                         // No 3rd and 4th arguments
        //                         else {
        //                             // Min & max values to include all possible dates
        //                             date1.day = -1;
        //                             date1.month = -1;
        //                             date1.year = -1;
        //                             date2.day = 99;
        //                             date2.month = 99;
        //                             date2.year = 9999;

        //                             // Call function
        //                             SkipList* vaccSkipList = virusSkipExists(skipVaccHead, virus);
        //                             SkipList* nonVaccSkipList = virusSkipExists(skipNonVaccHead, virus);
        //                             populationStatus(vaccSkipList, nonVaccSkipList, country, stateHead, date1, date2);
        //                         }
        //                         free(virus);
        //                     }
        //                     else {
        //                         printf("Please enter an existing virus name\n");
        //                     }
        //                 }
        //                 else {
        //                     printf("Please enter a virus name\n");
        //                 }
        //                 free(country);
        //             }
        //             // 1st argument: existing virusName
        //             else if (virusSkipExists(skipVaccHead, command) || virusSkipExists(skipNonVaccHead, command)) {
        //                 // No optional country argument passed
        //                 virus = malloc(strlen(command)+1);
        //                 strcpy(virus, command);

        //                 Date date1;
        //                 Date date2;
        //                 country = NULL;

        //                 // Get date1
        //                 command = strtok(NULL, " ");
        //                 if (command) {
        //                     // 3rd argument: date1
        //                     sscanf(command, "%d-%d-%d", &date1.day, &date1.month, &date1.year);

        //                     // Get date2
        //                     command = strtok(NULL, " ");
        //                     if (command) {
        //                         // 4rth argument: date2
        //                         sscanf(command, "%d-%d-%d", &date2.day, &date2.month, &date2.year);

        //                         // Call function
        //                         SkipList* vaccSkipList = virusSkipExists(skipVaccHead, virus);
        //                         SkipList* nonVaccSkipList = virusSkipExists(skipNonVaccHead, virus);
        //                         populationStatus(vaccSkipList, nonVaccSkipList, country, stateHead, date1, date2);
        //                       }
        //                     // No date2, exit command
        //                     else {
        //                         printf("ERROR:\nNo second date typed. Please submit the command again.\n");
        //                     }
        //                 }
        //                 // No 3rd and 4th arguments
        //                 else {
        //                     // Min & max values to include all possible dates
        //                     date1.day = 0;
        //                     date1.month = 0;
        //                     date1.year = 0;
        //                     date2.day = 99;
        //                     date2.month = 99;
        //                     date2.year = 9999;

        //                     // Call function
        //                     SkipList* vaccSkipList = virusSkipExists(skipVaccHead, virus);
        //                     SkipList* nonVaccSkipList = virusSkipExists(skipNonVaccHead, virus);
        //                     populationStatus(vaccSkipList, nonVaccSkipList, country, stateHead, date1, date2);
        //                 }

        //                 free(virus);
        //             }
        //             // 1st argument: neither country nor virus
        //             else {
        //                 printf("Please enter an existing country or virus name\n");
        //             }
        //         }
        //         else {
        //             printf("More arguments needed. Proper command structure:\n");
        //             printf("** /populationStatus [country] virusName date1 date2 **\n");
        //         }
        //     }
        //     // /popStatusByAge [country] virusName date1 date2
        //     else if (!strcmp(command, "/popStatusByAge")) {
        //         // Get country OR virusName
        //         command = strtok(NULL, " ");
        //         if (command) {
        //             // 1st argument: existing country
        //             if (stateExists(stateHead, command)){
        //                 country = malloc(strlen(command)+1);
        //                 strcpy(country, command);

        //                 // Get virusName
        //                 command = strtok(NULL, " ");
        //                 if (command) {
        //                     // 2nd argument: existing virusName
        //                     if (virusSkipExists(skipVaccHead, command) || virusSkipExists(skipNonVaccHead, command)) {
        //                         virus = malloc(strlen(command)+1);
        //                         strcpy(virus, command);
                        
        //                         Date date1;
        //                         Date date2;

        //                         // Get date1
        //                         command = strtok(NULL, " ");
        //                         if (command) {
        //                             // 3rd argument: date1
        //                             sscanf(command, "%d-%d-%d", &date1.day, &date1.month, &date1.year);

        //                             // Get date2
        //                             command = strtok(NULL, " ");
        //                             if (command) {
        //                                 // 4rth argument: date2
        //                                 sscanf(command, "%d-%d-%d", &date2.day, &date2.month, &date2.year);

        //                                 // Call function
        //                                 SkipList* vaccSkipList = virusSkipExists(skipVaccHead, virus);
        //                                 SkipList* nonVaccSkipList = virusSkipExists(skipNonVaccHead, virus);
        //                                 popStatusByAge(vaccSkipList, nonVaccSkipList, country, stateHead, date1, date2);
        //                             }
        //                             // No date2, exit command
        //                             else {
        //                                 printf("ERROR:\nNo second date typed. Please submit the command again.\n");
        //                             }
        //                         }
        //                         // No 3rd and 4th arguments
        //                         else {
        //                             // Min & max values to include all possible dates
        //                             date1.day = -1;
        //                             date1.month = -1;
        //                             date1.year = -1;
        //                             date2.day = 99;
        //                             date2.month = 99;
        //                             date2.year = 9999;

        //                             // Call function
        //                             SkipList* vaccSkipList = virusSkipExists(skipVaccHead, virus);
        //                             SkipList* nonVaccSkipList = virusSkipExists(skipNonVaccHead, virus);
        //                             popStatusByAge(vaccSkipList, nonVaccSkipList, country, stateHead, date1, date2);
        //                         }
        //                         free(virus);
        //                     }
        //                     else {
        //                         printf("Please enter an existing virus name\n");
        //                     }
        //                 }
        //                 else {
        //                     printf("Please enter a virus name\n");
        //                 }
        //                 free(country);
        //             }
        //             // 1st argument: existing virusName
        //             else if (virusSkipExists(skipVaccHead, command) || virusSkipExists(skipNonVaccHead, command)) {
        //                 // No optional country argument passed
        //                 virus = malloc(strlen(command)+1);
        //                 strcpy(virus, command);

        //                 Date date1;
        //                 Date date2;
        //                 country = NULL;

        //                 // Get date1
        //                 command = strtok(NULL, " ");
        //                 if (command) {
        //                     // 3rd argument: date1
        //                     sscanf(command, "%d-%d-%d", &date1.day, &date1.month, &date1.year);

        //                     // Get date2
        //                     command = strtok(NULL, " ");
        //                     if (command) {
        //                         // 4rth argument: date2
        //                         sscanf(command, "%d-%d-%d", &date2.day, &date2.month, &date2.year);

        //                         // Call function
        //                         SkipList* vaccSkipList = virusSkipExists(skipVaccHead, virus);
        //                         SkipList* nonVaccSkipList = virusSkipExists(skipNonVaccHead, virus);
        //                         popStatusByAge(vaccSkipList, nonVaccSkipList, country, stateHead, date1, date2);
        //                       }
        //                     // No date2, exit command
        //                     else {
        //                         printf("ERROR:\nNo second date typed. Please submit the command again.\n");
        //                     }
        //                 }
        //                 // No 3rd and 4th arguments
        //                 else {
        //                     // Min & max values to include all possible dates
        //                     date1.day = 0;
        //                     date1.month = 0;
        //                     date1.year = 0;
        //                     date2.day = 99;
        //                     date2.month = 99;
        //                     date2.year = 9999;

        //                     // Call function
        //                     SkipList* vaccSkipList = virusSkipExists(skipVaccHead, virus);
        //                     SkipList* nonVaccSkipList = virusSkipExists(skipNonVaccHead, virus);
        //                     popStatusByAge(vaccSkipList, nonVaccSkipList, country, stateHead, date1, date2);
        //                 }

        //                 free(virus);
        //             }
        //             // 1st argument: neither country nor virus
        //             else {
        //                 printf("Please enter an existing country or virus name\n");
        //             }
        //         }
        //         else {
        //             printf("More arguments needed. Proper command structure:\n");
        //             printf("** /popStatusByAge [country] virusName date1 date2 **\n");
        //         }
        //     }
        //     // /insertCitizenRecord citizenID firstName lastName country age virusName YES/NO [date]
        //     else if (!strcmp(command, "/insertCitizenRecord")) {
        //             // Get citizenID
        //             command = strtok(NULL, " ");
        //             if (command) {
        //                 // Validate that citizenID is an integer
        //                 if (strspn(command, "0123456789") == strlen(command)) {
        //                     citizenID = malloc(strlen(command)+1);
        //                     strcpy(citizenID, command);
        //                     // Get firstName
        //                     command = strtok(NULL, " ");
        //                     if (command) {
        //                         fName = malloc(strlen(command)+1);
        //                         strcpy(fName, command);
        //                         // Get lastName
        //                         command = strtok(NULL, " ");
        //                         if (command) {
        //                             lName = malloc(strlen(command)+1);
        //                             strcpy(lName, command);
        //                             // Get country
        //                             command = strtok(NULL, " ");
        //                             if (command) {
        //                                 country = malloc(strlen(command)+1);
        //                                 strcpy(country, command);
        //                                 // Get age
        //                                 command = strtok(NULL, " ");
        //                                 if (command) {
        //                                     sscanf(command, "%d", &age);
        //                                     // Get virusName
        //                                     command = strtok(NULL, " ");
        //                                     if (command) {
        //                                         virus = malloc(strlen(command)+1);
        //                                         strcpy(virus, command);
        //                                         // Get YES/NO
        //                                         command = strtok(NULL, " ");
        //                                         if (command) {
        //                                             if (!strcmp(command, "YES")) {
        //                                                 // Get date
        //                                                 command = strtok(NULL, " ");
        //                                                 if (command) {
        //                                                     if (sscanf(command, "%d-%d-%d", &vaccDate.day, &vaccDate.month, &vaccDate.year) == 3 ) {
        //                                                         int check = insertCitizenCheck(recordsHead, citizenID, fName, lName, country, age, virus);
        //                                                         // Check if new record is inconsistent
        //                                                         if (check == 1) {
        //                                                             printf("CitizenID %s already in use for another record. Please enter a different one.\n", citizenID);
        //                                                         }
        //                                                         // New record is consistent
        //                                                         else {
        //                                                             SkipList* vList;
        //                                                             SkipList* nonVList;
        //                                                             SkipNode* node;

        //                                                             // Add country in State linked list
        //                                                             state = stateExists(stateHead, country);
        //                                                             if (!state) {
        //                                                                 state = insertState(&stateHead, country);
        //                                                             }

        //                                                             // Record already exists. Add new virus for record
        //                                                             if (check == 2) {
        //                                                                 record = insertVirusOnly(&recordsHead, citizenID, virus);
        //                                                             }
        //                                                             // Record is new. Add new record in record linked list
        //                                                             else if (!check) {
        //                                                                 record = insertSortedRecord(&recordsHead, citizenID, fName, lName, state, age, virus);
        //                                                             }

        //                                                             // Vaccinated Skip List for this virus exists
        //                                                             if (vList = virusSkipExists(skipVaccHead, virus)) {
        //                                                                 // CitizenID already vaccinated
        //                                                                 if (node = searchSkipList(vList, citizenID)) {
        //                                                                     printf("ERROR: CITIZEN %s ALREADY VACCINATED ON %d-%d-%d\n", citizenID, node->vaccDate.day, node->vaccDate.month, node->vaccDate.year);
        //                                                                 }
        //                                                                 // CitizenID not yet vaccinated
        //                                                                 else {
        //                                                                     // Add record in Bloom Filter
        //                                                                     insertInBloom(bloomsHead, citizenID, virus);
        //                                                                     // Add record in Skip List
        //                                                                     insertInSkip(skipVaccHead, record, virus, vaccDate);
        //                                                                 }
        //                                                             }
        //                                                             // Vaccinated Skip List for this virus doesn't exist
        //                                                             else {
        //                                                                 // Create Skip List for this virus
        //                                                                 skipVaccHead = createList(skipVaccHead, virus);
        //                                                                 // Add record in Skip List
        //                                                                 insertInSkip(skipVaccHead, record, virus, vaccDate);
                                                                    
        //                                                                 // Create Bloom Filter for this virus
        //                                                                 bloomsHead = createBloom(bloomsHead, virus, bloomSize, k);
        //                                                                 // Add record in Bloom Filter
        //                                                                 insertInBloom(bloomsHead, citizenID, virus);
        //                                                             }

        //                                                             // Non vaccinated Skip List for this virus exists
        //                                                             if (nonVList = virusSkipExists(skipNonVaccHead, virus)) {
                                                                    
        //                                                                 // CitizenID in this Skip List exists
        //                                                                 if (node = searchSkipList(nonVList, citizenID)) {
        //                                                                     // Remove node from non vaccinated Skip List
        //                                                                     removeFromSkip (nonVList, node);
        //                                                                 }
        //                                                             }
        //                                                         }
        //                                                     }
        //                                                     else {
        //                                                         printf("Please enter the date in dd-mm-yyyy fromat\n");
        //                                                     }
        //                                                 }
        //                                                 else {
        //                                                     printf("Please also enter a date\n");
        //                                                 }
        //                                             }
        //                                             else if (!strcmp(command, "NO")) {
        //                                                 int check = insertCitizenCheck(recordsHead, citizenID, fName, lName, country, age, virus);
        //                                                 // Check if new record is inconsistent
        //                                                 if (check == 1) {
        //                                                     printf("CitizenID %s already in use for another record. Please enter a different one.\n", citizenID);
        //                                                 }
        //                                                 // New record is consistent
        //                                                 else {
        //                                                     SkipList* vList;
        //                                                     SkipList* nonVList;
        //                                                     SkipNode* node;

        //                                                     // Add country in State linked list
        //                                                     state = stateExists(stateHead, country);
        //                                                     if (!state) {
        //                                                         state = insertState(&stateHead, country);
        //                                                     }

        //                                                     // Record already exists. Add new virus for record
        //                                                     if (check == 2) {
        //                                                         record = insertVirusOnly(&recordsHead, citizenID, virus);
        //                                                     }
        //                                                     // Record is new. Add new record in record linked list
        //                                                     else if (!check) {
        //                                                         record = insertSortedRecord(&recordsHead, citizenID, fName, lName, state, age, virus);
        //                                                     }

        //                                                     // Vaccinated Skip List for this virus exists
        //                                                     if (vList = virusSkipExists(skipVaccHead, virus)) {
        //                                                         // CitizenID already vaccinated
        //                                                         if (node = searchSkipList(vList, citizenID)) {
        //                                                             printf("ERROR: CITIZEN %s ALREADY VACCINATED ON %d-%d-%d\n", citizenID, node->vaccDate.day, node->vaccDate.month, node->vaccDate.year);
        //                                                         }
        //                                                         else {
        //                                                             // Non vaccinated Skip List for this virus exists
        //                                                             if (nonVList = virusSkipExists(skipNonVaccHead, virus)) {
                                                                    
        //                                                                 // CitizenID in this Skip List exists
        //                                                                 if (node = searchSkipList(nonVList, citizenID)) {
        //                                                                     // No need to add existing CitizenID in non vaccinated Skip List
        //                                                                     printf("ERROR: RECORD FOR CITIZEN %s ALREADY EXISTS FOR VIRUS %s\n", citizenID, nonVList->virus);
        //                                                                 }
        //                                                                 else {
        //                                                                     // Add record in non vaccinated Skip List
        //                                                                     vaccDate.day=0;
        //                                                                     vaccDate.month=0;
        //                                                                     vaccDate.year=0;
        //                                                                     insertInSkip(skipNonVaccHead, record, virus, vaccDate);
        //                                                                 }
        //                                                             }
        //                                                         }
        //                                                     }

        //                                                 }
        //                                             }
        //                                             else {
        //                                                 printf("The 7th parameter can only be a YES or NO\n");
        //                                             }
        //                                         }
        //                                         else {
        //                                             printf("Please also enter the following parameters: YES/NO, [date]\n");
        //                                         }
        //                                         free(virus);
        //                                     }
        //                                     else {
        //                                         printf("Please also enter the following parameters: virusName, YES/NO, [date]\n");
        //                                     }
        //                                 }
        //                                 else {
        //                                     printf("Please also enter the following parameters: age, virusName, YES/NO, [date]\n");
        //                                 }
        //                                 free(country);
        //                             }
        //                             else {
        //                                 printf("Please also enter the following parameters: country, age, virusName, YES/NO, [date]\n");
        //                             }
        //                             free(lName);
        //                         }
        //                         else {
        //                             printf("Please also enter the following parameters: lastName, country, age, virusName, YES/NO, [date]\n");
        //                         }
        //                         free(fName);
        //                     }
        //                     else {
        //                         printf("Please also enter the following parameters: firstName, lastName, country, age, virusName, YES/NO, [date]\n");
        //                     }
        //                     free(citizenID);
        //                 }
        //                 else {
        //                     printf("Please enter a numbers-only citizenID\n");
        //                 }
        //             }
        //             else {
        //                 printf("Please enter the following parameters: citizenID, firstName, lastName, country, age, virusName, YES/NO, [date] \n");
        //             }
        //     }
        //     // /vaccinateNow citizenID firstName lastName country age virusName
        //     else if (!strcmp(command, "/vaccinateNow")) {
        //             // Get citizenID
        //             command = strtok(NULL, " ");
        //             if (command) {
        //                 // Validate that citizenID is an integer
        //                 if (strspn(command, "0123456789") == strlen(command)) {
        //                     citizenID = malloc(strlen(command)+1);
        //                     strcpy(citizenID, command);
        //                     // Get firstName
        //                     command = strtok(NULL, " ");
        //                     if (command) {
        //                         fName = malloc(strlen(command)+1);
        //                         strcpy(fName, command);
        //                         // Get lastName
        //                         command = strtok(NULL, " ");
        //                         if (command) {
        //                             lName = malloc(strlen(command)+1);
        //                             strcpy(lName, command);
        //                             // Get country
        //                             command = strtok(NULL, " ");
        //                             if (command) {
        //                                 country = malloc(strlen(command)+1);
        //                                 strcpy(country, command);
        //                                 // Get age
        //                                 command = strtok(NULL, " ");
        //                                 if (command) {
        //                                     sscanf(command, "%d", &age);
        //                                     // Get virusName
        //                                     command = strtok(NULL, " ");
        //                                     if (command) {
        //                                         virus = malloc(strlen(command)+1);
        //                                         strcpy(virus, command);
                                                
        //                                         int check = insertCitizenCheck(recordsHead, citizenID, fName, lName, country, age, virus);
        //                                         // Check if new record is inconsistent
        //                                         if (check == 1) {
        //                                             printf("CitizenID %s already in use for another record. Please enter a different one.\n", citizenID);
        //                                         }
        //                                         // New record is consistent
        //                                         else {
        //                                             SkipList* vList;
        //                                             SkipList* nonVList;
        //                                             SkipNode* node;
        //                                             // Use current date as record's date
        //                                             vaccDate = getTime();

        //                                             // Add country in State linked list
        //                                             state = stateExists(stateHead, country);
        //                                             if (!state) {
        //                                                 state = insertState(&stateHead, country);
        //                                             }

        //                                             // Record already exists. Add new virus for record
        //                                             if (check == 2) {
        //                                                 record = insertVirusOnly(&recordsHead, citizenID, virus);
        //                                             }
        //                                             // Record is new. Add new record in record linked list
        //                                             else if (!check) {
        //                                                 record = insertSortedRecord(&recordsHead, citizenID, fName, lName, state, age, virus);
        //                                             }

        //                                             // Vaccinated Skip List for this virus exists
        //                                             if (vList = virusSkipExists(skipVaccHead, virus)) {
        //                                                 // CitizenID already vaccinated
        //                                                 if (node = searchSkipList(vList, citizenID)) {
        //                                                     printf("ERROR: CITIZEN %s ALREADY VACCINATED ON %d-%d-%d\n", citizenID, node->vaccDate.day, node->vaccDate.month, node->vaccDate.year);
        //                                                 }
        //                                                 // CitizenID not yet vaccinated
        //                                                 else {
        //                                                     // Add record in Bloom Filter
        //                                                     insertInBloom(bloomsHead, citizenID, virus);
        //                                                     // Add record in Skip List
        //                                                     insertInSkip(skipVaccHead, record, virus, vaccDate);
        //                                                 }
        //                                             }
        //                                             // Vaccinated Skip List for this virus doesn't exist
        //                                             else {
        //                                                 // Create Skip List for this virus
        //                                                 skipVaccHead = createList(skipVaccHead, virus);
        //                                                 // Add record in Skip List
        //                                                 insertInSkip(skipVaccHead, record, virus, vaccDate);
                                                    
        //                                                 // Create Bloom Filter for this virus
        //                                                 bloomsHead = createBloom(bloomsHead, virus, bloomSize, k);
        //                                                 // Add record in Bloom Filter
        //                                                 insertInBloom(bloomsHead, citizenID, virus);
        //                                             }

        //                                             // Non vaccinated Skip List for this virus exists
        //                                             if (nonVList = virusSkipExists(skipNonVaccHead, virus)) {
                                                    
        //                                                 // CitizenID in this Skip List exists
        //                                                 if (node = searchSkipList(nonVList, citizenID)) {
        //                                                     // Remove node from non vaccinated Skip List
        //                                                     removeFromSkip (nonVList, node);
        //                                                 }
        //                                             }
        //                                         }
        //                                         free(virus);
        //                                     }
        //                                     else {
        //                                         printf("Please also enter a virusName parameter.\n");
        //                                     }
        //                                 }
        //                                 else {
        //                                     printf("Please also enter the following parameters: age, virusName\n");
        //                                 }
        //                                 free(country);
        //                             }
        //                             else {
        //                                 printf("Please also enter the following parameters: country, age, virusName\n");
        //                             }
        //                             free(lName);
        //                         }
        //                         else {
        //                             printf("Please also enter the following parameters: lastName, country, age, virusName\n");
        //                         }
        //                         free(fName);
        //                     }
        //                     else {
        //                         printf("Please also enter the following parameters: firstName, lastName, country, age, virusName\n");
        //                     }
        //                     free(citizenID);
        //                 }
        //                 else {
        //                     printf("Please enter a numbers-only citizenID\n");
        //                 }
        //             }
        //             else {
        //                 printf("Please enter the following parameters: citizenID, firstName, lastName, country, age, virusName\n");
        //             }
        //     }
        //     // /list-nonVaccinated-Persons virusName
        //     else if (!strcmp(command, "/list-nonVaccinated-Persons")) {
        //         // Get virusName
        //         command = strtok(NULL, " ");
        //         if (command) {
        //             // Check if Skip List for virusName exists
        //             if (virusSkipExists(skipNonVaccHead, command)) {
        //                 virus = malloc(strlen(command)+1);
        //                 strcpy(virus, command);
        //                 SkipList* nonVaccSkipList = virusSkipExists(skipNonVaccHead, virus);

        //                 // Call vaccineStatus function
        //                 printSkipNodes(nonVaccSkipList);
        //                 free(virus);
        //             }
        //             else {
        //                 printf("Please enter an existing virus name\n");
        //             }
        //         }
        //         else {
        //             printf("Please enter a virus name\n");
        //         }
        //     }
        //     // /exit app
        //     else if (!strcmp(command, "/exit")) {
        //         // Deallocate memory
        //         free(text);
        //         free(input);
        //         freeStateList(stateHead);
        //         freeRecordList(recordsHead);
        //         freeBlooms(bloomsHead);
        //         freeSkipLists(skipVaccHead);
        //         freeSkipLists(skipNonVaccHead);
        //         fclose(inputFile);
        //         printf("exiting\n");

        //         return 1;
        //     }
        //     // /printRecord (print individual records)
        //     else if (!strcmp(command, "/printRecord")) {
        //         // Get citizenID
        //         command = strtok(NULL, " ");
        //         if (command) {
        //             citizenID = malloc(strlen(command)+1);
        //             strcpy(citizenID, command);
        //             printSingleRecord(recordsHead, citizenID);
        //             free(citizenID);
        //         }
        //     }
        //     else {
        //         printf("Command '%s' is unknown\n", command);
        //         printf("Please type a known command:\n");
        //     }
    // }

    return 0;
}