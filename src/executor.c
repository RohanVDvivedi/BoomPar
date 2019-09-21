#include<executor.h>

// this is the function that will be continuously executed by the threads,
// dequeuing jobs continuously from the job_queue, to execute them
void* executors_pthread_runnable_function(void* args)
{
	executor* executor_p = ((executor*)(args));

	while(1)
	{
		job* job_p = NULL;

		// lock job_queue_mutex
		pthread_mutex_lock(&(executor_p->job_queue_mutex));

		// wait while the job_queue is empty, on job_queue_wait and
		// release the mutex job_queue_mutex, while we wait
		while(isQueueEmpty(executor_p->job_queue))
		{

		}

		// get top job_p, and then pop
		job_p = ((job*)(get_top(executor_p->job_queue)));
		pop(executor_p->job_queue);

		// unlock job_queue_mutex
		pthread_mutex_unlock(&(executor_p->job_queue_mutex));

		// the thread will now execute the job that it popped
		execute(job_p);
	}
}

void create_thread(executor* executor_p)
{

}

executor* get_executor(executor_type type, int maximum_threads)
{
	executor* executor_p = ((executor*)(malloc(sizeof(executor))));
	executor_p->type = type;
	executor_p->maximum_threads = maximum_threads > 0 ? maximum_threads : 1;

	switch(executor_p->type)
	{
		// for a FIXED_THREAD_COUNT_EXECUTOR, the min and max count of thread is same
		case FIXED_THREAD_COUNT_EXECUTOR :
		{
			executor_p->minimum_threads = executor_p->maximum_threads;
			break;
		}
		// while a CACHED_THREAD_POOL_EXECUTOR, starts with 1 thread, initially, and keeps on increasing with load
		case CACHED_THREAD_POOL_EXECUTOR :
		{
			executor_p->minimum_threads = 1;
			break;
		}
	}
	executor_p->job_queue = get_queue(maximum_threads);
	executor_p->threads = get_array(maximum_threads);
	for(int i = 0; i < executor_p->minimum_threads; i++)
	{
		create_thread(executor_p);
	}
	return executor_p;
}

void submit(executor* executor_p, job* job_p)
{
	// lock job_queue_mutex
	pthread_mutex_lock(&(executor_p->job_queue_mutex));

	// push job_p to job_queue
	push(executor_p->job_queue, job_p);

	// unlock job_queue_mutex
	pthread_mutex_unlock(&(executor_p->job_queue_mutex));

	// notify any thread that is waiting for job_queue to have a job, on job_queue_wait
}

void delete_executor(executor* executor_p)
{
	if(executor_p->job_queue != NULL)
	{
		delete_queue(executor_p->job_queue);
		executor_p->job_queue = NULL;
	}

	if(executor_p->threads != NULL)
	{
		delete_array(executor_p->threads);
		executor_p->threads = NULL;
	}

	free(executor_p);
}