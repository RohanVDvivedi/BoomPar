#include<executor.h>

#include<worker.h>

#include<stdio.h>
#include<stdlib.h>

static void start_up(void* args)
{
	executor* executor_p = ((executor*)(args));

	// call the callback before anything else
	if(executor_p->worker_startup != NULL)
		executor_p->worker_startup(executor_p->call_back_params);
}

static void clean_up(void* args)
{
	executor* executor_p = ((executor*)(args));

	// call the callback before decrementing the active_worker_count of the executor
	if(executor_p->worker_finish != NULL)
		executor_p->worker_finish(executor_p->call_back_params);

	pthread_mutex_lock(&(executor_p->active_worker_count_mutex));

		// we decrement the active_worker_count and wake up any thread that could be waiting for the active_worker_count to reach 0
		executor_p->active_worker_count--;
		if(executor_p->active_worker_count == 0)
			pthread_cond_broadcast(&(executor_p->active_worker_count_until_zero_wait));

	pthread_mutex_unlock(&(executor_p->active_worker_count_mutex));
}

// returns 1 if a new thread is created and added to the executor
static int create_worker(executor* executor_p)
{
	int is_thread_added = 0;

	pthread_mutex_lock(&(executor_p->active_worker_count_mutex));

	// create a new thread for the executor, only if we are not exceeding the maximum thread count for the executor
	if(executor_p->active_worker_count < executor_p->worker_count_limit)
	{
		// increment the active worker count, suggesting that we will now create a worker thread
		// logically active_worker_count is the sum of actual active worker threads and the threads that are in creation
		executor_p->active_worker_count++;

		// release the mutex and then create a worker
		pthread_mutex_unlock(&(executor_p->active_worker_count_mutex));

		pthread_t thread_id;
		int error_worker_creation = start_worker(&thread_id, &(executor_p->job_queue), executor_p->empty_job_queue_wait_time_out_in_micro_seconds, start_up, clean_up, executor_p);

		pthread_mutex_lock(&(executor_p->active_worker_count_mutex));

		// now we can take lock and reevaluate based on the return value of the start_worker function's error

		if(error_worker_creation) // error starting a worker
		{
			// we decrement the active_worker_count and wake up any thread that could be waiting for the active_worker_count to reach 0
			executor_p->active_worker_count--;
			if(executor_p->active_worker_count == 0)
				pthread_cond_broadcast(&(executor_p->active_worker_count_until_zero_wait));
		}
		else
			is_thread_added = 1;
	}

	pthread_mutex_unlock(&(executor_p->active_worker_count_mutex));

	return is_thread_added;
}

executor* new_executor(executor_type type, unsigned int worker_count_limit, cy_uint max_job_queue_capacity, unsigned long long int empty_job_queue_wait_time_out_in_micro_seconds, void (*worker_startup)(void* call_back_params), void (*worker_finish)(void* call_back_params), void* call_back_params)
{
	if(worker_count_limit == 0 || max_job_queue_capacity == 0)
		return NULL;

	executor* executor_p = malloc(sizeof(executor));
	if(executor_p == NULL)
		return NULL;

	executor_p->type = type;
	executor_p->worker_count_limit = worker_count_limit;

	switch(executor_p->type)
	{
		// for a FIXED_THREAD_COUNT_EXECUTOR, the worker must wait indefinitely until it could dequeue a job, hence the 0 timeout
		case FIXED_THREAD_COUNT_EXECUTOR :
		{
			executor_p->empty_job_queue_wait_time_out_in_micro_seconds = 0;
			break;
		}
		case CACHED_THREAD_POOL_EXECUTOR :
		{
			executor_p->empty_job_queue_wait_time_out_in_micro_seconds = empty_job_queue_wait_time_out_in_micro_seconds;
			break;
		}
	}

	initialize_sync_queue(&(executor_p->job_queue), max_job_queue_capacity);

	executor_p->active_worker_count = 0;
	pthread_cond_init(&(executor_p->active_worker_count_until_zero_wait), NULL);
	pthread_mutex_init(&(executor_p->active_worker_count_mutex), NULL);

	executor_p->submitters_count = 0;
	pthread_cond_init(&(executor_p->submitters_count_until_zero_wait), NULL);

	executor_p->shutdown_requested = 0;

	pthread_mutex_init(&(executor_p->shutdown_mutex), NULL);

	// to allow the threads to callback when they start, exit
	executor_p->worker_startup = worker_startup;
	executor_p->worker_finish = worker_finish;
	executor_p->call_back_params = call_back_params;

	// create all the workers upfront for the FIXED_THREAD_COUNT_EXECUTOR
	if(executor_p->type == FIXED_THREAD_COUNT_EXECUTOR)
		for(unsigned int i = 0; i < executor_p->worker_count_limit; i++)
			create_worker(executor_p);

	return executor_p;
}

