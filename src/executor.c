#include<executor.h>

// checks if the condition to stop the threads has been reached for the given executor
// returns 1 if the condition has been reached
// call this function only after protecting it with executor_p->job_queue_mutex
static int is_threads_shutdown_condition_reached(executor* executor_p)
{
	// if the threads have to stop after current job or 
	// if the threads have to stop after all the jobs in queue are completed
	return executor_p->requested_to_stop_after_current_job || 
	( isQueueEmpty(&(executor_p->job_queue)) && executor_p->requested_to_stop_after_queue_is_empty );
}

// checks if the condition to stop the currently calling thread has been reached for the given executor
// returns 1 if the condition has been reached
// call this function only after protecting it with executor_p->job_queue_mutex
static int is_calling_thread_shutdown_condition_reached(executor* executor_p, int wait_error)
{
	// shutdown the current thread if for a cached thread pool, 
	// we waited for sometime and timedout without receiving any new job
	// in that case we have to ask the current thread to exit
	return ( executor_p->type == CACHED_THREAD_POOL_EXECUTOR && wait_error == ETIMEDOUT );
}

// checks if the condition to stop submission of jobs have been reached for the given executor
// returns 1 if the condition has been reached (if shutdown was called)
// call this function only after protecting it with executor_p->job_queue_mutex
static int is_shutdown_called(executor* executor_p)
{
	// if the threads have to stop after current job or if the threads have to stop after all the jobs in queue are completed
	return executor_p->requested_to_stop_after_current_job || executor_p->requested_to_stop_after_queue_is_empty;
}

// this is the function that will be called by the workers when there are no jobs in their queues
static void* executors_pthread_runnable_function(worker* wrk, void* additional_params)
{
	// this is the executor, that is responsible for creation and execution of this worker
	executor* executor_p = ((executor*)(additional_params));

	unsigned long long int jobs_count_submitted_to_the_worker = 0;

	do
	{
		// lock job_queue_mutex
		pthread_mutex_lock(&(executor_p->job_queue_mutex));

		int wait_error = 0;

		// we wait only if the queue is empty and thread's shutdown conditions are not met (neither all threads shudown condition or current threads shutdown condition)
		while(isQueueEmpty(&(executor_p->job_queue)) && (!is_calling_thread_shutdown_condition_reached(executor_p, wait_error)) && (!is_threads_shutdown_condition_reached(executor_p)) )
		{
			// increment the thread count that are waiting on the empty job queue
			// since the current thread is goinf into wait state now
			executor_p->threads_waiting_on_empty_job_queue++;

			// CACHED_THREAD_POOL_EXECUTOR, we definetly go to wait, but we timedwait
			if( executor_p->type == CACHED_THREAD_POOL_EXECUTOR )
			{
				struct timespec current_time;
				clock_gettime(CLOCK_REALTIME, &current_time);
				unsigned long long int secs = executor_p->empty_job_queue_wait_time_out_in_micro_seconds / 1000000;
				unsigned long long int nano_secs_extra = (executor_p->empty_job_queue_wait_time_out_in_micro_seconds % 1000000) * 1000;
				struct timespec wait_till = {.tv_sec = (current_time.tv_sec + secs), .tv_nsec = (current_time.tv_nsec + nano_secs_extra)};
				// do timedwait on job_queue_empty_wait, while releasing job_queue_mutex, while we wait
				wait_error = pthread_cond_timedwait(&(executor_p->job_queue_empty_wait), &(executor_p->job_queue_mutex), &wait_till);
			}
			else
			{
				// wait on job_queue_empty_wait, while releasing job_queue_mutex, while we wait
				wait_error = pthread_cond_wait(&(executor_p->job_queue_empty_wait), &(executor_p->job_queue_mutex));
			}

			// decrement the thread count that are waiting on the empty job queue
			// since the current thread has woken up from wait state now
			executor_p->threads_waiting_on_empty_job_queue--;
		}

		// exit the loop if all the threads shutdown condition has been reached, or the condition variable waited till timeout
		if( is_calling_thread_shutdown_condition_reached(executor_p, wait_error) || is_threads_shutdown_condition_reached(executor_p) )
		{
			// then we can not keep looping
			keep_looping = 0;

			// unlock job_queue_mutex
			pthread_mutex_unlock(&(executor_p->job_queue_mutex));
		}
		else
		{
			// get top job_p, and then pop this job from the queue
			job* job_p = ((job*)(get_top_queue(&(executor_p->job_queue))));
			pop_queue(&(executor_p->job_queue));

			// unlock job_queue_mutex
			pthread_mutex_unlock(&(executor_p->job_queue_mutex));

			// the thread will now execute the job that it popped
			int current_job_type = job_p->job_type;
			execute(job_p);

			// once the job is executed we delete the job, if executor is memory managing the job
			// i.e. it was a job submitted by the client as a function
			if(current_job_type == JOB_WITH_MEMORY_MANAGED_BY_EXECUTOR)
			{
				delete_job(job_p);
			}
		}
	}

	if(jobs_count_submitted_to_the_worker == 0)
	{
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
	}

	return NULL;
}

