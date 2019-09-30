#ifndef EXECUTOR_H
#define EXECUTOR_H

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<errno.h>

#include<queue.h>
#include<hashmap.h>

#include<job.h>

typedef enum executor_type executor_type;
enum executor_type
{
	// there are fixed number of threads, that execute all your jobs
	FIXED_THREAD_COUNT_EXECUTOR,

	// the executor starts a new, thread for every job that is submitted
	NEW_THREAD_PER_JOB_SUBMITTED_EXECUTOR,

	// the thread count is maintained by the executor itself, more tasks are submitted, more threads you see
	CACHED_THREAD_POOL_EXECUTOR
};

typedef struct executor executor;
struct executor
{
	// defines the type of the executor it is
	executor_type type;

	// this variable is used only if this is a CACHED_THREAD_POOL_EXECUTOR,
	// and we want an upper bound on the count of threads
	// if this is -1, the maximum number of threads is infinite
	// if this is a FIXED_THREAD_COUNT_EXECUTOR, this is the fixed thread count
	unsigned long long int maximum_threads;
	unsigned long long int minimum_threads;

	// this is queue for the jobs, that gets submitted by the client
	queue* job_queue;

	// this is the number of threads that are waiting on the empty job_queue
	// this int is also protected using the job_queue_mutex, and is incremented and decremented, by the thread itself
	// as the thread go to wait, or wakes up on  job_queue_empty_wait
	int threads_waiting_on_empty_job_queue;

	unsigned long long int empty_job_queue_wait_time_out_in_micro_seconds;

	// job_queue_mutex for protection of job_queue data structure
	pthread_mutex_t job_queue_mutex;
	
	// job_queue_empty_wait for synchronization, when there are threads waiting for empty job_queue
	pthread_cond_t job_queue_empty_wait;

	// we pick one job from the job_queue top and assign one thread from thread queue top to execute

	// keeps the current count of threads created by executor
	int thread_count;

	// thread_count_mutex is for protection of thread count variable
	pthread_mutex_t thread_count_mutex;

	// thread_count_wait is for waiting on thread count variable
	pthread_cond_t thread_count_wait;

	// self explanatory variable, is set only by shutdown executor method
	int requested_to_stop_after_queue_is_empty;

	// self explanatory variable, is set only by shutdown executor method
	int requested_to_stop_after_current_job;
};

// creates a new executor, for the client
executor* get_executor(executor_type type, unsigned long long int maximum_threads, unsigned long long int empty_job_queue_wait_time_out_in_micro_seconds);

// called by client, this function enqueues a job in the job_queue of the executor
// it returns 0, if the job was not submitted, and 1 if the job submission succeeded
// job submission fails if any of the thread has called, shutdown_executor() on this executor
int submit(executor* executor_p, job* job_p);

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