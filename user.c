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
#include <stdbool.h>
#include "sharedMemory.h"
#include "messageQueue.h"

#define MAX_TIME_BETWEEN_RESOURCE_CHANGES_NANO 999999999
#define RESOURCE_COUNT 20

//takes in program name and error string, and runs error message procedure
void errorMessage(char programName[100], char errorString[100]){
	char errorFinal[200];
	sprintf(errorFinal, "%s : Error : %s", programName, errorString);
	perror(errorFinal);
	exit(1);
}

int randomNum(int min, int max) {
	if (min == max) {
		return min;
	}
	int num = (rand() % (max - min) + min);
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

	srand(time(0)); //placed here so we can generate random numbers later on
	
	printf("CHILD: Child #%d has started\n", getpid());
	
	//int maxKidsAtATime = 1; //this is only hear for testing. Eventually this will be replaced with a CONSTENT
	
	//set up shared memory
	int shmid;
	key_t smKey = 1094;
    shared_memory *sm; //allows us to request shared memory data
	if ((shmid = shmget(smKey, sizeof(shared_memory), IPC_CREAT | 0666)) == -1) { //connect to shared memory
        errorMessage(programName, "Function shmget failed. ");
    }
	sm = (shared_memory*) shmat(shmid, 0, 0); //attach to shared memory
	if (sm == NULL) {
		errorMessage(programName, "Function shmat failed. ");
	}
	
	int myResources[RESOURCE_COUNT];
	int i/*, j*/;
	for (i = 0; i < RESOURCE_COUNT; i++) {
		myResources[i] = 0; //start out with no resources
	}
	
	//set up message queue
	int msqid; 
	key_t mqKey = 2931; 
    msqid = msgget(mqKey, 0666 | IPC_CREAT);  //msgget creates a message queue and returns identifier 
	if (msqid < 0) {
		printf("Error, msqid equals %d\n", msqid);
	}
	
	int startSecs, startNano, durationSecs, durationNano, endSecs, endNano; 
	int terminate = 0;
	
	while (terminate == 0) {
		//wait a few seconds
		//decide to either request or release some resources
		//decide if we should terminate
		//rinse and repeat
		
		durationSecs = 0;
		durationNano = randomNum(0, MAX_TIME_BETWEEN_RESOURCE_CHANGES_NANO);
		startSecs = sm->clockSecs;
		startNano = sm->clockNano;
		
		endSecs = startSecs + durationSecs;
		endNano = startNano + durationNano;
		if (endNano >= 1000000000) {
			endSecs += 1;
			endNano -= 1000000000;
		}
		//printf("CHILD: We will change our resource needs at %d:%d\n", endSecs, endNano); //we will try to launch our next child at endSecs:endNano
		
		while((endSecs > sm->clockSecs) || ((endSecs == sm->clockSecs) && (endNano > sm->clockNano))); //this dead wait will not work in final project
		
		//printf("CHILD: We are done waiting since we could start at %d:%d and it is now %d:%d\n", endSecs, endNano, sm->clockSecs, sm->clockNano);
		//now we want to request/release a resource. To do this we need a PCT
			
		message.mesg_type = getppid(); 
		strncpy(message.mesg_text, "child to parent", 100);
		int percent = randomPercent();
		if (percent < 46) { //will be 46, but for now is 100 for EARLY TESTING!!!
			//release a resource
			//printf("CHILD: I want to release a resource\n");
			message.mesg_value = 1; //1 signifies release
			
			int numOurResources = 0;
			//let's decide which one we want to release
			for (i = 0; i < RESOURCE_COUNT; i++) {
				if (myResources[i] != 0) {
					numOurResources++;
				}
			} //now that we have the number of resources, we randomly pick one to release
			if (numOurResources == 0) {
				//printf("CHILD: We have no resources to release. Guess we better request\n");
				percent = 46; //set it up so we'll end up requesting instead
			} else {
				int ourPick = randomNum(1, numOurResources);
				//printf("CHILD this process currently has some of %d resources, so we've randomly decided to release some of the %dth resource\n", numOurResources, ourPick);
				int counter = 0;
				for (i = 0; i < RESOURCE_COUNT; i++) {
					if (myResources[i] != 0) {
						counter++;
						if (counter == ourPick) {
							//we want to decrease this resource
							ourPick = i;
							//printf("CHILD: I want to release some of resource %d\n", ourPick);
						}
					}
				}
				//printf("CHILD: That resource is resource #%d\n", ourPick);
				//printf("CHILD: I currently have %d of resource #%d, and the total amount of that resource is %d\n", myResources[ourPick], ourPick, sm->resource[ourPick][0]);
				//printf("test ends here...\n");
				int giveAway = randomNum(1, myResources[ourPick]);
				//printf("CHILD: I (%d) want to give away %d of resource %d\n", getpid(), giveAway, ourPick);
				
				message.mesg_type = getppid();
				message.request = false; //set to false if we were releasing resources
				message.resID = ourPick;
				message.resAmount = giveAway;
				message.return_address = getpid();
				//printf("CHILD: I release %d of resource %d\n", giveAway, ourPick);
				int send = msgsnd(msqid, &message, sizeof(message), 0);
				if (send == -1) {
					perror("Error on msgsnd\n");
				}
				//printf("CHILD: %d sent message --- awaiting response...\n", getpid());
				int receive;
				receive = msgrcv(msqid, &message, sizeof(message), getpid(), 0); //will wait until is receives a message
				if (receive < 0) {
					perror("No message received\n");
				}
				//printf("Data Received is: %d \n", message.mesg_test);
				
				//unless I'm missing something, there is no situation where we would not release a resource unless the process doesn't really have it
				//but in that case, there's an unsolveable error already, and we would terminate anyway
				
				//printf("CHILD: We released our resource1 Yay!\n");
				//release it on the child end now
				myResources[ourPick] -= giveAway;
				//printf("CHILD: We are left with %d of resource #%d\n", myResources[ourPick], ourPick);
				
			}		
		} 
		if (percent > 45) {
			//request a resource
			message.mesg_value = 2; //2 signifies request
			//printf("CHILD: I want to request a resource\n");
			bool validChoice = false;
			while (validChoice == false) { //if you accidently randomly pick a resource you already have all of, just pick another
				//we have found the process that wants to make the request
				//now we randomly pick the resource that it'll request
				int res = randomNum(0, 19);
				if (myResources[res] < sm->resource[res][0]) { //if we have less then all of this resource...
					//printf("CHILD check 4\n");
					validChoice = true;
					//pick a random value from 1-n where n is the literal max it can request before it requested more then could possibly exist
					int iWant = randomNum(1, sm->resource[res][0] - myResources[res]);
					//printf("CHILD check 4.5\n");
					bool complete = false;
					while (complete == false) { //until I get the resources I want

						message.mesg_type = getppid();
						message.request = true; //set to false if we were releasing resources
						message.resID = res;
						message.resAmount = iWant;
						message.return_address = getpid();
						//printf("CHILD: process %d requests %d of resource %d\n", getpid(), iWant, res);
						int send = msgsnd(msqid, &message, sizeof(message), 0);
						if (send == -1) {
							perror("Error on msgsnd\n");
						}
						// display the message 
						//printf("CHILD: Data send is : %s \n", message.mesg_text); 
						//printf("CHILD: %d sent message --- awaiting response...\n", getpid());
						int receive;
						receive = msgrcv(msqid, &message, sizeof(message), getpid(), 0); //will wait until is receives a message
						if (receive < 0) {
							perror("No message received\n");
						}
						// display the message 
						//printf("CHILD: Data Received is : %s \n", message.mesg_text); 
						if (message.mesg_value == 10) { //PLACEHOLDER VALUE FOR EARLY TESTING
							complete = true;
							//printf("CHILD check 6\n");
							//printf("CHILD: Process %d got %d more of resource %d\n", getpid(), iWant, res);
							myResources[res] += iWant;

						} //else, we continue to request and request. If we can never get what we want, we will end up deadlocked
					}
				}	
				//then randomly pick which resource it wants
					//if it has already maxed out on that resource, randomly choose another one
				//pick a random value from 1-n where n is the literaly max it can request before it requested more then could possibly exist.
				//update the resource table with these values,
				//send a message back confirming that the request has gone through.
					//if resources are not available, just display a message for now, idk...
			}
			
		}
		
		//now we ask ourselves if it is time to terminate or not? We'll start with a 1% chance and see what that gives us
		
		if (randomNum(1, 100) == 1) { //set to 21 percent for testing, though originally we though 1% ... DECIDE!!
			printf("CHILD %d decides it's time to terminate\n", getpid());
			terminate = 1;
		}

	}
		
	printf("This was ending allocation for process %d\n", getpid());
	for (i = 0; i < RESOURCE_COUNT; i++) {
		printf("%d ", myResources[i]);
	}
	printf("\n");	
	
	printf("CHILD: Ending child %d at %d:%d\n", getpid(), sm->clockSecs, sm->clockNano);
	return 0;
}