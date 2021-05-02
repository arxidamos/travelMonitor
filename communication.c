#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* readBytes(char* msg, int length, int fd, char *buf, int bufSize) {
    int receivedTotal = 0;
    // Read teh message
    while (receivedTotal < length) {
        // If bytes left are less than pipe size, read them only
        int left = length - receivedTotal;
        int bytesToRead = left < bufSize ? left : bufSize;

        int received;
        if ( (received = read(fd, buf, bytesToRead)) == -1 ) {
            perror("Error with reading message");
        }
 
        // printf("Reading...");
        // for (int k=0; k<received; k++) {
        //     printf("%c", buf[k]);
        // }
        // printf("\n");
        strncpy(msg + receivedTotal, buf, received);
        receivedTotal += received;
    }
    return msg;
}