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
	int verbose = 0;
	
	
	//let's process the getopt arguments
	int option;
	while ((option = getopt(argc, argv, "hvs:")) != -1) {
		switch (option) {
			case 'h' :	printf("Help page for OS_Klein_project5\n"); //for h, we print helpful information about arguments to the screen
						printf("Consists of the following:\n\tTwo .c files titled oss.c and user.c\n\tTwo .h files titled messageQueue.h and sharedMemory.h\n\tOne Makefile\n\tOne README file\n\tOne version control log.\n");
						printf("The command 'make' will run the makefile and compile the program\n");
						printf("Command line arguments for master executable:\n");
						printf("\t-s\t<maxChildrenAtATime>\tdefaults to 2. Larger values have not been tested\n");
						printf("\t-v\t<NoArgument>\tInforms program to use full verbose logging. Defaults to not\n");
						printf("\t-h\t<NoArgument>\n");
						printf("Version control acomplished using github. Log obtained using command 'git log > versionLog.txt'\n");
						printf("Please note that this project was not completed. Please see README file for more details\n");
						exit(0);
						break;
			case 's' :	if (atoi(optarg) <= 19) { //for s, we set the maximum of child processes we will have at a time
							maxKidsAtATime = atoi(optarg);
						} else {
							errno = 22;
							errorMessage(programName, "Cannot allow more then 19 process at a time. "); //the parent is running, so there's already 1 process running
						}
						break;
			case 'v' :  verbose = 1;
						break;
			default :	errno = 22; //anything else is an invalid argument
						errorMessage(programName, "You entered an invalid argument. ");
		}
	}	
	
	//we need our process control table
	struct PCB PCT[maxKidsAtATime]; //here we have a table of maxNum process blocks
	int p;
	//function to set all starting values to 0 and print out all allocated resources
	for (p = 0; p < maxKidsAtATime; p++) {
		PCT[p].myPID = 0; 
		printf("PARENT0: Slot #%d, containing PID %d: \n", p, PCT[p].myPID);
		int q;
		for (q = 0; q < 20; q++) {
			PCT[p].myResource[0][q] = 0;
			PCT[p].myResource[1][q] = 0;
			printf("%d ", PCT[p].myResource[0][q]);
		}
		printf("\n");
	}
	
	int shmid;
	key_t shKey = 2094;
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
    mqKey = 1112; 
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
	
	FILE * output;
	if (verbose == 1) {
		output = fopen("verboseLog.txt", "w");
	} else {
		output = fopen("log.txt", "w");
	}
	fprintf(output, "Deadlock Detection Simulation\n\n");
	printf("Running deadlock detection simulation\n");
	printf("Please note that this may take several seconds...\n");
	
	
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
							PCT[openSlot].myResource[1][i] = 0; //also starting our wanting 0 of each resource
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
			//printf("PARENT: ALERT! A new conversation started by child %d has just arrived!\n", message.return_address);
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
							break;
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
				}	
			
				
			} else { //we're releasing a resource
				printf("PARENT: Process %d is releasing %d of resource %d\n", message.return_address, message.resAmount, message.resID);
				
				for (i = 0; i < maxKidsAtATime; i++) {

					if (PCT[i].myPID == message.return_address) {
						//this is the slot we want to deallocate resources to
						PCT[i].myResource[0][message.resID] -= message.resAmount; //we just deallocated a resource
						sm->resource[message.resID][1] += message.resAmount; //just moved resources from allocation in PCT to free in resource table

						message.mesg_type = message.return_address;
						message.return_address = getpid();
						int send = msgsnd(msqid, &message, sizeof(message), 0); //send message
						if (send == -1) {
							perror("Error on msgsnd\n");
						}
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
					printf("PARENT: Deallocating process %d from PCT++++++++++++++++++++++++++++++++++++++++\n", PCT[i].myPID);
					PCT[i].myPID = 0; //remove PID value from this slot
					for (j = 0; j < RESOURCE_COUNT; j++) {
						//add each resource back before destroying it
						sm->resource[j][1] += PCT[i].myResource[0][j];
						PCT[i].myResource[0][j] = 0; //reset each resource to zero
						
					}
					printf("\n");
					break; //and for the moment, that's all we should need to deallocate
				}
			}
			processesRunning--;
			/*if (processesLaunched > 2 && processesRunning == 0) { //let's see if this works
				terminate = 1;
			}*/
		} //if temp == 0, nothing has ended and we simply carry on
		
		//check to see if any processes can be unblocked thanks to more resources
		for (i = 0; i < maxKidsAtATime; i++) {
			for (j = 0; j < RESOURCE_COUNT; j++) {
				if (PCT[i].myResource[1][j] > 0) { //if so, we are waiting on this amount of resource j
					//check if this resource is now available...
					//printf("Looks like process %d is waiting for %d of resource %d\n", PCT[i].myPID, PCT[i].myResource[1][j], j);	
					if (PCT[i].myResource[1][j] <= sm->resource[j][1]) {
						//we can allocate these resources and release this process
						//printf("We made it here because PCT[i].myResource[1][j] equals %d, and that is less then sm->resource[1][j], which equals %d\n",PCT[i].myResource[1][j], sm->resource[1][j]);
						printf("For the record, i:%d  j:%d \n", i, j);
						message.mesg_type = PCT[i].myPID;
						message.mesg_value = 10; //accept request
						printf("PARENT: Looks like we can finally give %d it's %d shares of %d$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n", PCT[i].myPID, PCT[i].myResource[1][j], j);
						
						printf("So since we had %d, and we want %d more, we should now have %d\n", PCT[i].myResource[0][j], PCT[i].myResource[1][j], PCT[i].myResource[0][j] + PCT[i].myResource[1][j]);
						PCT[i].myResource[0][j] += PCT[i].myResource[1][j];
						PCT[i].myResource[1][j] = 0;
						printf("But that means that while we used to have %d available, we now only have %d\n", sm->resource[j][1], sm->resource[j][1] - PCT[i].myResource[1][j]);
						sm->resource[j][1] -= PCT[i].myResource[1][j];
						
						message.return_address = getpid();
						int send = msgsnd(msqid, &message, sizeof(message), 0); //send message
						if (send == -1) {
							perror("Error on msgsnd\n");
						}
						
						int k, l;
						printf("Another printout of our resource board\n"); //THIS WHOLE SECTION IS IN A TESTING PHASE
						for (k = 0; k < y; k++) {
							for (l = 0; l < RESOURCE_COUNT; l++) {
								printf("%d\t", sm->resource[l][k]);
							}
							printf("\n");
						}
						
						printf("At present, this is what our PCT looks like:\n");
						for (p = 0; p < maxKidsAtATime; p++) {
							printf("PARENT: Slot #%d, containing PID %d: \n", p, PCT[p].myPID);
							int q;
							for (q = 0; q < 20; q++) {
								printf("%d ", PCT[p].myResource[0][q]);
							}
							printf("\n");
						}
						//kill(-1*getpid(), SIGKILL);	//just for TESTING ONLY!!!
						
					} else {
						//printf("But this clearly isn't true, since sm->resource[1][j] equals %d and that is less then PCT[i].myResource[1][j], which is %d\n", sm->resource[1][j], PCT[i].myResource[1][j]);
					}
				}
			}
		
			if (PCT[i].myPID == message.return_address) {
				PCT[i].myResource[1][message.resID] = message.resAmount; //say that this PID wants this resource
				break;
			}
		}
		
		//once every blue moon, we check for a deadlock here
		//printf("check to see if we should run deadlock detection algorithm\n");
		if (sm->clockNano == 0) { //once a "second," perhaps
			printf("At present, this is what our PCT looks like:\n");
			for (p = 0; p < maxKidsAtATime; p++) {
				printf("PARENT: Slot #%d, containing PID %d: \n", p, PCT[p].myPID);
				int q;
				for (q = 0; q < 20; q++) {
					printf("%d ", PCT[p].myResource[0][q]);
				}
				printf("\n");
			}
			
			//I suppose a first step would be to detect here if anyone was waiting for anything in particular
			int numWaiting = 0;
			for (i = 0; i < maxKidsAtATime; i++) {
				for (j = 0; j < RESOURCE_COUNT; j++) {
					if (PCT[i].myResource[1][j] > 0) { //if so, we are waiting on this amount of resource j
						printf("Looks like process %d is waiting for %d of resource %d, but only %d are free right now\n", PCT[i].myPID, PCT[i].myResource[1][j], j, sm->resource[j][1]);
						printf("FYI, i and j equal %d and %d\n", i, j);
						int k, l;
						printf("Another printout of our resource board\n"); //FOR TESTING!!!!!!!!!!!!!
						for (k = 0; k < y; k++) {
							for (l = 0; l < RESOURCE_COUNT; l++) {
								printf("%d\t", sm->resource[l][k]);
							} 
							printf("\n");
						}
	
						numWaiting++;
					}
				}
			}
			int numWaitHold = numWaiting;
			printf("We found %d processes waiting\n", numWaiting);
			printf("numWaitHold, however, things there are %d\n", numWaitHold);
			if (numWaitHold > 1) {//one process cannot deadlock itself. We must have at least 2

				printf("We still believe in %d processes waiting\n", numWaiting);
				int simRes[y][RESOURCE_COUNT];
				printf("DLC 1.5\n");
				printf("We still believe in %d processes waiting\n", numWaiting);
				for (i = 0; i < 2; i++) { //hard code 2 in for now, since sharable isn't important right now
					for (j = 0; j < RESOURCE_COUNT; j++) {
						simRes[j][i] = sm->resource[j][i];
						printf("Just saved %d into simRes[%d][%d]\n", sm->resource[j][i], j, i);
						//printf("At [%d][%d] We still believe in %d processes waiting\n", i, j, numWaiting);
					}
					//printf("checkpoint\n");
				}

				printf("We still believe in %d processes waiting\n", numWaiting);
				printf("numWaitHold, however, things there are %d\n", numWaitHold);
				int k, l;
				printf("Here is our simulated resource board\n"); //FOR TESTING!!! appears to be copied successfully
				for (k = 0; k < 2; k++) {
					for (l = 0; l < RESOURCE_COUNT; l++) {
						printf("%d\t", simRes[l][k]);
					} 
					printf("\n");
				}
				
				printf("We still believe in %d processes waiting\n", numWaiting);
				int waitingProcesses[numWaitHold]; //create temporary array to store our waiting PIDs
				int counter = 0;
				
				for (i = 0; i < maxKidsAtATime; i++) { //the exact same loop, but this time we save PIDs. Loop one for each process
					bool claimed = false;
					for (j = 0; j < RESOURCE_COUNT; j++) {
						if ((PCT[i].myResource[1][j] > 0) && (claimed == false)) { //if so, we are waiting on this amount of resource j
							printf("Process %d wants %d of resource %d it seems...\n", PCT[i].myPID, PCT[i].myResource[1][j], j);
							printf("Saving %d into waitingProcesses[]\n", PCT[i].myPID);
							waitingProcesses[counter] = PCT[i].myPID;
							claimed = true; //this seems to solve the overflow problem
							counter++;
						}
					}
				}
				printf("We still believe in %d processes waiting\n", numWaiting);
				for (i = 0; i < maxKidsAtATime; i++) { //the exact same loop, but this time we save PIDs. Loop one for each process
					bool isBlocked = false;
					for (k = 0; k < numWaitHold; k++) { //compare each PID with the PIDs of blocked processes
						if (waitingProcesses[k] == PCT[i].myPID) {
							printf("Process %d is blocked\n", PCT[i].myPID);
							isBlocked = true;
						} else {
							//printf("Clearly %d != %d\n", waitingProcesses[k], PCT[i].myPID);
						}
					}
					if (isBlocked == false) {
						for (j = 0; j < RESOURCE_COUNT; j++) {
							if (PCT[i].myResource[1][j] == 0) {
								//simulate releasing this resource in simRes[j][i]
								printf("Process %d is not waiting on resource %d, so we simulate releasing all %d of its instances of resource %d\n", PCT[i].myPID, j, PCT[i].myResource[0][j], j); //can't really test this till we have 3 processes...
								simRes[j][0] += PCT[i].myResource[0][j]; //add the current amount of the resource allocated to resouce available in the simulation
							}
						}
					}
				}				

				printf("Lets verify that PCT looks the same:\n");
				for (i = 0; i < 2; i++) {
					for (j = 0; j < RESOURCE_COUNT; j++) {
						printf("%d\t", sm->resource[j][i]);
					} 
					printf("\n");
				}
				
				printf("The following are the %d processes that are waiting: ", numWaitHold);
				for (i = 0; i < numWaitHold; i++) {
					printf("%d ", waitingProcesses[i]);
				}
				printf("\n");
				
				
				//now that all nonblocked processes have simulated releasing their resources, we enter a loop
				int numStillHold = numWaitHold;
				int result = 0; //1 means we're fine, 2 means deadlock
				while (result == 0) {
					bool change = false;
					int q;
					for (q = 0; q < numWaitHold; q++) {
						//first let's take this PID, PCT[i]
						for (i = 0; i < maxKidsAtATime; i++) {
							if (PCT[i].myPID == waitingProcesses[q]) {
								printf("Looks like %d is still blocked...\n", PCT[i].myPID);
								
								//can we satisfy it's needs
								for (j = 0; j < RESOURCE_COUNT; j++) {
									if (PCT[i].myResource[1][j] > 0) {
										printf("%d is waiting on %d of %d...", PCT[i].myPID, PCT[i].myResource[1][j], j);
										
										//can we give it to him???
										if (simRes[j][1] >= PCT[i].myResource[1][j]) {
											printf("The simulation currently has %d of that, so we can satisfy it's needs!\n", simRes[j][1]);
											
											//actually release resources here and do more
											int j2;
											for (j2 = 0; j2 < RESOURCE_COUNT; j2++) {
												if (PCT[i].myResource[1][j2] == 0) {
													//simulate releasing this resource in simRes[j][i]
													printf("Process %d is not waiting on resource %d, so we simulate releasing all %d of its instances of resource %d\n", PCT[i].myPID, j, PCT[i].myResource[0][j], j); //can't really test this till we have 3 processes...
													simRes[j2][0] += PCT[i].myResource[0][j2]; //add the current amount of the resource allocated to resouce available in the simulation
												}
											}
											waitingProcesses[q] = -1; //this shows that the process has escaped the "deadlock" if you will. It'll never match a real PID
											
											numStillHold--;
											change = true;
										} else {
											printf("The simulation currently only has %d of that, so we can't satisfy this request\n", simRes[j][1]);
										}
										break;
									}
									printf("We made it here\n"); 
								}
							}
						}
					}
					if (numStillHold == 0) {
						//we've escaped - there's no deadlock
						result = 1;
						
					} else if (change == false) {
						//we have a deadlock on all remaining processes!!
						printf("We have a deadlock!\n");
						result = 2;
						//kill them and give away their resources.
						//This hunk of code does not properly kill off only the deadlocked processes
						//but we have previously correctly identified them, so it should be simple
						//Unfortunatly, with variables seemingly being unreliable due to changing values,
						//this does not usually result in the correct output
						/*for (q = 0; q < numWaitHold; q++) {
							int j2;
							for (j2 = 0; j2 < RESOURCE_COUNT; j2++) {
								if (PCT[i].myResource[1][j2] == 0) {
									//simulate releasing this resource in simRes[j][i]
									printf("Process %d is not waiting on resource %d, so we simulate releasing all %d of its instances of resource %d\n", PCT[i].myPID, j, PCT[i].myResource[0][j], j); //can't really test this till we have 3 processes...
									simRes[j2][0] += PCT[i].myResource[0][j2]; //add the current amount of the resource allocated to resouce available in the simulation
								}
							}
						}	
						for (i = 0; i < maxKidsAtATime; i++) {
							for (q = 0; 1 < numWaitHold; q++) {
								if (PCT[i].myPID == waitingProcesses[q]) { //if this is a resource we need to kill off
									int deadlockPID = PCT[i].myPID;
									PCT[i].myPID = 0; //remove PID value from this slot
									for (j = 0; j < RESOURCE_COUNT; j++) {
										//add each resource back before destroying it
										sm->resource[j][1] += PCT[i].myResource[0][j];
										PCT[i].myResource[0][j] = 0; //reset each resource to zero
									}
									printf("Terminating deadlocked child %d\n", deadlockPID);
									kill(deadlockPID, SIGKILL);
									
								}
								
							}

						}*/
						//then properly end program
					} 
				}
				terminate = 1;
			}
			
			//look at each process in order - see if you can give it the resources it wants
				//if so, simulate releasing it's resources, and mark a flag saying a change was made
				//if not, move to the next one and do nothing (?)
			//continue until we've either ended all processes or we've gone once without making a change
				//if we've ended all processes, no deadlock exists. We're done and continue
				//if we've gone once without making a change, deadlock exists. Kill off those children and release their resources. Then continue
		}
	}

	fprintf(output, "\nEnd of log\n");
	
	fclose(output);
	
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
	kill(-1*getpid(), SIGKILL);	//because the program is not complete, this is what I had to do...
	return 0;
}