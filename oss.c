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

int randomNum() {
	int num = (rand() % (10) + 1);
	return num;
}

int main(int argc, char *argv[]) {
	srand(time(0)); //placed here so we can generate random numbers later on
	printf("Welcome to project 5\n");
	
	int x = 20;
	int y = 3;
	int resource[y][x];
	
	int i, j;
	for (i = 0; i < y; i++) {
		for (j = 0; j < x; j++) {
			if (i == 0) {
				resource[j][i] = randomNum(); //set the maximum number of this resource available to the system. These numbers do not change
			} else if ( i == 1) {
				resource[j][i] = resource[j][0]; //set the current number of available instances of this resource. This number does change
			} else {
				if (randomNum() < 3) {
					resource[j][i] = 1; //mark this resource as a sharable resource
				} else {
					resource[j][i] = 0; //mark this resource as a nonsharable resource
				}
			}
		}
	}
	
	//print our resource board
	for (i = 0; i < y; i++) {
		for (j = 0; j < x; j++) {
			printf("%d\t", resource[j][i]);
		}
		printf("\n");
	}
	
	return 0;
}