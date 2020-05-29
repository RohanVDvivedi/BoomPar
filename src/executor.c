#include<executor.h>

// checks if the condition to stop the threads has been reached for the given executor
static int is_threads_shutdown_condition_reached(executor* executor_p)
{
	return executor_p->requested_to_shutdown_as_soon_as_possible || 
	( is_empty_sync_queue(&(executor_p->job_queue)) && executor_p->requested_to_shutdown_after_jobs_complete );
}

// checks if the condition to stop submission of jobs have been reached for the given executor
static int is_shutdown_called(executor* executor_p)
{
	return executor_p->requested_to_shutdown_as_soon_as_possible || executor_p->requested_to_shutdown_after_jobs_complete;
}

// This function would be called by the workers when the workers are out of jobs in their queue
static void job_queue_empty_callback(worker* wrk, const void* additional_params)
{
	executor* executor_p = (executor*) additional_params;
}

// returns pointer to the new worker if created for the executor
static worker* create_worker(executor* executor_p)
{
	worker* wrk = NULL;

	// lock worker_count_lock, while we add add a thread to the data structure
	pthread_mutex_lock(&(executor_p->worker_count_lock));

	// create a new worker for the executor, only if we are not exceeding the maximum thread count for the executor
	if(executor_p->total_workers_count < executor_p->maximum_workers)
	{
		// create a new worker
		wrk = get_worker(8, 1, executor_p->empty_job_queue_wait_time_out_in_micro_seconds, USE_CALLBACK, job_queue_empty_callback, executor_p);

		// increment the count of the threads the executor manages
		executor_p->total_workers_count++;
	}

	// unlock worker_count_lock
	pthread_mutex_unlock(&(executor_p->worker_count_lock));

	return wrk;
}

executor* get_executor(executor_type type, unsigned long long int maximum_workers, unsigned long long int empty_job_queue_wait_time_out_in_micro_seconds)
{
	executor* executor_p = ((executor*)(malloc(sizeof(executor))));
	executor_p->type = type;
	executor_p->maximum_workers = ((maximum_workers == 0) ? 1 : maximum_workers);
	executor_p->empty_job_queue_wait_time_out_in_micro_seconds = empty_job_queue_wait_time_out_in_micro_seconds;

	initialize_sync_queue(&(executor_p->job_queue), maximum_workers, 0, empty_job_queue_wait_time_out_in_micro_seconds);

	pthread_mutex_init(&(executor_p->worker_count_lock), NULL);
	executor_p->waiting_workers_count = 0;
	executor_p->total_workers_count = 0;

	// unset the stop variables, just to be sure :p
	executor_p->requested_to_shutdown_as_soon_as_possible = 0;
	executor_p->requested_to_shutdown_after_jobs_complete = 0;

	// create the minimum number of threads required for functioning
	for(unsigned long long int i = 0; i < maximum_workers; i++)
	{
		worker* wrk = create_worker(executor_p);

		// #### TODO
		// do something with the worker

		executor_p->total_workers_count++;
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
			// #### TODO
			// push to random worker based on time and job* and job type 
			// or else
			// push job_p to job_queue
			// was_job_queued = push_sync_queue(&(executor_p->job_queue), job_p);
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
		executor_p->requested_to_shutdown_as_soon_as_possible = 1;

		// create a temporary queue
		sync_queue temp_queue;
		initialize_sync_queue(&(temp_queue), executor_p->job_queue.qp.queue_size, 0, 0);

		// transfer all elements to temporary queue
		while(!is_empty_sync_queue(&(executor_p->job_queue)))
		{
			transfer_elements_sync_queue(&(temp_queue), &(executor_p->job_queue), executor_p->maximum_workers);
		}

		// delete each left over job, or set its result to NULL
		while(!is_empty_sync_queue(&(temp_queue)))
		{
			job* job_p = ((job*)(pop_sync_queue_non_blocking(&(temp_queue))));

			// if the job that we just popped is memory managed by the worker
			// i.e. it was submitted by the client with submit_function, we delete the job
			if(job_p->job_type == JOB_WITH_MEMORY_MANAGED_BY_WORKER)
			{
				delete_job(job_p);
			}
			// else if the client is maintaining the memory of the job
			// we have to set the result to NULL, to make them stop waiting for the completion of this job
			else if(job_p->job_type == JOB_WITH_MEMORY_MANAGED_BY_CLIENT)
			{
				set_result(job_p, NULL);
			}
		}

		deinitialize_sync_queue(&(temp_queue));
	}
	else
	{
		executor_p->requested_to_shutdown_after_jobs_complete = 1;
	}
}

// returns if all the threads completed
int wait_for_all_threads_to_complete(executor* executor_p)
{
	// #### TODO
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