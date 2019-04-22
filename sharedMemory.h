#ifndef SHAREDMEMORY_H
#define SHAREDMEMROY_H

struct PCB { //my structure for a process control block
	int myPID; //your local simulated pid
	int myResource[20];
};

//my structure to attach to shared memory
typedef struct file {
	int clockSecs;
	int clockNano;
	int resource[3][20];
	struct PCB PCT[1]; //for testing this is 1, eventually this should be 18
} shared_memory;

#endif


//resource works in 3 rows
	//total that exits
	//total currently allocated
	//whether or not this resource is sharable