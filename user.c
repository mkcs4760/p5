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
#include "messageQueue.h"

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

	printf("CHILD: Welcome to the child\n");
	
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
	
	printf("CHILD: Child %d successfully connected to shared memory at %d:%d\n", getpid(), sm->clockSecs, sm->clockNano);
	
	/*//set up message queue
	key_t mqKey = 2461; 
    int msqid; 
    //msgget creates a message queue  and returns identifier
    msqid = msgget(mqKey, 0666 | IPC_CREAT); 
	if (msqid < 0) {
		errorMessage(programName, "Error using msgget for message queue ");
	}*/
	
	key_t mqKey; 
	int msqid;
    // ftok to generate unique key 
    mqKey = 2931; //ftok("progfile", 65); 
    // msgget creates a message queue and returns identifier 
    msqid = msgget(mqKey, 0666 | IPC_CREAT);  //create the message queue
	if (msqid < 0) {
		printf("Error, msqid equals %d\n", msqid);
	} else {
		printf("CHILD: We received a msqid value of %d\n", msqid);
	}
	
	
	int parentPID = atoi(argv[1]);
	
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
		printf("CHILD: We will change our resource needs at %d:%d\n", endSecs, endNano); //we will try to launch our next child at endSecs:endNano
		
		while((endSecs > sm->clockSecs) || ((endSecs == sm->clockSecs) && (endNano > sm->clockNano))); //this dead wait will not work in final project
		
		printf("CHILD: We are done waiting since we could start at %d:%d and it is now %d:%d\n", endSecs, endNano, sm->clockSecs, sm->clockNano);
		//now we want to request/release a resource. To do this we need a PCT
		
		//message.mesg_type = parentPID; //send a message to this specific PID, and no other
		/*message.mesg_type = getppid();
		strncpy(message.mesg_text, "child to parent over", 100); //right now we're just sending "go" later on we'll have to be more specific
		message.return_address = getpid(); //tell them who sent it
				
		int send = msgsnd(msqid, &message, sizeof(message), 0);
		if (send == -1) {
			errorMessage(programName, "Error sending message via msgsnd command ");
		} else {
			printf("CHILD: Message sent from child %d to parent %d\n", getpid(), parentPID);
		}
		
		sleep(1);
		
		//now we wait to receive a message back 
		int receive;
		receive = msgrcv(msqid, &message, sizeof(message), getpid(), 0); 
		if (receive < 0) {
			errorMessage(programName, "Error receiving message via msgrcv command ");
		} else {
			printf("CHILD: Message received from parent to child\n");
			printf("CHILD: Parent said: %s\n", message.mesg_text);
		}*/
		
		///////////////////
		
		message.mesg_type = 1; 
	
  
		printf("Write Data for queue 1 : "); 
		gets(message.mesg_text); 
		printf("CHILD : Before sending from child to parent, we have a msqid value of %d\n", msqid);
		// msgsnd to send message 
		int send = msgsnd(msqid, &message, sizeof(message), 0);
		if (send == -1) {
			perror("Error on msgsnd\n");
		}
		printf("CHILD : After sending from child to parent, we have a msqid value of %d\n", msqid);
		//arguments(message queue id, a pointer to the message which here is a structure,
		//the size of the message, and finally flags can be called in the last section but if you don't want any just use "0"
	  
		// display the message 
		printf("Data send is : %s \n", message.mesg_text); 
		
		//now comes the test!!!
		//sleep(1);
		
		 // msgrcv to receive message 
		int receive;
		receive = msgrcv(msqid, &message, sizeof(message), 2, 0); //will wait until is receives a message
		if (receive < 0) {
			perror("No message received\n");
		}
		
			// display the message 
		printf("Data Received is : %s \n",  
						message.mesg_text); 
		printf("CHILD : After receiving from parent to child, we have a msqid value of %d\n", msqid);			
					///////////////////////
		
		
		
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