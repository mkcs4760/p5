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

#define CLOCK_INC 10000
#define RESOURCE_COUNT 20
#define MAX_TIME_BETWEEN_NEW_PROCESSES_NS 500000000


struct PCB {
	int myPID; //your local simulated pid
	int myResource[2][RESOURCE_COUNT]; //[0] is what it has, [1] is what it wants
	/*int waitRes; //the resource this process is waiting on
	int waitAmount; //th*/
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

//checks our PCT for an open slot to save the process. Returns -1 if none exist
int checkForOpenSlot(struct PCB PCT[], int maxKidsAtATime) {
	int i;
	for (i = 0; i < maxKidsAtATime; i++) {
		if (PCT[i].myPID == 0) {
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
	int maxKidsAtATime = 2; //for now, this is our test value. This will eventually be up to 18, depending on command line arguments
	
	//we need our process control table
	struct PCB PCT[maxKidsAtATime]; //here we have a table of maxNum process blocks
	//struct PCB sPCT[maxKidsAtATime]; //simulated PCT used for deadlock detection
	//int sResource[3][RESOURCE_COUNT];
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
	//int makeChild = 1;
	bool newChildPrepped = false;
	int processesLaunched = 0;
	int processesRunning = 0;
	int durationNano, startSeconds, startNano, stopSeconds, stopNano;
	
	while (terminate != 1) {
		//printf("Start of while loop\n");
		if (newChildPrepped == false) { //let's prep a new child
			newChildPrepped = true;
			durationNano = randomNum(1, MAX_TIME_BETWEEN_NEW_PROCESSES_NS);
			startSeconds = sm->clockSecs;
			startNano = sm->clockNano;
			
			stopSeconds = startSeconds; //duration seconds is 0, so don't bother making a variable for it
			stopNano = startNano + durationNano;
			if (stopNano >= 1000000000) {
				stopSeconds += 1;
				stopNano -= 1000000000;
			}
			//we will try to launch our next child at stopSeconds:stopNano
		} else if ((stopSeconds < sm->clockSecs) || ((stopSeconds == sm->clockSecs) && (stopNano < sm->clockNano))) {
			if (processesLaunched < 3) { //for testing...
				
				int openSlot = checkForOpenSlot(PCT, maxKidsAtATime);
				if (openSlot != -1) { //a return value of -1 means all slots are currently filled, and per instructions we are to ignore this process

					pid_t pid;
					pid = fork();
					
					if (pid == 0) {
						execl ("user", "user", NULL);
						errorMessage(programName, "execl function failed. ");
					}
					else if (pid > 0) { //parent
						newChildPrepped = false;
						printf("PARENT: Created child %d at %d:%d\n", pid, sm->clockSecs, sm->clockNano);
						processesLaunched++; //these will be implemented later, so they remain here as a reminder
						processesRunning++;
						PCT[openSlot].myPID = pid;
						//printf("Lets set child %d to 0 resources for starting out\n", pid);
						for (i = 0; i < RESOURCE_COUNT; i++) {
							PCT[openSlot].myResource[0][i] = 0; //start out with having 0 of each resource
						} //does the above hunk of code now work???
						continue;
					}
					else {
						errorMessage(programName, "Unexpected return value from fork ");
					}
				}
			}
		
		}
		

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
			if (message.request == true) { //we're requesting a resource
				printf("PARENT: Process %d is requesting %d of resource %d\n", message.return_address, message.resAmount, message.resID);
				
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
							sm->resource[message.resID][1] -= message.resAmount; //these parameters may possibly be in the wrong order..........
						}
					}
					
					message.return_address = getpid();
					int send = msgsnd(msqid, &message, sizeof(message), 0); //send message
					if (send == -1) {
						perror("Error on msgsnd\n");
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
					//don't send a message back, so we don't unpause it
					//kill(-1*getpid(), SIGKILL);	//just for TESTING ONLY!!!
					
					
					
					//MORE NEEDS TO GO HERE
					//right now, the process needs to go into a blocked queue, and wait for its resource to become available.
					//Once it's available, it needs to be claimed and a message sent back to unapuse the child
					
				}

				//printf("Data sent is : %s \n", message.mesg_text); // display the message 	
			
				
			} else { //we're releasing a resource
				printf("PARENT: Process %d is releasing %d of resource %d\n", message.return_address, message.resAmount, message.resID);
				
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
		
		//we check to see if any of our processes have ended
		int temp = waitpid(-1, NULL, WNOHANG);
		if ((temp < 0) && (errno != 10)) { //a cheap escape for when there are no processes in the system...
			printf("Why does waitpid equal %d?\n", temp);
			errorMessage(programName, "Unexpected result from terminating process ");
		} else if (temp > 0) {
			printf("PARENT: Process %d confirmed to have ended at %d:%d\n", temp, sm->clockSecs, sm->clockNano); //SOMETIMES GET A SEGFAULT AFTER THIS...
			//deallocate resources and continue
			//printf("PARENT: ending process checkpoint 1\n");
			for (i = 0; i < maxKidsAtATime; i++) {
				//printf("PARENT: ending process checkpoint 2\n");
				if (PCT[i].myPID == temp) {
					//printf("PARENT: ending process checkpoint 3\n");
					//printf("Currently, i = %d\n", i); //i is the correct value and makes no difference as to whehter or not the next line crashes
					//printf("boolArray[%d] currently equals %d\n", i, boolArray[i]);
					//boolArray[i] = false; //this line is causing a seg fault...
					printf("PARENT: Deallocating process %d from PCT\n", PCT[i].myPID);
					PCT[i].myPID = 0; //remove PID value from this slot
					for (j = 0; j < RESOURCE_COUNT; j++) {
						PCT[i].myResource[0][j] = 0; //reset each resource to zero
					}
					printf("\n");
					break; //and for the moment, that's all we should need to deallocate
				}
			}
			//printf("PARENT: ending process checkpoint 4\n");
			processesRunning--;
			if (processesLaunched > 2 && processesRunning == 0) { //let's see if this works
				terminate = 1;
			}
			//printf("PARENT: ending process checkpoint 5\n");
		} //if temp == 0, nothing has ended and we simply carry on
		
		//check to see if any processes can be unblocked thanks to more resources
		/*for (i = 0; i < maxKidsAtATime; i++) {
			for (j = 0; j < RESOURCE_COUNT; j++) {
				if (PCT[i].myResource[i][j] > 0) { //if so, we are waiting on this amount of resource j
					//check if this resource is now available...
					if (sm->resource[1][j] >= PCT[i].myResource[i][j]) {
						//we can allocate these resources and release this process
						//printf("We made it here because PCT[i].myResource[i][j] equals %d, and that is less then sm->resource[1][j], which equals %d\n",PCT[i].myResource[i][j], sm->resource[1][j]);
						
						message.mesg_type = PCT[i].myPID;
						message.mesg_value = 10; //accept request
						printf("PARENT: Looks like we can finally give %d it's %d shares of %d\n", PCT[i].myPID, PCT[i].myResource[i][j], j);
						
						
						for (i = 0; i < maxKidsAtATime; i++) {
							if (PCT[i].myPID == message.return_address) {
								//this is the slot we want to allocate resources to
								PCT[i].myResource[0][message.resID] += message.resAmount; //we just allocated a resource
								PCT[i].myResource[1][message.resID] = 0; //we just got some of this resource, so set our desired amount to 0. Quick to just set it then check if it has been set or not
								sm->resource[message.resID][1] -= message.resAmount; //these parameters may possibly be in the wrong order..........
							}
						}
						
						message.return_address = getpid();
						int send = msgsnd(msqid, &message, sizeof(message), 0); //send message
						if (send == -1) {
							perror("Error on msgsnd\n");
						}
						
					}
					
					
				}
			
			}
		
			if (PCT[i].myPID == message.return_address) {
				PCT[i].myResource[1][message.resID] = message.resAmount; //say that this PID wants this resource
				break;
			}
		}*/
		
		//once every blue moon, we check for a deadlock here
		//printf("check to see if we should run deadlock detection algorithm\n");
		if (sm->clockNano == 0) { //once a "second," perhaps
			//began algorithm
			printf("\tDEADLOCK DETECTION ALGORITHM ...\n");
			
			printf("At present, this is what our PCT looks like:\n");
			for (p = 0; p < maxKidsAtATime; p++) {
				printf("PARENT: Slot #%d, containing PID %d: \n", p, PCT[p].myPID);
				int q;
				for (q = 0; q < 20; q++) {
					printf("%d ", PCT[p].myResource[0][q]);
				}
				printf("\n");
			}
			
			/*sprintf("DEADLOCK DETECTION ALGORITHM beginning\n");
			//copy all data into "simulation" version of them. These we will manipulate without destroying the system data
			
				//struct PCB sPCT[maxKidsAtATime]; //simulated PCT used for deadlock detection
				//int sResource[3][RESOURCE_COUNT];
				
			//so first step: copy all data into these two structures, then veryify that they're correct	
			printf("DDA check 1\n");
			for (i = 0; i < maxKidsAtATime; i++) {
				printf("DDA check 1.2\n");
				printf("Can we print sPCT? ");
				printf("%d\n", sPCT[i].myPID);
				printf("What about PCT? ");
				printf("%d\n", PCT[i].myPID);
				printf("Can we run a line with both together?\n");
				sPCT[i].myPID = PCT[i].myPID;
				printf("DDA check 1.3\n");
				for (j = 0; j < RESOURCE_COUNT; j++) {
					printf("DDA check 1.4\n");
					sPCT[i].myResource[0][j] = PCT[i].myResource[0][j];
					printf("DDA check 1.5\n");
					sPCT[i].myResource[1][j] = PCT[i].myResource[1][j];
					printf("DDA check 1\n.6");
				}
			}
			printf("DDA check 2\n");	
			
			for (i = 0; i < RESOURCE_COUNT; i++) {
				//printf("Saving %d is slot [%d][%d]\n", sm->resource[i][0], i, 0);
				sResource[i][0] = sm->resource[i][0];
				//printf("Saving %d is slot [%d][%d]\n", sm->resource[i][1], i, 1);
				sResource[i][1] = sm->resource[i][1];
				//printf("Saving %d is slot [%d][%d]\n", sm->resource[i][2], i, 2);
				sResource[i][2] = sm->resource[i][2];
			}
			printf("DDA check 3\n");
			printf("DEADLOCK DETECTION successful completion\n");
			
			*/
			
			//look at each process in order - see if you can give it the resources it wants
				//if so, simulate releasing it's resources, and mark a flag saying a change was made
				//if not, move to the next one and do nothing (?)
			//continue until we've either ended all processes or we've gone once without making a change
				//if we've ended all processes, no deadlock exists. We're done and continue
				//if we've gone once without making a change, deadlock exists. Kill off those children and release their resources. Then continue
		}
	}

	
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
	printf("Successful end of program\n");
	return 0;
}