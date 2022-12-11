#ifndef EXECUTOR_H
#define EXECUTOR_H

#include<pthread.h>

#include<sync_queue.h>
#include<job.h>

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
	// defines the type of the executor it is
	executor_type type;


	// this is queue for the jobs, that gets submitted by the client
	sync_queue job_queue;

	unsigned long long int empty_job_queue_wait_time_out_in_micro_seconds;

	unsigned int worker_count_limit;

	// keeps the current count of threads created by executor
	unsigned int active_worker_count;

	// thread_count_mutex is for protection of thread count variable
	pthread_mutex_t worker_count_mutex;

	// thread_count_wait is for waiting on thread count variable
	pthread_cond_t worker_count_until_zero_wait;


	// self explanatory variable, is set only by shutdown executor method
	volatile int requested_to_stop_after_queue_is_empty;

	// self explanatory variable, is set only by shutdown executor method
	volatile int requested_to_stop_after_current_job;


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

// creates a new executor, for the client
executor* new_executor(executor_type type, unsigned int worker_count_limit, unsigned int max_job_queue_capacity, unsigned long long int empty_job_queue_wait_time_out_in_micro_seconds, void (*worker_startup)(void* call_back_params), void (*worker_finish)(void* call_back_params), void* call_back_params);

// this function creates a job to execute function_p on input_p and set promise_for_output with the output of the job
// and enqueues this job  in the job_queue of the executor
// it returns 0, if the job was not submitted, and 1 if the job submission succeeded
// job submission fails if any of the thread has called, shutdown_executor() on this executor
// the promise_for_output may be NULL, if you do not wish to wait for completion of the job
// submission_timeout_in_microseconds is the timeout, that the executor will wait to get the job_queue have a slot for this job
// timeout = 0, implies waiting indefinitely
int submit_job(executor* executor_p, void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output, unsigned long long int submission_timeout_in_microseconds);

// the executor is asked to shutdown using this function,
// if shutdown_immediately, is set, executor asks all the threads to complete current process and exit, leaving the remaining jobs in the queue
// else executor will exit after completing all jobs in the queue
void shutdown_executor(executor* executor_p, int shutdown_immediately);

// this function, makes the calling thread to go in to wait state, untill all the threads are completed and destroyed
// after calling this function, in the current thread we are sure that, before the execution of the new line, all the threads of executor have been completed
// and that is basically means,that no new job will now be dequeued, retuns 1 if the thread_count of the gioven executor is 0 now
int wait_for_all_threads_to_complete(executor* executor_p);

// deletes the executor,
// can not and must not be called without, calling shutdown
// returns 1 if the executor was deleted
int delete_executor(executor* executor_p);

#endif