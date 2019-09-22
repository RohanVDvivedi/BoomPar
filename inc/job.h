#ifndef JOB_H
#define JOB_H

#include<stdio.h>
#include<stdlib.h>

#include<pthread.h>

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

	// signified if the result of the job is set and ready to be ready by anyother thread
	int result_ready_to_read;

	// a result_ready_mutex, to protect
	pthread_mutex_t result_ready_mutex;

	// a result_ready_wait, on which other threads will wait, for the current job to complete and receive result output_p
	pthread_cond_t result_ready_wait;
};

// creates a new job
job* get_job(void* (*function_p)(void* input_p), void* input_p);

// executes the given job 
// returns 0 if the job was executed, else returns 1
int execute(job* job);

// get the result of the job, as and when available
// but the calling thread goes to wait state, until the result is available
void* get_result_or_wait_for_result(job* job_p);

// deletes job object
void delete_job(job* job_p);

#endif 