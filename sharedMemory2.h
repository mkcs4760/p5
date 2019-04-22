#ifndef SHAREDMEMORY2_H
#define SHAREDMEMROY2_H

struct PCB { //my structure for a process control block
	int myPID; //your local simulated pid
	int myResource[20];
};

//my structure to attach to shared memory
typedef struct file2 {
	struct PCB PCT[1]; //for testing this is 1, eventually this should be 18
} shared_memory2;

#endif

//resource works in 3 rows
	//total that exits
	//total currently allocated
	//whether or not this resource is sharable