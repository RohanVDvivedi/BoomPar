#define JOB_H
#ifndef JOB_H

#include<jobstatus.h>

typedef struct job job;
struct job
{
	// input that will be provided to the function for execution, which will in turn produce output
	void* input;

	// the function that will be executed with input parameters input to produce output
	void* (*function)(void* input);

	// output produced when the function is called with input parameters input
	void* output;
};

#endif 