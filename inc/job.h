#ifndef JOB_H
#define JOB_H

#include<stdio.h>
#include<stdlib.h>
#include<jobstatus.h>

// you, the client is suppossed to free the input_p and output_p pointers

typedef struct job job;
struct job
{
	// the status of this job
	job_status status;

	// pointer to the input pramameter pointed to by input_p
	// that will be provided to the function for execution,
	// which will in turn produce output pointed to by pointer output_p
	void* input_p;

	// the function pointer that will be executed with input parameter
	// pointed to by pointer input_p to produce output pointed to by pointer output_p
	void* (*function_p)(void* input_p);

	// output pointed to by pointer output_p 
	// produced when the function is called with input parameters input
	void* output_p;
};

// creates a new job
job* get_job(void* (*function_p)(void* input_p), void* input_p);

// executes the given job 
// returns 0 if the job was executed, else returns 1
int execute(job* job);

// deletes job object
void delete_job(job* job_p);

#endif 