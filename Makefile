OBJS = main.o mainFunctions.o stateList.o recordList.o bloomFilter.o skipList.o
TEMP_OBJS = tisPoutsasMain.o
TEMP_SOURCE = tisPoutsasMain.c
SOURCE = main.c mainFunctions.c stateList.c recordList.c bloomFilter.c skipList.c
HEADER = structs.h functions.h
OUT = travelMonitor
TEMP_OUT = child
CC = gcc
FLAGS = -g3 -c -Wall

# all:$(OBJS)
# 	$(CC) -g $(OBJS) -o $(OUT)

all:$(OUT) $(TEMP_OUT)

$(OUT): $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT)

$(TEMP_OUT): $(TEMP_OBJS)
	$(CC) -g $(TEMP_OBJS) -o $(TEMP_OUT)

child.o: tisPoutsasMain.c
	$(CC) $(FLAGS) tisPoutsasmain.c

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

clean:@echo "Cleaning up..."
	rm -vf $(OBJS) $(OUT) 