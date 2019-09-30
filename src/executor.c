#include<executor.h>

// checks if the condition to stop the threads has been reached for the given executor
// returns 1 if the condition has been reached
// call this function only after protecting it with executor_p->job_queue_mutex
int is_threads_shutdown_condition_reached(executor* executor_p)
{
	// if the threads have to stop after current job or 
	// if the threads have to stop after all the jobs in queue are completed
	return executor_p->requested_to_stop_after_current_job || 
	( isQueueEmpty(executor_p->job_queue) && executor_p->requested_to_stop_after_queue_is_empty );
}

// checks if the condition to stop the currently calling thread has been reached for the given executor
// returns 1 if the condition has been reached
// call this function only after protecting it with executor_p->job_queue_mutex
int is_calling_thread_shutdown_condition_reached(executor* executor_p, int is_first_iter)
{
	// shutdown the current thread if, there are threads waiting on the job_queue
	// for a cached thread pool, we wait timed and hence if the job queue is empty, we must exit
	// and we have to take into ac count that it is not the first iteration of the current thread
	return ( executor_p->type == CACHED_THREAD_POOL_EXECUTOR && !is_first_iter &&  executor_p->threads_waiting_on_empty_job_queue > 0 );
}

// checks if the condition to stop submission of jobs have been reached for the given executor
// returns 1 if the condition has been reached (if shutdown was called)
// call this function only after protecting it with executor_p->job_queue_mutex
int is_shutdown_called(executor* executor_p)
{
	// if the threads have to stop after current job or if the threads have to stop after all the jobs in queue are completed
	return executor_p->requested_to_stop_after_current_job || executor_p->requested_to_stop_after_queue_is_empty;
}

// this is the function that will be continuously executed by the threads,
// dequeuing jobs continuously from the job_queue, to execute them
void* executors_pthread_runnable_function(void* args)
{
	// this is the executor, that is responsible for creation and execution of the thread
	executor* executor_p = ((executor*)(args));

	// we can not keep looping if the executor type = NEW_THREAD_PER_JOB_SUBMITTED_EXECUTOR
	// because we would have to kill the thread after execution
	int keep_looping = executor_p->type == NEW_THREAD_PER_JOB_SUBMITTED_EXECUTOR ? 0 : 1;

	// to check if it is the first iter of the current thread
	int is_first_iter = 1;

	do
	{
		// lock job_queue_mutex
		pthread_mutex_lock(&(executor_p->job_queue_mutex));

		// we wait only if the queue is empty and thread's shutdown conditions are not met (neither all threads shudown condition or current threads shutdown condition)
		while(isQueueEmpty(executor_p->job_queue) && (!is_calling_thread_shutdown_condition_reached(executor_p, is_first_iter)) && (!is_threads_shutdown_condition_reached(executor_p)) )
		{
			// increment the thread count that are waiting on the empty job queue
			// since the current thread is goinf into wait state now
			executor_p->threads_waiting_on_empty_job_queue++;

			// CACHED_THREAD_POOL_EXECUTOR, we definetly go to wait, but we timedwait
			if( executor_p->type == CACHED_THREAD_POOL_EXECUTOR )
			{
				struct timespec current_time;
				clock_gettime(CLOCK_REALTIME, &current_time);;
				struct timespec wait_till;
				// do timedwait on job_queue_empty_wait, while releasing job_queue_mutex, while we wait
				pthread_cond_timedwait(&(executor_p->job_queue_empty_wait), &(executor_p->job_queue_mutex), &wait_till);
			}
			else
			{
				// wait on job_queue_empty_wait, while releasing job_queue_mutex, while we wait
				pthread_cond_wait(&(executor_p->job_queue_empty_wait), &(executor_p->job_queue_mutex));
			}

			// decrement the thread count that are waiting on the empty job queue
			// since the current thread has woken up from wait state now
			executor_p->threads_waiting_on_empty_job_queue--;
		}

		// exit the loop if the thread shutdown condition has been reached
		if( is_threads_shutdown_condition_reached(executor_p) )
		{
			// then we can not keep looping
			keep_looping = 0;

			// unlock job_queue_mutex
			pthread_mutex_unlock(&(executor_p->job_queue_mutex));
		}
		else
		{
			// get top job_p, and then pop this job from the queue
			job* job_p = ((job*)(get_top_queue(executor_p->job_queue)));
			pop_queue(executor_p->job_queue);

			// unlock job_queue_mutex
			pthread_mutex_unlock(&(executor_p->job_queue_mutex));

			// the thread will now execute the job that it popped
			execute(job_p);
		}

		// first iteration completed
		is_first_iter = 0;
	}
	while(keep_looping);

	// lock threads, while we add add a thread to the data structure
	pthread_mutex_lock(&(executor_p->thread_count_mutex));

	// decrement the number of threads of spawned by the executor
	executor_p->thread_count--;

	// broadcast to all the threads that are waiting for completion of all the threads of this executor
	if(executor_p->thread_count == 0)
	{
		pthread_cond_broadcast(&(executor_p->thread_count_wait));
	}

	// unlock threads
	pthread_mutex_unlock(&(executor_p->thread_count_mutex));

	return NULL;
}

