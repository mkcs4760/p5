#ifndef SHAREDMEMORY_H
#define SHAREDMEMROY_H

//my structure to attach to shared memory
typedef struct file {
	int clockSecs;
	int clockNano;
	int resource[3][20];
} shared_memory;

#endif