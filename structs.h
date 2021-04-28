#ifndef __STRUCTS__
#define __STRUCTS__ 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define MAX 32

typedef struct Date {
    int day;    //dd
    int month;  //mm
    int year;   //yyyy
} Date;

typedef struct State {
    char* name;
    struct State* next;
} State;

typedef struct Record {
    char* citizenID;
    char* firstName;
    char* lastName;
    int age;
    char** virus;
    int virusCount;
    State* country;
    Date vaccDate; // dd-mm-yyyy
    struct Record* next;
} Record;

typedef struct BloomFilter {
    char* virus;
    int size;
    int k;
    int* bitArray;
    struct BloomFilter* next;
} BloomFilter;

typedef struct SkipNode {
    char* citizenID;
    Date vaccDate; // dd-mm-yyyy
    Record* record;
    int levels;
    struct SkipNode* next[MAX]; // Array of nexts, one for each level
} SkipNode;

typedef struct SkipList {
    SkipNode* head;
    int maxLevel;
    char* virus;
    struct SkipList* next;
} SkipList;

#endif