//let's start by getting a program that'll fork children at random times
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "sharedMemory.h"

#define CLOCK_INC 1000


int shmid;
int msgid;

struct PCB {
	int myPID; //your local simulated pid
	int myResource[20];
};

int randomNum() {
	int num = (rand() % (10) + 1);
	return num;
}

//called whenever we terminate program
void endAll(int error) {
    //destroy shared memory
	int ctl_return = shmctl(shmid, IPC_RMID, NULL);
	if (ctl_return == -1) {
		perror(" Error with shmctl command: Could not remove shared memory ");
		exit(1);
	}
	
	//close message queue
	msgctl(msgid, IPC_RMID, NULL);
	
	//destroy master process
	if (error)
		kill(-1*getpid(), SIGKILL);	
}

//checks our boolArray for an open slot to save the process. Returns -1 if none exist
int checkForOpenSlot(bool boolArray[], int maxKidsAtATime) {
	int i;
	for (i = 0; i < maxKidsAtATime; i++) {
		if (boolArray[i] == false) {
			return i;
		}
	}
	return -1;
}

//takes in program name and error string, and runs error message procedure
void errorMessage(char programName[100], char errorString[100]){
	char errorFinal[200];
	sprintf(errorFinal, "%s : Error : %s", programName, errorString);
	perror(errorFinal);
	endAll(1);
}

//called when interupt signal (^C) is called
void intHandler(int dummy) {
	printf(" Interupt signal received.\n");
	endAll(1);
}

int main(int argc, char *argv[]) {
	//this section of code allows us to print the program name in error messages
	char programName[100];
	strcpy(programName, argv[0]);
	if (programName[0] == '.' && programName[1] == '/') {
		memmove(programName, programName + 2, strlen(programName));
	}
	
	srand(time(0)); //placed here so we can generate random numbers later on
	signal(SIGINT, intHandler); //signal processing
	printf("Welcome to project 5\n");
	
	int i, j;
	int maxKidsAtATime = 1; //for now, this is our test value. This will eventually be up to 18, depending on command line arguments
	
	//we need our process control table
	struct PCB PCT[maxKidsAtATime]; //here we have a table of maxNum blocks
	//our "bit vector" (or boolean array here) that will tell us which PRBs are free
	bool boolArray[maxKidsAtATime]; //by default, these are all set to false. This has been tested and verified
	for (i = 0; i < maxKidsAtATime; i++) {
		boolArray[i] = false; //just set them all to false - quicker then checking
	}
	
	key_t shKey = 1094;
    shared_memory *sm; //allows us to request shared memory data
	//connect to shared memory
	if ((shmid = shmget(shKey, sizeof(shared_memory) + 256, IPC_CREAT | 0666)) == -1) {
		errorMessage(programName, "Function shmget failed. ");
    }
	//attach to shared memory
    sm = (shared_memory *) shmat(shmid, 0, 0);
    if (sm == NULL ) {
        errorMessage(programName, "Function shmat failed. ");
    }
	
	//set up message queue
	key_t mqKey = 2461; 
    //msgget creates a message queue  and returns identifier
    msgid = msgget(mqKey, 0666 | IPC_CREAT); 
	if (msgid < 0) {
		errorMessage(programName, "Error using msgget for message queue ");
	}
	
	
	sm->clockSecs = 0;
	sm->clockNano = 0;
	
	int y = 3;
	int x = 20;
	
	for (i = 0; i < y; i++) {
		for (j = 0; j < x; j++) {
			if (i == 0) {
				sm->resource[j][i] = randomNum(); //set the maximum number of this resource available to the system. These numbers do not change
			} else if ( i == 1) {
				sm->resource[j][i] = sm->resource[j][0]; //set the current number of available instances of this resource. This number does change
			} else {
				if (randomNum() < 3) {
					sm->resource[j][i] = 1; //mark this resource as a sharable resource
				} else {
					sm->resource[j][i] = 0; //mark this resource as a nonsharable resource
				}
			}
		}
	}
	//printf("b\n");
	//print our resource board
	for (i = 0; i < y; i++) {
		for (j = 0; j < x; j++) {
			printf("%d\t", sm->resource[j][i]);
		}
		printf("\n");
	}
	//printf("a\n");
	//now let's start our loop, or for this variation, one child called
	int terminate = 0;
	int makeChild = 1;
	
	while (terminate != 1) {
		if (makeChild == 1) { //we need to make a child process
		
			printf("Lets make a child process\n");
			int openSlot = checkForOpenSlot(boolArray, maxKidsAtATime);
			if (openSlot != -1) { //a return value of -1 means all slots are currently filled, and per instructions we are to ignore this process
				printf("Found empty slot in boolArray[%d]\n", openSlot);
				pid_t pid;
				pid = fork();
				
				if (pid == 0) { //child //NOTE, FOR TESTING THE TWO LINES BELOW ARE COMMENTED OUT
					execl ("user", "user", NULL);
					errorMessage(programName, "execl function failed. ");
				}
				else if (pid > 0) { //parent
					makeChild = 0; //for now, we only want to create one child, for testing purposes
					printf("Created child %d at %d:%d\n", pid, sm->clockSecs, sm->clockNano);
					boolArray[openSlot] = true; //claim that spot
					//processesLaunched++;
					//processesRunning++;
					//prepNewChild = false;
					
					//let's populate the control block with our data
					PCT[openSlot].myPID = pid;
					printf("Lets set child %d to 0 resources for starting out\n", pid); //there is a seg fault right after this!!!!!
					for (i = 0; i < 20; i++) {
						PCT[openSlot].myResource[i] = 0; //start out with having 0 of each resource
					}
					printf("Child %d is ready to roll\n", pid);
					continue;
				}
				else {
					errorMessage(programName, "Unexpected return value from fork ");
				}
			}
		}
		 
		sm->clockNano += CLOCK_INC; //increment clock
		if (sm->clockNano >= 1000000000) { //increment the next unit
			sm->clockSecs += 1;
			sm->clockNano -= 1000000000;
		}
		
		int temp = waitpid(-1, NULL, WNOHANG); //required to properly end processes and avoid a fork bomb
		if (temp < 0) {
			errorMessage(programName, "Unexpected result from terminating process ");
		} else if (temp > 0) {
			printf("Process %d confirmed to have ended at %d:%d\n", temp, sm->clockSecs, sm->clockNano);
			//deallocate resources and continue
			for (i = 0; i < sizeof(PCT); i++) {
				if (PCT[i].myPID == temp) {
					boolArray[i] = false;
					PCT[i].myPID = 0; //remove PID value from this slot
					for (j = 0; j < 20; j++) {
						PCT[i].myResource[j] = 0; //reset each resource to zero
					}
					break; //and for the moment, that's all we should need to deallocate
				}
			}
			//processesRunning--;
			terminate = 1;
		}
	
	}

	printf("Successful end of program\n");
	
	endAll(0);
	return 0;
}