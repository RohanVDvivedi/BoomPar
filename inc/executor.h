#ifndef EXECUTOR_H
#define EXECUTOR_H

#include<stdio.h>
#include<stdlib.h>

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

	// this is queue for the jobs, that gets submitted by the client
	queue* job_queue;

	// job_queue_mutex for protection of job_queue data structure
	
	// job_queue_wait for synchronization, when there are threads waiting for empty job_queue

	// this is queue for the idle threads
	queue* idle_thread_queue;

	// idle_thread_queue_mutex for protection of idle_thread_queue data structure

	// idle_thread_queue_wait for synchronization, when there are threads waiting for empty idle_thread_queue

	// we pick one job from the job_queue top and assign one thread from thread queue top to execute
	// then the corresponding thread gets stored in a hashmap while it is running

	// this is the hashmap from thread id to thread object references, for all running threads 
	hashmap* running_threads;

	// mutex for protection of running_threads data structure

	// some data structure that stores completed jobs, or their results, along with their job id and thread id
	// not thought off yet :p
};

// creates a new executor, for the client
executor* get_executor(executor_type type, int maximum_threads);

// called by client, this function enqueues a job in the job_queue of the executor
void submit(executor* executor_p, job* job_p);

// a job is popped from the queue and the thread from the idle_thread_queue
// assigns them to eachother, store the thread in hashmap against its id
// this is called by the executor, to run a job by some thread
void assign_and_run(executor* executor_p, /*thread* thread_p,*/ job* job_p);

// this is the function called by the thread, whose execution has just been completed
// it removed the thread, with thread_is from the running_threads hash and push it in idle_thread_queue 
void completed_job(int thread_id);

// deletes the executor
void delete_executor(executor* executor_p);

#endif