void create_thread(executor* executor_p)
{
	// lock threads, while we add add a thread to the data structure
	pthread_mutex_lock(&(executor_p->thread_count_mutex));

	// the id to the new thread
	pthread_t thread_id_p;

	// create a new thread that runs, with an executor, executor_p
	pthread_create(&thread_id_p, NULL, executors_pthread_runnable_function, executor_p);

	// increment the count of the threads the executor manages
	executor_p->thread_count++;

	// unlock threads
	pthread_mutex_unlock(&(executor_p->thread_count_mutex));
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
		// while a CACHED_THREAD_POOL_EXECUTOR, starts with 0 thread, initially, and keeps on increasing with load
		// a NEW_THREAD_PER_JOB_SUBMITTED_EXECUTOR, creates a new thread for every new job submitted, hence minimum and maximum threads are undefined
		case NEW_THREAD_PER_JOB_SUBMITTED_EXECUTOR :
		case CACHED_THREAD_POOL_EXECUTOR :
		{
			executor_p->minimum_threads = 0;
			break;
		}
	}
	executor_p->job_queue = get_queue(maximum_threads);
	pthread_mutex_init(&(executor_p->job_queue_mutex), NULL);
	pthread_cond_init(&(executor_p->job_queue_empty_wait), NULL);

	pthread_mutex_init(&(executor_p->thread_count_mutex), NULL);
	pthread_cond_init(&(executor_p->thread_count_wait), NULL);
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

int submit(executor* executor_p, job* job_p)
{
	int was_job_queued = 0;

	// lock job_queue_mutex
	pthread_mutex_lock(&(executor_p->job_queue_mutex));

	// queue the job shutwdown was never called, on this executor
	if( !is_shutdown_called(executor_p))
	{
		// update job status, from CREATED to QUEUED
		// if this update of job status is successfull, then only we go forward and queue the job
		if(job_status_change(job_p, QUEUED))
		{
			// push job_p to job_queue
			push_queue(executor_p->job_queue, job_p);

			// notify any one thread that is waiting for job_queue to have a job, on job_queue_empty_wait
			pthread_cond_signal(&(executor_p->job_queue_empty_wait));

			was_job_queued = 1;
		}

		if(executor_p->type == NEW_THREAD_PER_JOB_SUBMITTED_EXECUTOR)
		{
			create_thread(executor_p);
		}
	}

	// unlock job_queue_mutex
	pthread_mutex_unlock(&(executor_p->job_queue_mutex));

	// return to let the caller know that he can no longet submit on this executor
	return was_job_queued;
}

// incomplete functionality, yet
void shutdown_executor(executor* executor_p, int shutdown_immediately)
{
	// lock job_queue_mutex
	pthread_mutex_lock(&(executor_p->job_queue_mutex));

	if(shutdown_immediately == 1)
	{
		executor_p->requested_to_stop_after_current_job = 1;
	}
	else
	{
		executor_p->requested_to_stop_after_queue_is_empty = 1;
	}

	// wake all the threads to recheck their wait conditions
	// broadcast to all the thread that are waiting for job_queue, because it is now never going to be filled
	pthread_cond_broadcast(&(executor_p->job_queue_empty_wait));

	// unlock job_queue_mutex
	pthread_mutex_unlock(&(executor_p->job_queue_mutex));
}

// returns if all the threads completed
int wait_for_all_threads_to_complete(executor* executor_p)
{
	// to check if the shutdown condition is reached 
	int was_shutdown_called = 0;

	// lock job_queue_mutex
	pthread_mutex_lock(&(executor_p->job_queue_mutex));

	// if the shutdown condition has not been reached, we need to exit, the wait operation
	was_shutdown_called = is_shutdown_called(executor_p);

	// unlock job_queue_mutex
	pthread_mutex_unlock(&(executor_p->job_queue_mutex));

	// return if the waiting for complettion of thread is not allowed
	if(!was_shutdown_called)
	{
		return 0;
	}

	// the return vgariable that says if all the threads, completed 
	int threads_completed = 0;

	// always lock before accesssing thread_count variable
	pthread_mutex_lock(&(executor_p->thread_count_mutex));

	while(executor_p->thread_count > 0)
	{
		// wait on threads to complete and decrement the executor thread count
		pthread_cond_wait(&(executor_p->thread_count_wait), &(executor_p->thread_count_mutex));	
	}

	threads_completed = (executor_p->thread_count == 0);

	// always unlock, once done
	pthread_mutex_unlock(&(executor_p->thread_count_mutex));

	return threads_completed;
}

int delete_executor(executor* executor_p)
{
	// executor can not be deleted if "wait for threads to complete" fails
	if(!wait_for_all_threads_to_complete(executor_p))
	{
		// return that executor not deleted
		return 0;
	}

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

	// destory the thread_count mutexes
	pthread_mutex_destroy(&(executor_p->thread_count_mutex));
	// destory the thread_count conditional wait variables
	pthread_cond_destroy(&(executor_p->thread_count_wait));

	// free the executor
	free(executor_p);

	return 1;
}