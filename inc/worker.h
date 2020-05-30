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


	// => for any type of worker can be killed, by pushing a NULL in the job queue
};

pthread_t start_worker(sync_queue* job_queue, worker_policy policy, unsigned long long int job_queue_empty_timeout_in_microseconds);

int stop_worker(pthread_t thread_id);

// submit function or job, returns 1 if the job was successfully submitted to the worker
// function fails and returns 0 if, the job_queue is blocking and it is full 
int submit_function_worker(sync_queue* job_queue, void* (*function_p)(void* input_p), void* input_p);
int submit_job_worker(sync_queue* job_queue, job* job_p);

// The function below will submit a NULL in the job_queue, this kill any one worker that dequeues it
// It returns 1, if NULL was pushed, else it will return NULL
// A WAIT_FOREVER_ON_JOB_QUEUE can be stopped only by sending this command at the end of all the jobs
int submit_stop_worker(sync_queue* job_queue);

// to discard jobs that are still remaining, in the job queue, even after the worker thread has been killed
// there would not be any jobs remaining in the queue after this call
void discard_leftover_jobs(sync_queue* job_queue);

#endif