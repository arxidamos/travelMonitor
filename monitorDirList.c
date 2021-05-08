#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "functions.h"

// Insert country to MonitorDir linked list
MonitorDir* insertDir (MonitorDir** head, DIR* dir, char* country, char* files[], int fileCount) {
    MonitorDir* newNode;
    MonitorDir* current;

    newNode = malloc(sizeof(MonitorDir));
    newNode->next = NULL;

    newNode->dir = dir;

    newNode->country = malloc(strlen(country)+1);
    strcpy(newNode->country, country);

    
    newNode->fileCount = fileCount;

    newNode->files = malloc(sizeof(char*)*newNode->fileCount);
    for (int i=0; i<fileCount; i++) {
        newNode->files[i] = malloc(strlen(files[i])+1);
        strcpy(newNode->files[i], files[i]);
    }

    // Check if to insert on head
    if (!*head) {
        newNode->next = *head;
        *head = newNode;
    }
    // Move across list
    else {
        current = *head;
        while (current->next) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }
    return *head;
}

// Print this Monitor's directory list
void printMonitorDirList (MonitorDir* monitorDir) {
    printf("MonitorDir directories:");
    
    if (monitorDir == NULL) {
        printf("Huh? MONITORDIR is null?\n");
    }
    while (monitorDir != NULL) {
        printf(" %s,", monitorDir->country);
        printf("With files:");
        for (int i=0; i<monitorDir->fileCount; i++) {
            printf(" %s", monitorDir->files[i]);
        }
        monitorDir = monitorDir->next;
    }
    printf("\n");
}

// Free memory allocated for linked list 
void freeMonitorDirList (MonitorDir* head) {
    MonitorDir* current = head;
    MonitorDir* tmp;

    while (current) {
        tmp = current;
        current = current->next;
        for (int i=0; i<tmp->fileCount; i++) {
            free(tmp->files[i]);
        }
        free(tmp->files);
        free(tmp->country);
        free(tmp);
    }
}