// returns pointer to the new worker if created for the executor
static worker* create_worker(executor* executor_p)
{
	worker* wrk = NULL;

	// lock worker_count_lock, while we add add a thread to the data structure
	pthread_mutex_lock(&(executor_p->worker_count_lock));

	// create a new thread for the executor, if the executor type is NEW_THREAD_PER_JOB_SUBMITTED_EXECUTOR
	// or if we are not exceeding the maximum thread count for the executor
	if(executor_p->type == NEW_THREAD_PER_JOB_SUBMITTED_EXECUTOR || executor_p->total_workers_count < executor_p->maximum_workers)
	{
		// create a new worker
		wrk = get_worker(3, 1, long long int job_queue_wait_timeout_in_microseconds, USE_CALLBACK, void (*job_queue_empty_timedout_callback)(worker* wrk, const void* additional_params), executor_p);

		// increment the count of the threads the executor manages
		executor_p->total_workers_count++;
	}

	// unlock worker_count_lock
	pthread_mutex_unlock(&(executor_p->worker_count_lock));

	return wrk;
}

executor* get_executor(executor_type type, unsigned long long int maximum_threads, unsigned long long int empty_job_queue_wait_time_out_in_micro_seconds)
{
	executor* executor_p = ((executor*)(malloc(sizeof(executor))));
	executor_p->type = type;
	executor_p->maximum_workers = ((maximum_workers == 0) ? 1 : maximum_workers);
	executor_p->empty_job_queue_wait_time_out_in_micro_seconds = empty_job_queue_wait_time_out_in_micro_seconds;

	initialize_sync_queue(&(executor_p->job_queue), maximum_threads, 0, empty_job_queue_wait_time_out_in_micro_seconds);

	pthread_mutex_init(&(executor_p->worker_count_lock), NULL);
	executor_p->waiting_workers_count = waiting_workers_count;
	executor_p->total_workers_count = total_workers_count;

	// unset the stop variables, just to be sure :p
	executor_p->requested_to_stop_after_current_job = 0;
	executor_p->requested_to_stop_after_queue_is_empty = 0;

	// create the minimum number of threads required for functioning
	for(unsigned long long int i = 0; i < executor_p->minimum_threads; i++)
	{
		create_thread(executor_p);
	}

	return executor_p;
}

