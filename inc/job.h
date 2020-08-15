#ifndef JOB_H
#define JOB_H

#include<pthread.h>

// a job can be in any of the three states
typedef enum job_status job_status;
enum job_status
{
	CREATED   = 0,
	QUEUED    = 1,
	RUNNING   = 2,
	COMPLETED = 3
};

// you, the client is suppossed to free the input_p and output_p pointers of any job
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

	// ***************
	// the below job variables help to wait on result for job
	// you have utilities to get, set result, aswellas wait for result
	// check if result is ready, and also you can learn the number of threads that are waiting for the result

	unsigned int threads_waiting_for_result;

	// signified if the result of the job is set and ready to be ready by anyother thread
	int result_ready_to_read;

	// a result_ready_mutex, that protect : threads_waiting_for_result, result_ready_to_read and result_ready_wait
	pthread_mutex_t result_ready_mutex;

	// a result_ready_wait, on which other threads will wait, for the current job to complete and receive result output_p
	pthread_cond_t result_ready_wait;

	// ****************
	// the below variable, is something, that has to be used by other libraries
	// if they are using the job structure, to define their task representation of quantum
	// it is not used by job.h or job.c, but it is default to 0, when you create job by get_job function
	int job_type;
};

// creates a new job
job* get_job(void* (*function_p)(void* input_p), void* input_p);

// initializes a job, as if it is just created, this function can be used to create a job on stack, or inside other objects
void initialize_job(job* job_p, void* (*function_p)(void* input_p), void* input_p);

// it updates the job status and returns 1 else returns 0, for an error
int job_status_change(job* job_p, job_status job_new_status);

// executes the given job, in async, on a new thread
// returns pthread_t, the thread on which the job will be executed, on error returns 0
pthread_t execute_async(job* job_p);

// executes the given job 
// returns 0 if the job was executed, else returns 1
int execute(job* job);

// sets the output of the job_p pointing to the data pointed by output_p
// it is thread safe, and complimentry function to get_result
void set_result(job* job_p, void* output_p);

// get the result of the job, as and when available
// but the calling thread goes to wait state, until the result is available, it is thread safe 
// thus job can be used as a promise, as in java or other programming languages
void* get_result(job* job_p);

// this function checks if the result of the job is avalable
// it returns 1 if the get_result function is capable of returning the result, without any waiting
// if it returns 0 and you call get_result, the thread might enter wait state, until the result is avalable
int check_result_ready(job* job_p);

// this function will return the number of threads that are waiting for a result to be available, at any instant
unsigned int get_thread_count_waiting_for_result(job* job_p);

// once deinitialized, a job variable can be reused, by using initialize_job function
void deinitialize_job(job* job_p);

// deletes job object
void delete_job(job* job_p);

#endif 