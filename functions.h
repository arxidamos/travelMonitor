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

#endif