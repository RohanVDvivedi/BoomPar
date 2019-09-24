#include<executor.h>

// this is the function that will be continuously executed by the threads,
// dequeuing jobs continuously from the job_queue, to execute them
void* executors_pthread_runnable_function(void* args)
{
	// this is the executor, that is responsible for creation and execution and creation of the thread
	executor* executor_p = ((executor*)(args));

	while(1)
	{
		job* job_p = NULL;

		// lock job_queue_mutex
		pthread_mutex_lock(&(executor_p->job_queue_mutex));

		// wait while the job_queue is empty, on job_queue_empty_wait and
		// release the mutex job_queue_mutex, while we wait
		while(isQueueEmpty(executor_p->job_queue))
		{
			// wait on job_queue_empty_wait, while releasing job_queue_mutex
			pthread_cond_wait(&(executor_p->job_queue_empty_wait), &(executor_p->job_queue_mutex));
		}

		// get top job_p, and then pop
		job_p = ((job*)(get_top_queue(executor_p->job_queue)));
		pop_queue(executor_p->job_queue);

		// unlock job_queue_mutex
		pthread_mutex_unlock(&(executor_p->job_queue_mutex));

		// the thread will now execute the job that it popped
		execute(job_p);
	}
}

void create_thread(executor* executor_p)
{
	// the id to the new thread
	pthread_t* thread_id_p = ((pthread_t*)(malloc(sizeof(pthread_t))));

	// create a new thread that runs, with an executor, executor_p
	pthread_create(thread_id_p, NULL, executors_pthread_runnable_function, executor_p);

	// lock threads, while we add add a thread to the data structure
	pthread_mutex_lock(&(executor_p->threads_array_mutex));

	// if created store the thread_id in the threads array
	set_element(executor_p->threads, thread_id_p, executor_p->thread_count);

	// increment the count of the threads the executor manages
	executor_p->thread_count++;

	// unlock threads
	pthread_mutex_unlock(&(executor_p->threads_array_mutex));
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
	pthread_mutex_init(&(executor_p->job_queue_mutex), NULL);
	pthread_cond_init(&(executor_p->job_queue_empty_wait), NULL);

	executor_p->threads = get_array(maximum_threads);
	pthread_mutex_init(&(executor_p->threads_array_mutex), NULL);
	executor_p->thread_count = 0;

	// create the minimum number of threads required for functioning
	for(int i = 0; i < executor_p->minimum_threads; i++)
	{
		create_thread(executor_p);
	}

	// unset the stop variables, just to be sure :p
	executor_p->requested_to_stop_after_current_job = 0;
	executor_p->requested_to_stop_after_queue_is_empty = 0;

	return executor_p;
}

void submit(executor* executor_p, job* job_p)
{
	// lock job_queue_mutex
	pthread_mutex_lock(&(executor_p->job_queue_mutex));

	// push job_p to job_queue
	push_queue(executor_p->job_queue, job_p);

	// notify any one thread that is waiting for job_queue to have a job, on job_queue_empty_wait
	pthread_cond_signal(&(executor_p->job_queue_empty_wait));

	// update job status, from CREATED to QUEUED
	set_to_next_status(&(job_p->status));

	// unlock job_queue_mutex
	pthread_mutex_unlock(&(executor_p->job_queue_mutex));
}


// incomplete functionality, yet
void shutdown_executor(executor* executor_p, int shutdown_immediately)
{
	if(shutdown_immediately == 1)
	{
		executor_p->requested_to_stop_after_current_job = 1;
	}
	else
	{
		executor_p->requested_to_stop_after_queue_is_empty = 1;
	}
}

// incomplete functionality, yet
void wait_for_all_threads_to_complete(executor* executor_p)
{
	pthread_mutex_lock(&(executor_p->threads_array_mutex));
	while(executor_p->thread_count > 0)
	{
		pthread_join((*((pthread_t*)get_element(executor_p->threads, executor_p->thread_count - 1))), NULL);
		executor_p->thread_count--;
	}
	pthread_mutex_unlock(&(executor_p->threads_array_mutex));
}

void delete_executor(executor* executor_p)
{
	// lock the job_queue, before deleting it
	// we the executor did not create the job, so it is not our responsibility to free the job_memory
	// you will delete the jobs, by calling delete_job on each of them
	pthread_mutex_lock(&(executor_p->job_queue_mutex));
	if(executor_p->job_queue != NULL)
	{
		delete_queue(executor_p->job_queue);
		executor_p->job_queue = NULL;
	}
	pthread_mutex_unlock(&(executor_p->job_queue_mutex));

	// destory the job_queue mutexes
	pthread_mutex_destroy(&(executor_p->job_queue_mutex));
	// destory the job_queue conditional wait variables
	pthread_cond_destroy(&(executor_p->job_queue_empty_wait));

	// lock array before deleting the array
	pthread_mutex_lock(&(executor_p->threads_array_mutex));
	if(executor_p->threads != NULL)
	{
		delete_array(executor_p->threads);
		executor_p->threads = NULL;
	}
	executor_p->threads = NULL;
	pthread_mutex_unlock(&(executor_p->threads_array_mutex));

	pthread_mutex_destroy(&(executor_p->threads_array_mutex));

	free(executor_p);
}