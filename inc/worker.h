#ifndef WORKER_H
#define WORKER_H

#include<errno.h>

#include<job.h>
#include<sync_queue.h>

typedef enum worker_policy worker_policy;
enum worker_policy
{
	WAIT_ON_TIMEDOUT,			// again goes to wait, after the job_queue timesout
	KILL_ON_TIMEDOUT,			// worker suicides, after the job_queue timesout
	USE_CALLBACK_AFTER_TIMEDOUT	// uses callback in hopes that new jobs will be added
};

typedef struct worker worker;
struct worker
{
	// thread_id of the worker thread
	pthread_t thread_id;

	// job_queue for storing jobs to be executed by the worker
	sync_queue job_queue;

	// indicates how the worker should respond once the blocking wait has returned with a timedout
	worker_policy policy;

	// the callback function that gets called when the job_queue of the worker is empty
	// return 0 			=> 		to return and wait for new tasks
	// return (non zero) 	=> 		to safely kill the worker
	int (*job_queue_empty_timedout_callback)(worker* wrk, const void* additional_params);
	const void* additional_params;
};

worker* get_worker(unsigned long long int size, int is_bounded_queue, long long int job_queue_wait_timeout_in_microseconds, worker_policy policy, int (*job_queue_empty_timedout_callback)(worker* wrk, const void* additional_params), const void* additional_params);

void initialize_worker(worker* wrk, unsigned long long int size, int is_bounded_queue, long long int job_queue_wait_timeout_in_microseconds, worker_policy policy, int (*job_queue_empty_timedout_callback)(worker* wrk, const void* additional_params), const void* additional_params);

void deinitialize_worker(worker* wrk);

void delete_worker(worker* wrk);

int start_worker(worker* wrk);

int stop_worker(worker* wrk);

int submit_function_to_worker(worker* wrk, void* (*function_p)(void* input_p), void* input_p);

int submit_job_to_worker(worker* wrk, job* job_p);

#endif