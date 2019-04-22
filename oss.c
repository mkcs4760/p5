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
#include "messageQueue.h"

#define CLOCK_INC 1000
#define RESOURCE_COUNT 20


struct PCB {
	int myPID; //your local simulated pid
	int myResource[20];
};

int randomNum(int min, int max) {
	int num = (rand() % (max - min) + min);
	return num;
}

/*
int randomNum() {
	int num = (rand() % (10) + 1);
	return num;
}
*/

//called whenever we terminate program
void endAll(int error) {
	//message queue is closed separately, as the id is no longer a global variable due to errors while testing
	

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
	printf("For the record, parent pid = %d\n", getpid());
	
	int i, j;
	int maxKidsAtATime = 1; //for now, this is our test value. This will eventually be up to 18, depending on command line arguments
	
	//we need our process control table
	struct PCB PCT[maxKidsAtATime]; //here we have a table of maxNum process blocks
	int p;
	//function to set all starting values to 0 and print out all allocated resources
	for (p = 0; p < maxKidsAtATime; p++) {
		PCT[p].myPID = 0; 
		printf("PARENT: Slot #%d, containing PID %d: \n", p, PCT[p].myPID);
		int q;
		for (q = 0; q < 20; q++) {
			PCT[p].myResource[q] = 0;
			printf("%d ", PCT[p].myResource[q]);
		}
		printf("\n");
	}
	bool boolArray[maxKidsAtATime]; //our "bit vector" (or boolean array here) that will tell us which PRBs are free
	for (i = 0; i < maxKidsAtATime; i++) {
		boolArray[i] = false; //just set them all to false - quicker then checking
	}
	
	int shmid;
	key_t shKey = 1094;
    shared_memory *sm; //allows us to request shared memory data
	//connect to shared memory
	if ((shmid = shmget(shKey, sizeof(shared_memory), IPC_CREAT | 0666)) == -1) {
		errorMessage(programName, "Function shmget failed. ");
    }
	//attach to shared memory
    sm = (shared_memory *) shmat(shmid, 0, 0);
    if (sm == NULL ) {
        errorMessage(programName, "Function shmat failed. ");
    }
	
	/*int shm2id; //needed a 2nd shared memory segment to avoid errors where values overwrote each other
	key_t sh2Key = 1095;
    shared_memory2 *sm2; //allows us to request shared memory data
	//connect to shared memory
	if ((shm2id = shmget(sh2Key, sizeof(shared_memory2), IPC_CREAT | 0666)) == -1) {
		errorMessage(programName, "Function shmget failed. ");
    }
	//attach to shared memory
    sm2 = (shared_memory2 *) shmat(shm2id, 0, 0);
    if (sm2 == NULL ) {
        errorMessage(programName, "Function shmat failed. ");
    }*/
	
	
	//function to print out all allocated resources
	for (p = 0; p < maxKidsAtATime; p++) {
		printf("PARENT: Slot #%d, containing PID %d: \n", p, PCT[p].myPID);
		int q;
		for (q = 0; q < 20; q++) {
			printf("%d ", PCT[p].myResource[q]);
		}
		printf("\n");
	}
	
	int msqid;
	key_t mqKey; 
    // ftok to generate unique key 
    mqKey = 2931; //ftok("progfile", 65); 
	printf("We received a key value of %d\n", mqKey);
    // msgget creates a message queue and returns identifier 
    msqid = msgget(mqKey, 0666 | IPC_CREAT);  //create the message queue
	if (msqid < 0) {
		printf("Error, msqid equals %d\n", msqid);
	}
	else {
		printf("PARENT: We received a msqid value of %d\n", msqid);
	}
	
	sm->clockSecs = 0; //start the clock at 0
	sm->clockNano = 0;
	
	int y = 3;
	
	for (i = 0; i < y; i++) {
		for (j = 0; j < RESOURCE_COUNT; j++) {
			if (i == 0) {
				sm->resource[j][i] = randomNum(1, 10); //set the maximum number of this resource available to the system. These numbers do not change
			} else if ( i == 1) {
				sm->resource[j][i] = sm->resource[j][0]; //set the current number of available instances of this resource. This number does change
			} else {
				if (randomNum(1, 10) < 3) {
					sm->resource[j][i] = 1; //mark this resource as a sharable resource
				} else {
					sm->resource[j][i] = 0; //mark this resource as a nonsharable resource
				}
			}
		}
	}
	//print our resource board
	printf("First printout of our resource board\n");
	for (i = 0; i < y; i++) {
		for (j = 0; j < RESOURCE_COUNT; j++) {
			printf("%d\t", sm->resource[j][i]);
		}
		printf("\n");
	}
	
	//now let's start our loop, or for this variation, one child called
	int terminate = 0;
	int makeChild = 1;
	
	while (terminate != 1) {
		if (makeChild == 1) { //we need to make a child process
		
			printf("Lets make a child process\n");
			int openSlot = checkForOpenSlot(boolArray, maxKidsAtATime);
			if (openSlot != -1) { //a return value of -1 means all slots are currently filled, and per instructions we are to ignore this process
				printf("Found empty slot in boolArray[%d]\n", openSlot);
				
				printf("Another print out of our resource board right before we fork\n");
				for (i = 0; i < y; i++) {
					for (j = 0; j < RESOURCE_COUNT; j++) {
						printf("%d\t", sm->resource[j][i]);
					}
					printf("\n");
				}
				
				pid_t pid;
				pid = fork();
				
				if (pid == 0) {
					execl ("user", "user", NULL);
					errorMessage(programName, "execl function failed. ");
				}
				else if (pid > 0) { //parent
					
					printf("Another print out of our resource board right after we fork\n");
					for (i = 0; i < y; i++) {
						for (j = 0; j < RESOURCE_COUNT; j++) {
							printf("%d\t", sm->resource[j][i]);
						}
						printf("\n");
					}
					
					makeChild = 0; //for now, we only want to create one child, for testing purposes
					printf("Created child %d at %d:%d\n", pid, sm->clockSecs, sm->clockNano);
					boolArray[openSlot] = true; //claim that spot
					//processesLaunched++; //these will be implemented later, so they remain here as a reminder
					//processesRunning++;
					//prepNewChild = false;
					
					//let's populate the control block with our data //NOTE: THIS STUFF CAUSED A PROBLEM. IT WAS REMOVED, BUT IT SHOULD BE REPLACED WITH SOMETHING THAT WORKS
					printf("Changing value %d to %d in PCT[%d]\n", PCT[openSlot].myPID, pid, openSlot);
					PCT[openSlot].myPID = pid;
					printf("Lets set child %d to 0 resources for starting out\n", pid);
					for (i = 0; i < RESOURCE_COUNT; i++) {
						PCT[openSlot].myResource[i] = 0; //start out with having 0 of each resource
					} //does the above hunk of code now work???
					printf("Child %d is ready to roll\n", pid);
					printf("Another print out of our resource board at very end of fork section\n");
					for (i = 0; i < y; i++) {
						for (j = 0; j < RESOURCE_COUNT; j++) {
							printf("%d\t", sm->resource[j][i]);
						}
						printf("\n");
					}
					
					
					int p;
					//function to print out all allocated resources
					for (p = 0; p < maxKidsAtATime; p++) {
						printf("PARENT: Slot #%d, containing PID %d: \n", p, PCT[p].myPID);
						int q;
						for (q = 0; q < 20; q++) {
							printf("%d ", PCT[p].myResource[q]);
						}
						printf("\n");
					}
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
		
		//we check if we've received a message from any of our processes
		int receive;
		receive = msgrcv(msqid, &message, sizeof(message), getpid(), IPC_NOWAIT); //will wait until is receives a message
		if (receive > 0) {
			printf("Data Received from child to parent is : %s \n", message.mesg_text); //display the message
			printf("The bool 'receive' that was sent over had a value of %d\n", message.request);
			if (message.request == true) { //we're requesting a resource
				printf("Process %d is requesting %d of resource %d\n.", message.return_address, message.resAmount, message.resID);
				
				message.mesg_type = message.return_address; //send it to the process that just sent you something
				strncpy(message.mesg_text, "parent to child", 100);
				if (randomNum(1, 10) == 1) {
					message.mesg_value = 10; //accept request
					printf("PARENT: I choose to accept the request\n");
					
					for (i = 0; i < maxKidsAtATime; i++) {
						if (PCT[i].myPID == message.return_address) {
							//this is the slot we want to allocate resources to
							PCT[i].myResource[message.resID] += message.resAmount; //we just allocated a resource
							printf("Decreasing value %d...\n", sm->resource[message.resID][1]);
							sm->resource[message.resID][1] -= message.resAmount; //these parameters may possibly be in the wrong order..........
							printf("PARENT: Process %d got what it wanted!!!!!!!!!!!!!!!\n", PCT[i].myPID);
							//now let's print out the updated board
							//print our resource board
							printf("PARENT: updated printout of our resource board\n");
							for (i = 0; i < y; i++) {
								for (j = 0; j < RESOURCE_COUNT; j++) {
									printf("%d\t", sm->resource[j][i]);
								}
								printf("\n");
							}
							
							
							//function to print out all allocated resources
							for (p = 0; p < maxKidsAtATime; p++) {
								printf("PARENT: Slot #%d, containing PID %d: \n", p, PCT[p].myPID);
								int q;
								for (q = 0; q < 20; q++) {
									printf("%d ", PCT[p].myResource[q]);
								}
								printf("\n");
							}
						}
					}
				} else {
					message.mesg_value = 1; //deny request
					printf("PARENT: I choose to deny the request\n");
				}
				message.return_address = getpid();
				int send = msgsnd(msqid, &message, sizeof(message), 0); //send message
				if (send == -1) {
					perror("Error on msgsnd\n");
				}
				printf("Data sent is : %s \n", message.mesg_text); // display the message 	
			
				
			} else { //we're releasing a resource
				printf("Process %d is releasing %d of resource %d\n.", message.return_address, message.resAmount, message.resID);
			}
			
		
		}
		
		//we check to see if any of our processes have ended
		int temp = waitpid(-1, NULL, WNOHANG);
		if (temp < 0) {
			errorMessage(programName, "Unexpected result from terminating process ");
		} else if (temp > 0) {
			printf("Process %d confirmed to have ended at %d:%d\n", temp, sm->clockSecs, sm->clockNano);
			//deallocate resources and continue
			for (i = 0; i < maxKidsAtATime; i++) {
				if (PCT[i].myPID == temp) {
					boolArray[i] = false;
					printf("PARENT: Deallocating process %d from PCT\n", PCT[i].myPID);
					PCT[i].myPID = 0; //remove PID value from this slot
					for (j = 0; j < 20; j++) {
						PCT[i].myResource[j] = 0; //reset each resource to zero
					}
					break; //and for the moment, that's all we should need to deallocate
				}
			}
			//processesRunning--;
			terminate = 1;
		} //if temp == 0, nothing has ended and we simply carry on
	}

	//print our resource board
	for (i = 0; i < y; i++) {
		for (j = 0; j < RESOURCE_COUNT; j++) {
			printf("%d\t", sm->resource[j][i]);
		}
		printf("\n");
	}
	
	//function to print out all allocated resources
	for (p = 0; p < maxKidsAtATime; p++) {
		printf("PARENT: Slot #%d, containing PID %d: \n", p, PCT[p].myPID);
		int q;
		for (q = 0; q < 20; q++) {
			printf("%d ", PCT[p].myResource[q]);
		}
		printf("\n");
	}
	
	printf("Successful end of program\n");
	
	//clean up resources
	int mqDestroy = msgctl(msqid, IPC_RMID, NULL); //destroy message queue
	int smDestroy = shmctl(shmid, IPC_RMID, NULL); //destroy shared memory
	if (mqDestroy == -1) {
		perror(" Error with msgctl command: Could not remove message queue ");
		exit(1);
	}	
	if (smDestroy == -1) {
		perror(" Error with shmctl command: Could not remove shared memory ");
		exit(1);
	}


	endAll(0);
	return 0;
}