static int submit_job_internal(executor* executor_p, job* job_p)
{
	int was_job_queued = 0;

	// we can go ahead and create and queue the job, if shutdown was never called, on this executor
	if( !is_shutdown_called(executor_p))
	{
		// update job status, from CREATED to QUEUED
		// if this update of job status is successfull, then only we go forward and queue the job
		if(job_status_change(job_p, QUEUED))
		{
			// push job_p to job_queue
			was_job_queued = push_sync_queue(&(executor_p->job_queue), job_p);

			switch(executor_p->type)
			{
				case FIXED_THREAD_COUNT_EXECUTOR :
				{
					// notify any one thread that is waiting for job_queue to have a job, on job_queue_empty_wait
					// we do not care if no one wakes up now, somneone will will eventually complete their current job and pick this one up
					// do not create any new thread beacuse it is a FIXED_THREAD_COUNT_EXECUTOR
					pthread_cond_signal(&(executor_p->job_queue_empty_wait));
					break;
				}
				case NEW_THREAD_PER_JOB_SUBMITTED_EXECUTOR :
				{
					// we do not care, and so if a new job is creted and submitted, we must create a new thread for it
					create_thread(executor_p);
					break;
				}
				case CACHED_THREAD_POOL_EXECUTOR :
				{
					// if there are threads waiting for new job, we just wake some one up
					if(executor_p->threads_waiting_on_empty_job_queue > 0)
					{
						pthread_cond_signal(&(executor_p->job_queue_empty_wait));
					}
					// else since there are no threads, waiting for the job, we have to create one for this job now
					else
					{
						create_thread(executor_p);
					}
					break;
				}
			}
		}
	}

	// return to let the caller know that he can no longet submit on this executor
	return was_job_queued;
}

int submit_function(executor* executor_p, void* (*function_p)(void* input_p), void* input_p)
{
	int was_job_queued = 0;

	// create a new job with the given parameters
	job* job_p = get_job(function_p, input_p);
	job_p->job_type = JOB_WITH_MEMORY_MANAGED_BY_WORKER;

	was_job_queued = submit_job_internal(executor_p, job_p);

	if(was_job_queued == 0)
	{
		delete_job(job_p);
	}

	return was_job_queued;
}

int submit_job(executor* executor_p, job* job_p)
{
	int was_job_queued = 0;

	job_p->job_type = JOB_WITH_MEMORY_MANAGED_BY_CLIENT;
	was_job_queued = submit_job_internal(executor_p, job_p);

	return was_job_queued;
}

void shutdown_executor(executor* executor_p, int shutdown_immediately)
{
	if(shutdown_immediately == 1)
	{
		executor_p->requested_to_stop_after_current_job = 1;

		sync_queue temp_queue;
		initialize_sync_queue(&(temp_queue), executor_p->job_queue.qp.queue_size, 0, 200);
		transfer_elements_sync_queue(&(temp_queue), &(executor_p->job_queue), executor_p->job_queue.qp.queue_size * 2);

		// if we are asked to shutdown immediately, we have to clean up the jobs from the job_queue, here
		// first pop each remaining job and delete it individually, after checking if this is required
		job* top_job_p = NULL;
		while(top_job_p = pop_sync_queue_non_blocking(&(temp_queue)))
		{
			// if the job that we just popped is memory managed by the worker
			// i.e. it was submitted by the client with submit_function, we delete the job
			// else we only need to pop it 
			if(top_job_p->job_type == JOB_WITH_MEMORY_MANAGED_BY_WORKER)
			{
				delete_job(top_job_p);
			}
			// else if the client is maintaining the memeory of the job
			// we have to set the result to NULL, to make them stop waiting for the completion of this job
			else if(top_job_p->job_type == JOB_WITH_MEMORY_MANAGED_BY_CLIENT)
			{
				set_result(top_job_p, NULL);
			}
		}

		deinitialize_sync_queue(&(temp_queue));
	}
	else
	{
		executor_p->requested_to_stop_after_queue_is_empty = 1;
	}
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

	// deleting/deinitializing job_queue
	deinitialize_sync_queue(&(executor_p->job_queue));

	// destory the job_queue mutexes
	pthread_mutex_destroy(&(executor_p->worker_count_lock));

	// free the executor
	free(executor_p);

	return 1;
}