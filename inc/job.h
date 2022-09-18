#ifndef JOB_H
#define JOB_H

#include<pthread.h>
#include<promise.h>

#include<ucontext.h>

// a job can be in any of the three states
typedef enum job_status job_status;
enum job_status
{
	CREATED   = 0,
	QUEUED    = 1,
	WAITING   = 2,
	RUNNING   = 3,
	COMPLETED = 4
};

// A job is not a concurrent object hence all accesses to the job object must be done by only 1 (worker) thread, at a time

// you, the client is suppossed to free the input_p and promise_for_output pointers of any job
typedef struct job job;
struct job
{
	// pointer to the input pramameter pointed to by input_p
	// that will be provided to the function for execution,
	// which will in turn produce output pointed to by pointer output_p
	void* input_p;

	// the function pointer that will be executed with input parameter
	// pointed to by pointer input_p to produce output pointed to by pointer output_p
	void* (*function_p)(void* input_p);

	// stack size of the stack to run the job on
	// to run the job of the calling thread's stack make this value 0
	size_t stack_size;

	// promised output, has to be returned, no action is taken if you do not provide a promise pointer
	// once the job is executed, the output of the job is set in this promise
	// promise has to be provided by the user, creating a job does not implicityly create promise
	promise* promise_for_output;

	// the above attributed of job are constant and do not change during the lifetime of the job

	// the status of this job
	job_status status;

	// context of the job
	ucontext_t job_context;

	// context of the thread that ran this job
	ucontext_t thread_context;
};

// A job can be created/initialized with promise_for_output,
// in that case you can wait to receive output of the job by calling get_promised_result using that promise
// on the contrary if you do not need the output of the job you can also create job without a promise by
// passing a NULL for promise_for_output attribute in the get_job/initilialize_job functions,
// and then you have effectively created a job, for which you do not need to get its output or wait for it to complete

// every job has its own stack, and stack_size refers to the bytes that will be allocated for the job for its stack

// creates a new job
job* new_job(void* (*function_p)(void* input_p), void* input_p, size_t stack_size, promise* promise_for_output);

// initializes a job, as if it is just created, this function can be used to create a job on stack, or inside other objects
void initialize_job(job* job_p, void* (*function_p)(void* input_p), void* input_p, size_t stack_size, promise* promise_for_output);

// it updates the job status and returns 1 else returns 0, for an error
int job_status_change(job* job_p, job_status job_new_status);

// executes the given job 
// returns 1 if the job was executed, else returns 0
// this function may be called only by a worker
// after this function returns the job can be completed or waiting
int execute(job* job);

// once deinitialized, a job variable can be reused, by using initialize_job function
void deinitialize_job(job* job_p);

// deletes job object
void delete_job(job* job_p);

#endif 