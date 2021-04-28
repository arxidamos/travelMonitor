#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
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
    char* fifo_dir_path = "./named_pipes";
    // if (access(dir_path, F_OK) == 0)  // If dir already exists (due to abnormal previous termination, eg: SIGKILL)
    // delete_flat_dir(dir_path);      // remove it
    if (mkdir(fifo_dir_path, 0777) == -1) {
        perror("Error creating named_pipes directory");
        exit(1);
    }
    
    // Store the child processes' pids
    pid_t childpids[numMonitors];
    // Parent process' fds for read and write
    int parentRead_fd[numMonitors];
    int parentWrite_fd[numMonitors];

    for (int i=0; i<numMonitors; i++) {
        printf("Iteration %d\n", i);

        // Parent process' read and write ends
        char parentRead[32];
        char parentWrite[32];

        // Name them
        sprintf(parentRead, "./named_pipes/Read%d", i);
        sprintf(parentWrite, "./named_pipes/Write%d", i);
        
        printf("Creating fifos %d\n", i);

        // Create named pipes for read and write
        if (mkfifo(parentRead, 0666) == -1) {
            perror("Error creating named pipe");
            exit(1);
        }
        if (mkfifo(parentWrite, 0666) == -1) {
            perror("Error creating named pipe");
            exit(1);
        }

        // Open reading fifo for each child process
        if ((parentRead_fd[i] = open(parentRead, O_RDONLY | O_NONBLOCK)) == -1) {
            perror("Error opening named pipe for reading");
            exit(1);
        }
        // Open writing fifo for each child process
        if ((parentWrite_fd[i] = open(parentWrite, O_RDWR | O_NONBLOCK)) == -1) {
            perror("Error opening named pipe for writing");
            exit(1);
        }        

        // Create child processes
        if ( (childpids[i] = fork()) == -1) {
            perror("Error with fork");
            exit(1);
        }
        else if (childpids[i] == 0) {
            printf ("I am the child process with ID: %lu \n", ( long ) getpid ());
            execl("child", "lol", NULL);
            perror("Error with execl");
            // exit(0);
        }
        else {
            printf ("I am the parent process with ID: %lu \n", ( long ) getppid ());
        }
    }

    int status = 0;
    pid_t pid;
    int size = numMonitors;
    // for (int i=0; i<size; i++) {
        //     // pid = wait(&status);
        //     printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
        //     if (i != size-1) {
        //         childpids[i] = childpids[i+1];
        //     }   
        //     size--;

    // }

    // SIGNAL HANDLER FOR WHEN CHILD PROCESS EXITS
    // -1: wait for any child process to end, NULL: ignore child's return status, WNOHANG: avoid suspending the caller
    while ( (pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        
        int index;
        for (int i=0; i<size; i++) {
            if (childpids[i] == pid) {
                index = i;
                break;
            }
        }

        if ( (childpids[index] = fork()) == -1) {
            perror("ton poulo");
            exit(1);
        }
        else if (childpids[index] == 0) {
            printf ("I am the child process with ID: %lu \n", ( long ) getpid ());
            exit(0);
        }

    }

    for (int i=0; i<numMonitors; i++) {
        printf ("Childpid[%d]=%d\n", i, childpids[i]);
    }
    // char buffSize[20];
    // sprintf(buffSize, "%d", bufferSize);

    // for (int i=0; i<numMonitors; i++) {

    //     printf("%d iteration\n", i);

    //     // Read and write ends of parent
    //     char parentRead[32];
    //     char parentWrite[32];

    //     sprintf(parentRead, "./named_pipes/Read%d", i);
    //     sprintf(parentWrite, "./named_pipes/Write%d", i);
    //     printf("Creating fifos %d\n", i);

    //     // Create the named pipes
    //     if (mkfifo(parentRead, 0666) == -1) {
    //         perror("Error creating named pipe");
    //         exit(1);
    //     }
    //     if (mkfifo(parentWrite, 0666) == -1) {
    //         perror("Error creating named pipe");
    //         exit(1);
    //     }

    //     // Open reading fifo for each child process
    //     if ((parentRead_fd[i] = open(parentRead, O_RDONLY | O_NONBLOCK)) == -1) {
    //         perror("Error opening named pipe for reading");
    //         exit(1);
    //     }
    //     // Open writing fifo for each child process
    //     if ((parentWrite_fd[i] = open(parentWrite, O_RDWR | O_NONBLOCK)) == -1) {
    //         perror("Error opening named pipe for writing");
    //         exit(1);
    //     }

    //     printf("Opening fifos %d\n", i);

    //     if (childPids[i] = fork() == -1) {
    //         perror("Error with fork");
    //         exit(1);
    //     }
    //     if (childPids[i] == 0) {
    //         //  kalei execl me executable to Monitor kai orismata ta path twn pipes
    //         // execl("./diseaseAggregator_worker", "diseaseAggregator_worker", buf_size_str, read_p, writ_p, input_dir, NULL);
    //         // perror("execl");
    //         printf("Child process %d with ID %lu\n", i, (long)getpid());
    //         sleep(1);
    //         exit(1);
    //     }

        // else {
        //     printf("Parent process %d with ID %lu\n", i, (long)getpid());
        //     exit(1);
        //     // parent        process
        //     for (;;) {
        //         fd_set readFds;

        //         FD_ZERO(&readFds);  // Clear FD set for select
        //         for (int i=0; i<numMonitors; i++) {
        //             FD_SET(parentRead_fd[i], &readFds);
        //         }

        //         // int maxfd;
        //         // for (int i=0; i<numMonitors-1; i++) {
        //         //     maxfd = parentRead_fd[i] > parentRead_fd[i+1] ?  parentRead_fd[i] : parentRead_fd[i+1];
        //         // }
                
        //         int ret = select(FD_SETSIZE, &readFds, NULL, NULL, NULL);
        //         if (ret == -1) {
        //             perror("Error with select");
        //         }
        //         else if (ret) {
        //             for (int i=0; i<numMonitors; i++) {
        //                 // True if available data in this fd
        //                 if (FD_ISSET(parentRead_fd[i], &readFds)) {
        //                     char* message = 
        //                 }

        //             }
        //         }


        //     }

        // }

    // }



    // το travelMonitor app φτιάχνει Χ named pipes για επικοινωνία με child processes
    // το travelMonitor app κάνει fork X child processes.
    // το child process καλεί exec με εκτελέσιμο το Monitor και ορίσματα του Monitor τα path των pipes
    // με αυτά τα pipes μιλάει το travelMonitor με κάθε Monitor
    // το travelMonitor ενημερώνει κάθε Monitor μέσω του pipe για το ποια subdirs θα πάρει
    // το travelMonitor μοιράζει round-robin τα subdirs
    // τα Monitor στέλνoυν bloom filter στο travelMonitor
    // το travelMonitor λαμβάνει αυτά και αναμένει εντολές
    // τα Monitor διαβάζουν από τα ανατεθέντα αρχεία, γεμίζουν δομές


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

    free(dir_path);
    closedir(input_dir);


    fflush(stdout);

    return 0;
}