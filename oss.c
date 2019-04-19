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
	
	//destroy master process
	if (error)
		kill(-1*getpid(), SIGKILL);	
}


//takes in program name and error string, and runs error message procedure
void errorMessage(char programName[100], char errorString[100]){
	char errorFinal[200];
	sprintf(errorFinal, "%s : Error : %s", programName, errorString);
	perror(errorFinal);
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
	printf("Welcome to project 5\n");
	
	key_t key = 1094;
	
    shared_memory *sm; //allows us to request shared memory data

	//connect to shared memory
	if ((shmid = shmget(key, sizeof(shared_memory) + 256, IPC_CREAT | 0666)) == -1) {
		errorMessage(programName, "Function shmget failed. ");
    }
	//attach to shared memory
    sm = (shared_memory *) shmat(shmid, 0, 0);
    if (sm == NULL ) {
        errorMessage(programName, "Function shmat failed. ");
    }
	
	sm->clockSecs = 0;
	sm->clockNano = 0;
	
	int y = 3;
	int x = 20;
	
	int i, j;
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
	
	//print our resource board
	for (i = 0; i < y; i++) {
		for (j = 0; j < x; j++) {
			printf("%d\t", sm->resource[j][i]);
		}
		printf("\n");
	}
	
	//now let's start our loop, or for this variation, one child called
	int terminate = 0;
	int makeChild = 1;
	
	while (terminate != 1) {
		if (makeChild == 1) {
			pid_t pid;
			pid = fork();
						
			if (pid == 0) { //child
				execl ("user", "user", NULL);
				errorMessage(programName, "execl function failed. ");
			} else if (pid > 0) { //parent
				makeChild = 0; //for now, we only want to create one child, for testing purposes
				//write to output file the time this process was launched
				printf("Created child %d at %d:%d\n", pid, sm->clockSecs, sm->clockNano);
				continue;
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
			terminate = 1;
		}
	
	}

	printf("Successful end of program\n");
	
	endAll(0);
	return 0;
}