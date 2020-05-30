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
	WAIT_FOREVER_ON_JOB_QUEUE,
		// again goes to wait, after the job_queue timesout 	
		// => jobs are blockingly popped from sync job queue
		// This is policy is to be used when you want to keep on queuing to the worker, forever
		// Call worker cancel, to stop this thread, later in time
		// in this policy the job_queue_empty_timeout_in_microseconds parameter is ignored

	KILL_ON_TIMEDOUT,
		// worker suicides, after the job_queue timesout
		// => jobs are blockingly popped from sync job queue
		// This is policy is to be used when you have already queued all the jobs, 
		// and expect the worker thread to kill itself, once finished
		// in this policy, the thread will wait for job_queue_empty_timeout_in_microseconds time before killing itself
};

pthread_t start_worker(sync_queue* job_queue, worker_policy policy, unsigned long long int job_queue_empty_timeout_in_microseconds);

int stop_worker(pthread_t thread_id);

// submit function or job, returns 1 if the job was successfully submitted to the worker
// function fails and returns 0 if, the job_queue is blocking and it is full 
int submit_function(sync_queue* job_queue, void* (*function_p)(void* input_p), void* input_p);
int submit_job(sync_queue* job_queue, job* job_p);

#endif