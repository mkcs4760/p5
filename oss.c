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
	int myResource[2][RESOURCE_COUNT]; //[0] is what it has, [1] is what it wants
};

int randomNum(int min, int max) {
	int num = (rand() % (max - min) + min);
	return num;
}

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
	//printf("For the record, parent pid = %d\n", getpid());
	
	int i, j;
	int maxKidsAtATime = 1; //for now, this is our test value. This will eventually be up to 18, depending on command line arguments
	
	//we need our process control table
	struct PCB PCT[maxKidsAtATime]; //here we have a table of maxNum process blocks
	struct PCB sPCT[maxKidsAtATime]; //simulated PCT used for deadlock detection
	int sResource[3][RESOURCE_COUNT];
	int p;
	//function to set all starting values to 0 and print out all allocated resources
	for (p = 0; p < maxKidsAtATime; p++) {
		PCT[p].myPID = 0; 
		printf("PARENT0: Slot #%d, containing PID %d: \n", p, PCT[p].myPID);
		int q;
		for (q = 0; q < 20; q++) {
			PCT[p].myResource[0][q] = 0;
			printf("%d ", PCT[p].myResource[0][q]);
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
	
	int msqid;
	key_t mqKey; 
    mqKey = 2931; 
    msqid = msgget(mqKey, 0666 | IPC_CREAT);  //create the message queue
	if (msqid < 0) {
		printf("Error, msqid equals %d\n", msqid);
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
		//printf("Start of while loop\n");
		if (makeChild == 1) { //we need to make a child process
		
			printf("Lets make a child process\n");
			int openSlot = checkForOpenSlot(boolArray, maxKidsAtATime);
			if (openSlot != -1) { //a return value of -1 means all slots are currently filled, and per instructions we are to ignore this process

				pid_t pid;
				pid = fork();
				
				if (pid == 0) {
					execl ("user", "user", NULL);
					errorMessage(programName, "execl function failed. ");
				}
				else if (pid > 0) { //parent
					makeChild = 0; //for now, we only want to create one child, for testing purposes
					printf("PARENT: Created child %d at %d:%d\n", pid, sm->clockSecs, sm->clockNano);
					boolArray[openSlot] = true; //claim that spot
					//processesLaunched++; //these will be implemented later, so they remain here as a reminder
					//processesRunning++;
					//prepNewChild = false;
					PCT[openSlot].myPID = pid;
					//printf("Lets set child %d to 0 resources for starting out\n", pid);
					for (i = 0; i < RESOURCE_COUNT; i++) {
						PCT[openSlot].myResource[0][i] = 0; //start out with having 0 of each resource
					} //does the above hunk of code now work???
					printf("Child %d is ready to roll\n", pid);

					continue;
				}
				else {
					errorMessage(programName, "Unexpected return value from fork ");
				}
			}
		}
		//printf("increment clock\n"); 
		sm->clockNano += CLOCK_INC; //increment clock
		if (sm->clockNano >= 1000000000) { //increment the next unit
			sm->clockSecs += 1;
			sm->clockNano -= 1000000000;
		}
		
		//we check if we've received a message from any of our processes
		//printf("check if we've received something or not\n");
		int receive;
		receive = msgrcv(msqid, &message, sizeof(message), getpid(), IPC_NOWAIT); //will wait until is receives a message
		if (receive > 0) {
			printf("PARENT: ALERT! A new conversation started by child %d has just arrived!\n", message.return_address);
			//printf("Data Received from child %d to parent is : %s \n", message.return_address, message.mesg_text); //display the message
			//printf("The bool 'receive' that was sent over had a value of %d\n", message.request);
			if (message.request == true) { //we're requesting a resource
				printf("Process %d is requesting %d of resource %d\n.", message.return_address, message.resAmount, message.resID);
				
				message.mesg_type = message.return_address; //send it to the process that just sent you something
				strncpy(message.mesg_text, "parent to child", 100);
				
				if (message.resAmount <= sm->resource[message.resID][1]) { //if we are requesting a value less then or equal to that which is available
					message.mesg_value = 10; //accept request
					printf("PARENT: Looks like we can accept this request\n");
					
					for (i = 0; i < maxKidsAtATime; i++) {
						if (PCT[i].myPID == message.return_address) {
							//this is the slot we want to allocate resources to
							PCT[i].myResource[0][message.resID] += message.resAmount; //we just allocated a resource
							PCT[i].myResource[1][message.resID] = 0; //we just got some of this resource, so set our desired amount to 0. Quick to just set it then check if it has been set or not
							//printf("Decreasing value %d...\n", sm->resource[message.resID][1]);
							sm->resource[message.resID][1] -= message.resAmount; //these parameters may possibly be in the wrong order..........
						}
					}
				} else {
					message.mesg_value = 1; //deny request
					printf("PARENT: I am afraid I cannot accept this request\n");
					//mark it as desired
					for (i = 0; i < maxKidsAtATime; i++) {
						if (PCT[i].myPID == message.return_address) {
							PCT[i].myResource[1][message.resID] = message.resAmount; //say that this PID wants this resource
							break;
						}
					}
				}
				message.return_address = getpid();
				int send = msgsnd(msqid, &message, sizeof(message), 0); //send message
				if (send == -1) {
					perror("Error on msgsnd\n");
				}
				//printf("Data sent is : %s \n", message.mesg_text); // display the message 	
			
				
			} else { //we're releasing a resource
				printf("Process %d is releasing %d of resource %d\n", message.return_address, message.resAmount, message.resID);
				
				for (i = 0; i < maxKidsAtATime; i++) {

					if (PCT[i].myPID == message.return_address) {
						//printf("PARENT check 1\n");
						//this is the slot we want to deallocate resources to
						PCT[i].myResource[0][message.resID] -= message.resAmount; //we just deallocated a resource
						sm->resource[message.resID][1] += message.resAmount; //just moved resources from allocation in PCT to free in resource table

						message.mesg_type = message.return_address;
						message.return_address = getpid();
						int send = msgsnd(msqid, &message, sizeof(message), 0); //send message
						if (send == -1) {
							perror("Error on msgsnd\n");
						}
						//printf("Data sent is : %s \n", message.mesg_text); // display the messag
						//printf("PARENT check 4\n");
					}
				}
			}
		}
		
		//printf("check to see if any processes have ended\n");
		//we check to see if any of our processes have ended
		int temp = waitpid(-1, NULL, WNOHANG);
		if (temp < 0) {
			errorMessage(programName, "Unexpected result from terminating process ");
		} else if (temp > 0) {
			printf("PARENT: Process %d confirmed to have ended at %d:%d\n", temp, sm->clockSecs, sm->clockNano); //SOMETIMES GET A SEGFAULT AFTER THIS...
			//deallocate resources and continue
			printf("PARENT: ending process checkpoint 1\n");
			for (i = 0; i < maxKidsAtATime; i++) {
				printf("PARENT: ending process checkpoint 2\n");
				if (PCT[i].myPID == temp) {
					printf("PARENT: ending process checkpoint 3\n");
					printf("Currently, i = %d\n", i); //i is the correct value and makes no difference as to whehter or not the next line crashes
					printf("boolArray[%d] currently equals %d\n", i, boolArray[i]);
					boolArray[i] = false; //this line is causing a seg fault...
					printf("PARENT: Deallocating process %d from PCT\n", PCT[i].myPID);
					PCT[i].myPID = 0; //remove PID value from this slot
					for (j = 0; j < RESOURCE_COUNT; j++) {
						printf("C");
						PCT[i].myResource[0][j] = 0; //reset each resource to zero
					}
					printf("\n");
					break; //and for the moment, that's all we should need to deallocate
				}
			}
			printf("PARENT: ending process checkpoint 4\n");
			//processesRunning--;
			terminate = 1;
			printf("PARENT: ending process checkpoint 5\n");
		} //if temp == 0, nothing has ended and we simply carry on
		
		//once every blue moon, we check for a deadlock here
		//printf("check to see if we should run deadlock detection algorithm\n");
		if (sm->clockNano == 0) { //once a "second," perhaps
			//began algorithm
			printf("DEADLOCK DETECTION ALGORITHM beginning\n");
			//copy all data into "simulation" version of them. These we will manipulate without destroying the system data
			
				//struct PCB sPCT[maxKidsAtATime]; //simulated PCT used for deadlock detection
				//int sResource[3][RESOURCE_COUNT];
				
			//so first step: copy all data into these two structures, then veryify that they're correct	
			
			for (i = 0; i < maxKidsAtATime; i++) {
				sPCT[i].myPID = PCT[i].myPID;
				for (j = 0; j < RESOURCE_COUNT; j++) {
					sPCT[i].myResource[0][j] = PCT[i].myResource[0][j];
					sPCT[i].myResource[1][j] = PCT[i].myResource[1][j];
				}
			}
				
			
			for (i = 0; i < RESOURCE_COUNT; i++) {
				//printf("Saving %d is slot [%d][%d]\n", sm->resource[i][0], i, 0);
				sResource[i][0] = sm->resource[i][0];
				//printf("Saving %d is slot [%d][%d]\n", sm->resource[i][1], i, 1);
				sResource[i][1] = sm->resource[i][1];
				//printf("Saving %d is slot [%d][%d]\n", sm->resource[i][2], i, 2);
				sResource[i][2] = sm->resource[i][2];
			}
			
			printf("DEADLOCK DETECTION successful completion\n");
			
			
			
			//look at each process in order - see if you can give it the resources it wants
				//if so, simulate releasing it's resources, and mark a flag saying a change was made
				//if not, move to the next one and do nothing (?)
			//continue until we've either ended all processes or we've gone once without making a change
				//if we've ended all processes, no deadlock exists. We're done and continue
				//if we've gone once without making a change, deadlock exists. Kill off those children and release their resources. Then continue
		}
		//printf("time to restart the loop\n");
	}

	//print our resource board
	printf("PARENT: End of all loops, printout our data before closing:\n");
	for (i = 0; i < y; i++) {
		for (j = 0; j < RESOURCE_COUNT; j++) {
			printf("%d\t", sm->resource[j][i]);
		}
		printf("\n");
	}
	
	//function to print out all allocated resources
	for (p = 0; p < maxKidsAtATime; p++) {
		printf("PARENT5: Slot #%d, containing PID %d: \n", p, PCT[p].myPID);
		int q;
		for (q = 0; q < 20; q++) {
			printf("%d ", PCT[p].myResource[0][q]);
		}
		printf("\n");
	}
	
	printf("Successful end of program\n");
	
	//clean up resources
	printf("clean up check 1\n");
	int mqDestroy = msgctl(msqid, IPC_RMID, NULL); //destroy message queue
	printf("clean up check 2\n");
	int smDestroy = shmctl(shmid, IPC_RMID, NULL); //destroy shared memory
	printf("clean up check 3\n");
	if (mqDestroy == -1) {
		perror(" Error with msgctl command: Could not remove message queue ");
		exit(1);
	}	
	printf("clean up check 4\n");
	if (smDestroy == -1) {
		perror(" Error with shmctl command: Could not remove shared memory ");
		exit(1);
	}
	printf("clean up check 5\n");
	endAll(0);
	printf("clean up check 6\n");
	return 0;
}