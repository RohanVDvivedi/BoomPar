#ifndef WORKER_H
#define WORKER_H

#include<errno.h>

#include<job.h>
#include<sync_queue.h>

typedef enum worker_job_type worker_job_type;
enum worker_job_type
{
	JOB_WITH_MEMORY_MANAGED_BY_CLIENT = 0,
	JOB_WITH_MEMORY_MANAGED_BY_WORKER = 1
};

typedef enum worker_policy worker_policy;
enum worker_policy
{
	WAIT_ON_TIMEDOUT,
		// again goes to wait, after the job_queue timesout 	
		// => jobs are blockingly popped from sync job queue
		// This is policy is to be used when you want to keep on queuing to the worker, forever
		// Call worker cancel, to stop this thread, later in time

	KILL_ON_TIMEDOUT,
		// worker suicides, after the job_queue timesout
		// => jobs are blockingly popped from sync job queue
		// This is policy is to be used when you have already queued all the jobs, 
		// and expect the worker thread to kill itself, once finished

	USE_CALLBACK
		// uses callback in hopes that new jobs will be added
		// => jobs are non blockingly popped from sync job queue
		// This is policy is to be used when you want the worker to notify you using call back, 
		// to get additional jobs, once it does not find any job
};

typedef struct worker worker;
struct worker
{
	// thread_id of the worker thread
	volatile pthread_t thread_id;

	// job_queue for storing jobs to be executed by the worker
	sync_queue job_queue;

	// indicates how the worker should respond once the blocking wait has returned with a timedout
	worker_policy policy;

	// the callback function that gets called when the job_queue of the worker is empty
	// return 0 			=> 		to return and wait for new tasks
	// return (non zero) 	=> 		to safely kill the worker
	void (*job_queue_empty_callback)(worker* wrk, const void* additional_params);
	const void* additional_params;
};

worker* get_worker(unsigned long long int size, int is_bounded_queue, long long int job_queue_wait_timeout_in_microseconds, worker_policy policy, void (*job_queue_empty_callback)(worker* wrk, const void* additional_params), const void* additional_params);

void initialize_worker(worker* wrk, unsigned long long int size, int is_bounded_queue, long long int job_queue_wait_timeout_in_microseconds, worker_policy policy, void (*job_queue_empty_callback)(worker* wrk, const void* additional_params), const void* additional_params);

void deinitialize_worker(worker* wrk);

void delete_worker(worker* wrk);

int start_worker(worker* wrk);

int stop_worker(worker* wrk);

// submit function or job, returns 1 if the job was successfully submitted to the worker
// function fails and returns 0 if, the job_queue is blocking and it is full 
int submit_function_to_worker(worker* wrk, void* (*function_p)(void* input_p), void* input_p);
int submit_job_to_worker(worker* wrk, job* job_p);

#endif