int submit_job_executor(executor* executor_p, void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output, void (*cancellation_callback)(void* input_p), unsigned long long int submission_timeout_in_microseconds)
{
	pthread_mutex_lock(&(executor_p->shutdown_mutex));
		// take the shudown_mutex lock to check if the shutdown was requested or not
		if(executor_p->shutdown_requested)
		{
			pthread_mutex_unlock(&(executor_p->shutdown_mutex));
			return 0;
		}

		// and if not called, increment the submitters_count, to signify that there is a thread waiting for a job to be pushed to the job_queue
		executor_p->submitters_count++;
	pthread_mutex_unlock(&(executor_p->shutdown_mutex));

	int was_job_queued = submit_job_worker(&(executor_p->job_queue), function_p, input_p, promise_for_output, cancellation_callback, submission_timeout_in_microseconds);

	// attempt to create new worker only for a CACHED_THREAD_POOL_EXECUTOR
	if(was_job_queued && executor_p->type == CACHED_THREAD_POOL_EXECUTOR && get_threads_waiting_on_empty_sync_queue(&(executor_p->job_queue)) == 0 && !is_empty_sync_queue(&(executor_p->job_queue)))
		create_worker(executor_p);

	// we again take the lock to decrement the submitter's count,
	// and wake up any thread that could be waiting for the submitter's count to reach 0
	pthread_mutex_lock(&(executor_p->shutdown_mutex));

		executor_p->submitters_count--;
		if(executor_p->submitters_count == 0 && executor_p->shutdown_requested)
			pthread_cond_broadcast(&(executor_p->submitters_count_until_zero_wait));

	pthread_mutex_unlock(&(executor_p->shutdown_mutex));

	return was_job_queued;
}

void shutdown_executor(executor* executor_p, int shutdown_immediately)
{
	pthread_mutex_lock(&(executor_p->shutdown_mutex));

		// early return if the shutdown was already requested earlier
		if(executor_p->shutdown_requested)
		{
			pthread_mutex_unlock(&(executor_p->shutdown_mutex));
			return;
		}

		// request shutdown
		executor_p->shutdown_requested = 1;

		// close the job_queue so no more jobs can be pushed
		// we can do this while shutdown_mutex unlocked, but we don't gain much doing that
		close_sync_queue(&(executor_p->job_queue));

		// wait for all the submitters to quit
		while(executor_p->submitters_count > 0)
			pthread_cond_wait(&(executor_p->submitters_count_until_zero_wait), &(executor_p->shutdown_mutex));

	pthread_mutex_unlock(&(executor_p->shutdown_mutex));

	// if the executor was to shutdown immediately, then discard all the jobs
	if(shutdown_immediately)
		discard_leftover_jobs(&(executor_p->job_queue));
}

// returns 1, if all the threads completed
// you must call the thread shutdown, before calling this function
int wait_for_all_executor_workers_to_complete(executor* executor_p)
{
	pthread_mutex_lock(&(executor_p->shutdown_mutex));

		// return failure, if shutdown was not requested
		if(!executor_p->shutdown_requested)
		{
			pthread_mutex_unlock(&(executor_p->shutdown_mutex));
			return 0;
		}

	pthread_mutex_unlock(&(executor_p->shutdown_mutex));

	pthread_mutex_lock(&(executor_p->active_worker_count_mutex));

		// wait for active worker count to reach 0
		while(executor_p->active_worker_count > 0)
			pthread_cond_wait(&(executor_p->active_worker_count_until_zero_wait), &(executor_p->active_worker_count_mutex));

	pthread_mutex_unlock(&(executor_p->active_worker_count_mutex));

	return 1;
}

int delete_executor(executor* executor_p)
{
	// executor can not be deleted if "wait for threads to complete" fails
	if(!wait_for_all_executor_workers_to_complete(executor_p))
		return 0;

	discard_leftover_jobs(&(executor_p->job_queue));

	deinitialize_sync_queue(&(executor_p->job_queue));

	pthread_cond_destroy(&(executor_p->active_worker_count_until_zero_wait));
	pthread_mutex_destroy(&(executor_p->active_worker_count_mutex));

	pthread_cond_destroy(&(executor_p->submitters_count_until_zero_wait));
	pthread_mutex_destroy(&(executor_p->shutdown_mutex));

	free(executor_p);

	return 1;
}