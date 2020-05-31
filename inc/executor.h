#ifndef EXECUTOR_H
#define EXECUTOR_H

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<errno.h>

#include<sync_queue.h>
#include<worker.h>
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

	// the maximum and minimum worker count allowed by the executor
	unsigned int maximum_worker_count;
	unsigned int minimum_worker_count;

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
};

// creates a new executor, for the client
executor* get_executor(executor_type type, unsigned int maximum_threads, unsigned long long int empty_job_queue_wait_time_out_in_micro_seconds);

// called by client, this function enqueues a job (with function function_p that will execute with input params input_p) in the job_queue of the executor
// it returns 0, if the job was not submitted, and 1 if the job submission succeeded
// job submission fails if any of the thread has called, shutdown_executor() on this executor
int submit_function(executor* executor_p, void* (*function_p)(void* input_p), void* input_p);

// called by client, this function enqueues a job_p in the job_queue of the executor
// it returns 0, if the job was not submitted, and 1 if the job submission succeeded
// job submission fails if any of the thread has called, shutdown_executor() on this executor
int submit_job(executor* executor_p, job* job_p);

/* ************************************

Points to note : 
1. internal submit_function also uses job, it queues a job by constructing it with the given function_p and input_p pointers
2. but a job that gets submitted by submit_function, is created and deleted by the executor itself
3. while you are suppossed to delete the job_p that you sibmit by submit_job function
4. although, it is self explanatory that in both cases you are solely responsible to delete the input parameter of the job

when to use submit_function : 
1. when you do not want to wait for the function to execute and complete. i.e. you are not going to be waiting for the job to finish
2. you are not interested in the result of the job/execution of the function
3. in these case you are suppossed to delete the input parameter in the function itself
4. we request you to submit a function_p that returns a Null, since there is noone going to wait for result or use it, and noone freeing the respective memory

when to use submit_job :
1. when you are going to be asynchronously waiting fior the job to finish. i.e. you want to wait for the result.
2. in this case you are suppossed to be calling the delete_job function on the sumbitted job
3. also in this case, you can wait for the result, after receiving the result delete the job, along with the input parameter and output result that you receive

*************************************** */

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