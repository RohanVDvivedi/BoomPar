#ifndef EXECUTOR_H
#define EXECUTOR_H

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>

#include<queue.h>
#include<hashmap.h>

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

	// this variable is used only if this is a CACHED_THREAD_POOL_EXECUTOR,
	// and we want an upper bound on the count of threads
	// if this is -1, the maximum number of threads is infinite
	// if this is a FIXED_THREAD_COUNT_EXECUTOR, this is the fixed thread count
	int maximum_threads;

	int minimum_threads;

	// this is queue for the jobs, that gets submitted by the client
	queue* job_queue;

	// job_queue_mutex for protection of job_queue data structure
	pthread_mutex_t job_queue_mutex;
	
	// job_queue_empty_wait for synchronization, when there are threads waiting for empty job_queue
	pthread_cond_t job_queue_empty_wait;

	// we pick one job from the job_queue top and assign one thread from thread queue top to execute
	// then the corresponding thread gets stored in a hashmap while it is running

	// keeps the current count of threads created by executor
	unsigned long long int thread_count;

	// this is the array to store created thread ids 
	array* threads;

	// threads_array_mutex is for protection of threads data structure
	pthread_mutex_t threads_array_mutex;

	// some data structure that stores completed jobs, or their results, along with their job id and thread id
	// not thought off yet :p
};

// creates a new executor, for the client
executor* get_executor(executor_type type, int maximum_threads);

// called by client, this function enqueues a job in the job_queue of the executor
void submit(executor* executor_p, job* job_p);

// deletes the executor
void delete_executor(executor* executor_p);

#endif