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
#define MAX_TIME_BETWEEN_RESOURCE_CHANGES_SECS 1
#define RESOURCE_COUNT 20

//takes in program name and error string, and runs error message procedure
void errorMessage(char programName[100], char errorString[100]){
	char errorFinal[200];
	sprintf(errorFinal, "%s : Error : %s", programName, errorString);
	perror(errorFinal);
	exit(1);
}

//generates a random number between 1 and 3
/*int randomNum(int max) {
	int num = (rand() % (max + 1));
	return num;
}*/

int randomNum(int min, int max) {
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
	
	printf("CHILD: Welcome to the child\n");
	
	int maxKidsAtATime = 1; //this is only hear for testing. Eventually this will be replaced with a CONSTENT
	
	//set up shared memory
	int shmid;
	key_t smKey = 1094;
    shared_memory *sm; //allows us to request shared memory data
	if ((shmid = shmget(smKey, sizeof(shared_memory) + 256, IPC_CREAT | 0666)) == -1) { //connect to shared memory
        errorMessage(programName, "Function shmget failed. ");
    }
	sm = (shared_memory*) shmat(shmid, 0, 0); //attach to shared memory
	if (sm == NULL) {
		errorMessage(programName, "Function shmat failed. ");
	}
	
	//set up message queue
	int msqid; 
	key_t mqKey = 2931; 
    msqid = msgget(mqKey, 0666 | IPC_CREAT);  //msgget creates a message queue and returns identifier 
	if (msqid < 0) {
		printf("Error, msqid equals %d\n", msqid);
	} else {
		printf("CHILD: We received a msqid value of %d\n", msqid);
	}
	
	printf("CHILD: To prove we can, let's print the board here\n");
	//print our resource board
	int i, j;
	int y = 3;
	for (i = 0; i < y; i++) {
		for (j = 0; j < RESOURCE_COUNT; j++) {
			printf("%d\t", sm->resource[j][i]);
		}
		printf("\n");
	}
	
	
	int startSecs, startNano, durationSecs, durationNano, endSecs, endNano; 
	int terminate = 0;
	
	for (i = 0; i < sizeof(sm->PCT); i++) {
		if (sm->PCT[i].myPID == getpid()) {
			printf("CHILD: Cool, I found myself in the PCT is slot %d!\n", i);
			break;
		}
	}
	
	while (terminate != 1) {
		//wait a few seconds
		//decide to either request or release some resources
		//decide if we should terminate
		//rinse and repeat
		
		durationSecs = randomNum(0, MAX_TIME_BETWEEN_RESOURCE_CHANGES_SECS);
		durationNano = randomNum(0, MAX_TIME_BETWEEN_RESOURCE_CHANGES_NANO);
		startSecs = sm->clockSecs;
		startNano = sm->clockNano;
		
		endSecs = startSecs + durationSecs;
		endNano = startNano + durationNano;
		if (endNano >= 1000000000) {
			endSecs += 1;
			endNano -= 1000000000;
		}
		printf("CHILD: We will change our resource needs at %d:%d\n", endSecs, endNano); //we will try to launch our next child at endSecs:endNano
		
		while((endSecs > sm->clockSecs) || ((endSecs == sm->clockSecs) && (endNano > sm->clockNano))); //this dead wait will not work in final project
		
		printf("CHILD: We are done waiting since we could start at %d:%d and it is now %d:%d\n", endSecs, endNano, sm->clockSecs, sm->clockNano);
		//now we want to request/release a resource. To do this we need a PCT
		
		message.mesg_type = getppid(); 
		strncpy(message.mesg_text, "child to parent", 100);
		int percent = randomPercent();
		if (percent < 0) { //will be 46, but for now is 0 for EARLY TESTING!!!
			//release a resource
			printf("CHILD: I want to release a resource\n");
			message.mesg_value = 1; //1 signifies release
		} else {
			//request a resource
			message.mesg_value = 2; //2 signifies request
			printf("CHILD: I want to request a resource\n");
			//first we should find it in the PCT
			for (i = 0; i < maxKidsAtATime; i++) {
				printf("CHILD check 1\n");
				if (sm->PCT[i].myPID == getpid()) {
					printf("CHILD check 2\n");
					bool validChoice = false;
					while (validChoice == false) { //if you accidently randomly pick a resource you already have all of, just pick another
						printf("CHILD check 3\n");
						//we have found the process that wants to make the request
						//now we randomly pick the resource that it'll request
						int res = randomNum(0, 19);
						printf("We have a request for resource #%d\n", res);
						printf("I currently have %d of that resource, and the max available is %d\n", sm->PCT[i].myResource[res], sm->resource[res][1]);
						if (sm->PCT[i].myResource[res] < sm->resource[res][0]) { //if we have less then all of this resource...
							printf("CHILD check 4\n");
							validChoice = true;
							//pick a random value from 1-n where n is the literal max it can request before it requested more then could possibly exist
							int iWant = randomNum(1, sm->resource[res][0] - sm->PCT[i].myResource[res]);
							bool complete = false;
							while (complete == false) { //until I get the resources I want
								printf("CHILD check 5\n");
								//satisfy request
								//sm->resource[res][1] += iWant; //we want to increment total allocated at table, but we need oss to do that for us
								//send message back confirming request
								message.mesg_type = getppid();
								message.request = true; //set to false if we were releasing resources
								message.resID = res;
								message.resAmount = iWant;
								message.return_address = getpid();
								printf("CHILD: We request %d of resource %d\n", iWant, res);
								int send = msgsnd(msqid, &message, sizeof(message), 0);
								if (send == -1) {
									perror("Error on msgsnd\n");
								}
								// display the message 
								printf("Data send is : %s \n", message.mesg_text); 
								int receive;
								receive = msgrcv(msqid, &message, sizeof(message), getpid(), 0); //will wait until is receives a message
								if (receive < 0) {
									perror("No message received\n");
								}
								// display the message 
								printf("Data Received is : %s \n", message.mesg_text); 
								if (message.mesg_value == 10) { //PLACEHOLDER VALUE FOR EARLY TESTING
									complete = true;
									printf("CHILD check 6\n");
									printf("CHILD: We got what we wanted!!\n");
								} //else, we continue to request and request. If we can never get what we want, we will end up deadlocked
							}
						}	
					}
					printf("Looks like child %d got what it wanted\n", getpid());
				}
				//then randomly pick which resource it wants
					//if it has already maxed out on that resource, randomly choose another one
				//pick a random value from 1-n where n is the literaly max it can request before it requested more then could possibly exist.
				//update the resource table with these values,
				//send a message back confirming that the request has gone through.
					//if resources are not available, just display a message for now, idk...
			}
			
			
			
			/*if (message.mesg_value == 1) {
				//process wants to release resources
			} else if (message.mesg_value == 2) {
				//process wants to request resources
				
				//first we should find it in the PCT
				for (i = 0; i < maxKidsAtATime; i++) {
					if (PCT[i].myPID == message.return_address) {
						//we have found the process that wants to make the request
						//now we randomly pick the resource that it'll request
						int res = randomNum(0, 19);
						printf("We have a request for resource #%d\n", res);
						printf("I currently have %d of that resource, and the max available is %d\n", PCT[i].myResource[res], sm->resource[res][0]);
						if (PCT[i].myResource[res] < sm->resource[res][0]) { //if we have less then all of this resource...
							//pick a random value from 1-n where n is the literal max it can request before it requested more then could possibly exist
							int iWant = randomNum(1, sm->resource[res][0] - PCT[i].myResource[res]);
							//decide if we can satisfy this request
							if (sm->resource[res][0] - sm->resource[res][1] <= iWant) { //if we are requesting an amount less then or equal to what's available
								//satisfy request
								sm->resource[res][1] += iWant; //increment total allocated at table
								//send message back confirming request
							} else {
								//you simply have to wait...
							}
						}
					}
				//then randomly pick which resource it wants
					//if it has already maxed out on that resource, randomly choose another one
				//pick a random value from 1-n where n is the literaly max it can request before it requested more then could possibly exist.
				//update the resource table with these values,
				//send a message back confirming that the request has gone through.
					//if resources are not available, just display a message for now, idk...
			}*/
			
			
		}
		printf("WARNING! POTENTIAL LEAK FOUND!!\n");
		message.return_address = getpid();
		// msgsnd to send message 
		int send = msgsnd(msqid, &message, sizeof(message), 0);
		if (send == -1) {
			perror("Error on msgsnd\n");
		}
		// display the message 
		printf("Data send is : %s \n", message.mesg_text); 
		int receive;
		receive = msgrcv(msqid, &message, sizeof(message), getpid(), 0); //will wait until is receives a message
		if (receive < 0) {
			perror("No message received\n");
		}
		// display the message 
		printf("Data Received is : %s \n", message.mesg_text); 
		
		/*int percent = randomPercent();
		
		if (percent < 46) {
			//release a resource
		} 
		if (percent > 45) {
			//request a resource
		}*/
		
		terminate = 1;
	}
	
	printf("CHILD: Ending child %d at %d:%d\n", getpid(), sm->clockSecs, sm->clockNano);
	return 0;
}