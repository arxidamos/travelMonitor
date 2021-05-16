OBJS = main.o mainFunctions.o stateList.o recordList.o bloomFilter.o skipList.o communication.o monitorDirList.o parentSignals.o childSignals.o
CHILD_OBJS = childMain.o mainFunctions.o stateList.o recordList.o bloomFilter.o skipList.o communication.o monitorDirList.o childSignals.o parentSignals.o
SOURCE = main.c mainFunctions.c stateList.c recordList.c bloomFilter.c skipList.c communication.c monitorDirList.c parentSignals.c childSignals.c
CHILD_SOURCE = childMain.c mainFunctions.c stateList.c recordList.c bloomFilter.c skipList.c communication.c monitorDirList.c childSignals.c parentSignals.c 
HEADER = structs.h functions.h
OUT = travelMonitor
CHILD_OUT = child
DIRS = ./named_pipes ./log_files
CC = gcc
FLAGS = -g3 -c -Wall

all:$(OUT) $(CHILD_OUT)

$(OUT): $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT)

$(CHILD_OUT): $(CHILD_OBJS)
	$(CC) -g $(CHILD_OBJS) -o $(CHILD_OUT)

child.o: childMain.c
	$(CC) $(FLAGS) childMain.c

travelMonitor.o:main.c
	$(CC) $(FLAGS) main.c

mainFunctions.o:mainFunctions.c
	$(CC) $(FLAGS) mainFunctions.c

stateList.o:stateList.c
	$(CC) $(FLAGS) stateList.c	

recordList.o:recordList.c
	$(CC) $(FLAGS) recordList.c

bloomFilter.o:bloomFilter.c
	$(CC) $(FLAGS) bloomFilter.c

skipList.o:skipList.c
	$(CC) $(FLAGS) skipList.c

monitorDirList.o:monitorDirList.c
	$(CC) $(FLAGS) monitorDirList.c

communication.o:communication.c
	$(CC) $(FLAGS) communication.c		

parentSignals.o:parentSignals.c
	$(CC) $(FLAGS) parentSignals.c

childSignals.o:childSignals.c
	$(CC) $(FLAGS) childSignals.c

clean:
	rm -f $(OBJS) $(OUT) $(CHILD_OBJS) $(CHILD_OUT)
	rm -rf $(DIRS)