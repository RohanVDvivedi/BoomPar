#ifndef EXECUTOR_H
#define EXECUTOR_H

#include<stdint.h>
#include<pthread.h>

#include<boompar/sync_queue.h>
#include<boompar/job.h>

typedef enum executor_type executor_type;
enum executor_type
{
	// there are fixed number of threads, that execute all your jobs
	FIXED_THREAD_COUNT_EXECUTOR,

	// the thread count is maintained by the executor itself, more tasks are submitted, more threads you see
	CACHED_THREAD_POOL_EXECUTOR
};

typedef struct executor executor;
struct executor
{
	// --------------------------------------------
	// Constants through out the life of the executor

	executor_type type;

	// only valid for type = CACHED_THREAD_POOL_EXECUTOR
	// it defines the number of microseconds that a worker will wait for job, before self destructing
	// a worker never gets killed in a FIXED_THREAD_COUNT_EXECUTOR
	uint64_t empty_job_queue_wait_time_out_in_micro_seconds;

	// maximum numbers of workers that this executor is allowed to spawn
	uint64_t worker_count_limit;

	// --------------------------------------------
	// Main job_queue managed by the executor

	// this is queue for the jobs, that gets submitted by the client
	sync_queue job_queue;

	// --------------------------------------------

	// number of active workers in the executor and 
	// a condition variable to wait for this count to reach 0, after a shutdown is called
	uint64_t active_worker_count;
	pthread_cond_t active_worker_count_until_zero_wait;

	// active_worker_count_mutex is for protection of the thread count variable
	pthread_mutex_t active_worker_count_mutex;

	// --------------------------------------------

	// number of threads currently that have called submit_job on the executor
	// a condition variable to wait for this count to reach 0, after a shutdown is called
	uint64_t submitters_count;
	pthread_cond_t submitters_count_until_zero_wait;

	// will be set if the shutdown gets called
	int shutdown_requested;

	// protects shutdown_requested flag and the submitters_count
	pthread_mutex_t shutdown_mutex;

	// --------------------------------------------
	// worker_startup and worker_finish callbacks

	// Functionality to call worker_startup worker_finish with predefined call_back_params
	// called when a new worker starts in the executor
	void (*worker_startup)(void* call_back_params);
	// called when a worker dies or gets killed in the executor
	void (*worker_finish)(void* call_back_params);
	void* call_back_params;

	/*
	*	 call back functionality can be used to
	*	1. To allocate and de allocate thread specific memory so that the jobs do not have to allocate memory
	*	2. To restart/start/close Socket connections for each thread, so that such resources can be reused
	*/
};

// here empty_job_queue_wait_time_out_in_micro_seconds can be any positive value except NON_BLOCKING, it can also be BLOCKING
executor* new_executor(executor_type type, uint64_t worker_count_limit, cy_uint max_job_queue_capacity, uint64_t empty_job_queue_wait_time_out_in_micro_seconds, void (*worker_startup)(void* call_back_params), void (*worker_finish)(void* call_back_params), void* call_back_params);

// returns 0, if the job was not submitted, and 1 if the job submission succeeded
// submission_timeout_in_microseconds is the timeout, that the executor will wait to get the job_queue to have a slot for this job
// if this function fails with a 0, then the corresponding promise (if any) will never be fulfilled, because
// a return of 0 from this function, implies that a corresponding job with the given parameters never existed
// the timeout parameter can also be BLOCKING or NON_BLOCKING
int submit_job_executor(executor* executor_p, void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output, void (*cancellation_callback)(void* input_p), uint64_t submission_timeout_in_microseconds);

// if shutdown_immediately, is set the shutdown will discard all the queued jobs,
// the executor's job_queue would be closed regardless
// this function only returns after the submitter's count has reached 0
void shutdown_executor(executor* executor_p, int shutdown_immediately);

// this function, makes the calling thread to go in to wait state, until all the workers have exited
// fails if the shutdown has not been called
int wait_for_all_executor_workers_to_complete(executor* executor_p);

// deletes the executor, fails if the shutdown has not been called
int delete_executor(executor* executor_p);

#endif