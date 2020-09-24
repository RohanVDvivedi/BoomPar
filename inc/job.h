#ifndef JOB_H
#define JOB_H

#include<pthread.h>
#include<promise.h>

// a job can be in any of the three states
typedef enum job_status job_status;
enum job_status
{
	CREATED   = 0,
	QUEUED    = 1,
	RUNNING   = 2,
	COMPLETED = 3
};

// you, the client is suppossed to free the input_p and promise_for_output pointers of any job
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

	// promised output, has to be returned, no action is taken if you do not provide a promise pointer
	// once the job is executed, the output of the job is set in this promise
	// promise has to be provided by the user, creating a job does not implicityly create promise
	promise* promise_for_output;
};

// A job can be created/initialized with promise_for_output,
// in that case other threads can waitt to receive output of the job vy calling get_promised_result in that promise value
// on the contrary if you do not need the output of the job you can also create job without a promise by
// passing a NULL for promise_for_output attribute in the get_job/initilialize_job functions,
// and then you have effectively created a job, for which you do not need to get its output or wait for it to complete

// creates a new job
job* get_job(void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output);

// initializes a job, as if it is just created, this function can be used to create a job on stack, or inside other objects
void initialize_job(job* job_p, void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output);

// it updates the job status and returns 1 else returns 0, for an error
int job_status_change(job* job_p, job_status job_new_status);

// executes the given job, in async, on a new thread
// returns pthread_t, the thread on which the job will be executed, on error returns 0
pthread_t execute_async(job* job_p);

// executes the given job 
// returns 0 if the job was executed, else returns 1
int execute(job* job);

// once deinitialized, a job variable can be reused, by using initialize_job function
void deinitialize_job(job* job_p);

// deletes job object
void delete_job(job* job_p);

#endif 