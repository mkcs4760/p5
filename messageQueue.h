#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <stdbool.h>

// structure for message queue 
struct mesg_buffer { 
    long mesg_type; 
    char mesg_text[100];
	int mesg_value;
	bool request;
	int resID;
	int resAmount;
	int return_address;
} message; 

#endif