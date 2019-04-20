#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "sharedMemory.h"

#define MAX_TIME_BETWEEN_RESOURCE_CHANGES_NANO 999999999
#define MAX_TIME_BETWEEN_RESOURCE_CHANGES_SECS 1

//takes in program name and error string, and runs error message procedure
void errorMessage(char programName[100], char errorString[100]){
	char errorFinal[200];
	sprintf(errorFinal, "%s : Error : %s", programName, errorString);
	perror(errorFinal);
	exit(1);
}

//generates a random number between 1 and 3
int randomNum(int max) {
	int num = (rand() % (max + 1));
	return num;
}

int randomPercent() {
	int num = rand() % 100 + 1;
	return num;
}

int main(int argc, char *argv[]) {
	//this section of code allows us to print the program name in error messages
	char programName[100];
	strcpy(programName, argv[0]);
	if (programName[0] == '.' && programName[1] == '/') {
		memmove(programName, programName + 2, strlen(programName));
	}

	printf("Welcome to the child\n");
	
	//set up shared memory
	int shmid;
	key_t smKey = 1094;
    shared_memory *sm; //allows us to request shared memory data
	//connect to shared memory
	if ((shmid = shmget(smKey, sizeof(shared_memory) + 256, IPC_CREAT | 0666)) == -1) {
        errorMessage(programName, "Function shmget failed. ");
    }
	//attach to shared memory
	sm = (shared_memory*) shmat(shmid, 0, 0);
	if (sm == NULL) {
		errorMessage(programName, "Function shmat failed. ");
	}
	
	printf("Child %d successfully connected to shared memory at %d:%d\n", getpid(), sm->clockSecs, sm->clockNano);
	
	//set up message queue
	key_t mqKey = 2461; 
    int msgid; 
    //msgget creates a message queue  and returns identifier
    msgid = msgget(mqKey, 0666 | IPC_CREAT); 
	if (msgid < 0) {
		errorMessage(programName, "Error using msgget for message queue ");
	}
	
	
	
	int startSecs, startNano, durationSecs, durationNano, endSecs, endNano; 
	int terminate = 0;
	while (terminate != 1) {
		//wait a few seconds
		//decide to either request or release some resources
		//decide if we should terminate
		//rinse and repeat
		
		durationSecs = randomNum(MAX_TIME_BETWEEN_RESOURCE_CHANGES_SECS);
		durationNano = randomNum(MAX_TIME_BETWEEN_RESOURCE_CHANGES_NANO);
		startSecs = sm->clockSecs;
		startNano = sm->clockNano;
		
		endSecs = startSecs + durationSecs;
		endNano = startNano + durationNano;
		if (endNano >= 1000000000) {
			endSecs += 1;
			endNano -= 1000000000;
		}
		printf("We will change our resource needs at %d:%d\n", endSecs, endNano); //we will try to launch our next child at endSecs:endNano
		
		while((endSecs > sm->clockSecs) || ((endSecs == sm->clockSecs) && (endNano > sm->clockNano))); //this dead wait will not work in final project
		
		printf("We are done waiting since we could start at %d:%d and it is now %d:%d\n", endSecs, endNano, sm->clockSecs, sm->clockNano);
		//now we want to request/release a resource. To do this we need a PCT
		
		int percent = randomPercent();
		
		if (percent < 46) {
			//release a resource
		} 
		if (percent > 45) {
			//request a resource
		}
		
		
		terminate = 1;
	}
	
	
	
	printf("Ending child %d at %d:%d\n", getpid(), sm->clockSecs, sm->clockNano);
	
	return 0;
}