#ifndef __FUNCTIONS__
#define __FUNCTIONS__ 
#include "structs.h"

// stateList.c
State* insertState (State** head, char* country);
State* stateExists (State* head, char* country);
void printStateList (State* state);
void freeStateList (State* head);

// recordList.c
Record* createRecord (char* citizenID, char* fName, char* lName, char* country, int age, char* virus, Date vaccDate);
Record* insertSortedRecord (Record** head, char* citizenID, char* fName, char* lName, State* state, int age, char* virus);
Record* insertVirusOnly (Record** head, char* citizenID, char* virus);
int checkDuplicate (Record* head, char* citizenID, char* fName, char* lName, State* state, int age, char* virus);
int checkExistence (Record* head, char* citizenID);
void printRecordsList (Record* record);
void printSingleRecord (Record* record, char* citizenID);
void freeRecordList (Record* head);

// bloomFilter.c
unsigned long djb2 (unsigned char *str);
unsigned long sdbm (unsigned char *str);
unsigned long hash_i (unsigned char *str, unsigned int i);
BloomFilter* createBloom (BloomFilter* bloomsHead, char* virus, int size, int k);
void insertInBloom (BloomFilter* bloomsHead, char* citizenID, char* virus);
int virusBloomExists (BloomFilter* bloomsHead, char* virus);
void printBloomsList (BloomFilter* head);
void freeBlooms (BloomFilter* head);

// skipList.c
SkipList* createList (SkipList* skipListHead, char* virus);
void insertInSkip (SkipList* skipListHead, Record* record, char* virus, Date vaccDate);
SkipNode* searchSkipList (SkipList* skipListHead, char* citizenID);
int searchCountrySkipList (SkipList* skipListHead, char* country, Date date1, Date date2);
int* searchCountryByAge (SkipList* skipListHead, char* country, Date date1, Date date2);
SkipList* virusSkipExists (SkipList* skipListHead, char* virus);
void removeFromSkip (SkipList* skipListHead, SkipNode* node);
int getHeight (int maximum);
void printSkipLists (SkipList* head);
void printSkipNodes (SkipList* skipList);
void freeSkipLists (SkipList* head);
void freeSkipNodes (SkipList* skipList);

// mainFunctions.c
// Command functions
void travelRequest (BloomFilter* head, ChildMonitor* childMonitor, int numMonitors, int* incfd, int* outfd, int* accepted, int* rejected, char* citizenID, char* countryFrom, char* countryTo, char* virus);
void vaccineStatusBloom (BloomFilter* head, char* citizenID, char* virus);
void vaccineStatus (SkipList* head, char* citizenID, char* virus);
void vaccineStatusAll (SkipList* head, char* citizenID);
void populationStatus (SkipList* skipVaccHead, SkipList* skipNonVaccHead, char* country, State* stateHead, Date date1, Date date2);
void popStatusByAge (SkipList* skipVaccHead, SkipList* skipNonVaccHead, char* country, State* stateHead, Date date1, Date date2);
int insertCitizenCheck (Record* head, char* citizenID, char* fName, char* lName, char* country, int age, char* virus);
// Auxiliary functions
int compareDate (Date a, Date b);
int isBetweenDates (Date a, Date x, Date b);
Date getTime ();

//
void sigchldHandler();
void sigQuitHandler (int sigNum);
void handleSignals(void);
void checkSigQuit (State** stateHead, Record** recordsHead, BloomFilter** bloomsHead, SkipList** skipVaccHead, SkipList** skipNonVaccHead, MonitorDir* MonitorDir, char* dir_path);
void analyseChildMessage(Message* message, int *readyMonitors, int outfd, int bufSize, BloomFilter** bloomsHead, int bloomSize);
// void analyseMessage (MonitorDir** monitorDir, Message* message, int outfd, int* bufSize, int* bloomSize, char* dir_path, BloomFilter* bloomsHead);
void analyseMessage (MonitorDir** monitorDir, Message* message, int outfd, int* bufSize, int* bloomSize, char* dir_path, BloomFilter** bloomsHead, State** stateHead, Record** recordsHead, SkipList** skipVaccHead, SkipList** skipNonVaccHead);
int getMessage (Message* incMessage, int incfd, int bufSize);
char* readBytes(char* msg, int length, int fd, int bufSize);
void sendBytes (char code, char* body, int fd, int bufSize);
void mapCountryDirs (char* dir_path, int numMonitors, int outfd[], ChildMonitor childMonitor[], int bufSize);
int compare (const void * a, const void * b);
int getUserCommand(int* readyMonitors, int numMonitors, ChildMonitor* childMonitor, BloomFilter* bloomsHead,
 char* dir_path, DIR* input_dir, int* incfd, int* outfd, int bufSize, int* accepted, int* rejected);

void updateParentBlooms(BloomFilter* bloomsHead, int outfd, int bufSize);
BloomFilter* insertBloomInParent (BloomFilter** bloomsHead, char* virus, int size, int k);
void updateBitArray (BloomFilter* bloomFilter, char* bitArray);
MonitorDir* insertDir (MonitorDir** head, DIR* dir, char* country, char* files[], int fileCount);
void printMonitorDirList (MonitorDir* monitorDir);
void freeMonitorDirList (MonitorDir* head);
#endif