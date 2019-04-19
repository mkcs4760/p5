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

int main(int argc, char *argv[]) {
	
	int y = 2;
	int x = 3;
	int resource[y][x];
	
	int i, j;
	for (i = 0; i < y; i++) {
		for (j = 0; j < x; j++) {
			resource[i][j] = 5; //a value
		}
	}
	
	for (i = 0; i < y; i++) {
		for (j = 0; j < x; j++) {
			printf("%d ", resource[i][j]);
		}
		printf("\n");
	}
	
	return 0;
}