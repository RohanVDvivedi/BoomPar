#include<executor.h>

#include<worker.h>

#include<stdio.h>
#include<stdlib.h>

static int is_shutdown_called(executor* executor_p)
{
	return executor_p->requested_to_stop_after_current_job || executor_p->requested_to_stop_after_queue_is_empty;
}

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

	pthread_mutex_lock(&(executor_p->worker_count_mutex));

		executor_p->active_worker_count--;
		if(executor_p->active_worker_count == 0)
			pthread_cond_broadcast(&(executor_p->worker_count_until_zero_wait));

	pthread_mutex_unlock(&(executor_p->worker_count_mutex));
}

// returns 1 if a new thread is created and added to the executor
static int create_worker(executor* executor_p)
{
	int is_thread_added = 0;

	pthread_mutex_lock(&(executor_p->worker_count_mutex));

	// create a new thread for the executor, only if we are not exceeding the maximum thread count for the executor
	if(executor_p->active_worker_count < executor_p->worker_count_limit)
	{
		pthread_t thread_id;
		int error_worker_creation = start_worker(&thread_id, &(executor_p->job_queue), executor_p->empty_job_queue_wait_time_out_in_micro_seconds, start_up, clean_up, executor_p);

		if(!error_worker_creation)
		{
			executor_p->active_worker_count++;
			is_thread_added = 1;
		}
	}

	pthread_mutex_unlock(&(executor_p->worker_count_mutex));

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

	pthread_mutex_init(&(executor_p->worker_count_mutex), NULL);
	pthread_cond_init(&(executor_p->worker_count_until_zero_wait), NULL);

	// unset the stop variables, just to be sure :p
	executor_p->requested_to_stop_after_current_job = 0;
	executor_p->requested_to_stop_after_queue_is_empty = 0;

	// to allow the threads to callback when they start, exit
	executor_p->worker_startup = worker_startup;
	executor_p->worker_finish = worker_finish;
	executor_p->call_back_params = call_back_params;

	// create the minimum number of threads required for functioning
	if(executor_p->type == FIXED_THREAD_COUNT_EXECUTOR)
	{
		for(unsigned int i = 0; i < executor_p->worker_count_limit; i++)
			create_worker(executor_p);
	}

	return executor_p;
}

int submit_job(executor* executor_p, void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output, unsigned long long int submission_timeout_in_microseconds)
{
	if(is_shutdown_called(executor_p))
	{
		return 0;
	}

	int was_job_queued = submit_job_worker(&(executor_p->job_queue), function_p, input_p, promise_for_output, submission_timeout_in_microseconds);

	// attempt to create new worker only for a CACHED_THREAD_POOL_EXECUTOR
	if(was_job_queued && executor_p->type == CACHED_THREAD_POOL_EXECUTOR && get_threads_waiting_on_empty_sync_queue(&(executor_p->job_queue)) == 0 && !is_empty_sync_queue(&(executor_p->job_queue)))
	{
		create_worker(executor_p);
	}

	return was_job_queued;
}

void shutdown_executor(executor* executor_p, int shutdown_immediately)
{
	if(shutdown_immediately == 1)
	{
		executor_p->requested_to_stop_after_current_job = 1;
		discard_leftover_jobs(&(executor_p->job_queue));
	}
	else
	{
		executor_p->requested_to_stop_after_queue_is_empty = 1;
	}

	pthread_mutex_lock(&(executor_p->worker_count_mutex));

	for(unsigned int i = 0; i < executor_p->active_worker_count;i++)
	{
		submit_stop_worker(&(executor_p->job_queue), 0);
	}

	pthread_mutex_unlock(&(executor_p->worker_count_mutex));
}

// returns 1, if all the threads completed
// you must call the thread shutdown, before calling this function
int wait_for_all_threads_to_complete(executor* executor_p)
{
	// return if the waiting for complettion of thread is not allowed
	if(!is_shutdown_called(executor_p))
	{
		return 0;
	}

	pthread_mutex_lock(&(executor_p->worker_count_mutex));

	while(executor_p->active_worker_count > 0)
	{
		pthread_cond_wait(&(executor_p->worker_count_until_zero_wait), &(executor_p->worker_count_mutex));	
	}

	pthread_mutex_unlock(&(executor_p->worker_count_mutex));

	return 1;
}

int delete_executor(executor* executor_p)
{
	// executor can not be deleted if "wait for threads to complete" fails
	if(!wait_for_all_threads_to_complete(executor_p))
	{
		// return that executor not deleted
		return 0;
	}

	deinitialize_sync_queue(&(executor_p->job_queue));

	pthread_mutex_destroy(&(executor_p->worker_count_mutex));
	pthread_cond_destroy(&(executor_p->worker_count_until_zero_wait));

	free(executor_p);

	return 1;
}