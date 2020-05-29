#include<worker.h>

worker* get_worker(unsigned long long int size, int is_bounded_queue, long long int job_queue_wait_timeout_in_microseconds, worker_policy policy, void (*job_queue_empty_callback)(worker* wrk, const void* additional_params), const void* additional_params)
{
	worker* wrk = (worker*) malloc(sizeof(worker));
	initialize_worker(wrk, size, is_bounded_queue, job_queue_wait_timeout_in_microseconds, policy, job_queue_empty_callback, additional_params);
	return wrk;
}

void initialize_worker(worker* wrk, unsigned long long int size, int is_bounded_queue, long long int job_queue_wait_timeout_in_microseconds, worker_policy policy, void (*job_queue_empty_callback)(worker* wrk, const void* additional_params), const void* additional_params)
{
	wrk->thread_id = 0;
	wrk->policy = policy;
	wrk->job_queue_empty_callback = job_queue_empty_callback;
	wrk->additional_params = additional_params;
	initialize_sync_queue(&(wrk->job_queue), size, is_bounded_queue, job_queue_wait_timeout_in_microseconds);
}

static void* worker_function(void* args);

int start_worker(worker* wrk)
{
	int return_val = 0;
	if(wrk->thread_id == 0)
	{
		return_val = pthread_create(((pthread_t*)(&(wrk->thread_id))), NULL, worker_function, wrk);
		if(return_val)
		{
			printf("error starting worker %d\n", return_val);
			wrk->thread_id = 0;
		}
	}
	else
	{
		printf("worker already running at %d\n", (int) wrk->thread_id);
	}
	return return_val;
}

int stop_worker(worker* wrk)
{
	int return_val = 0;
	if(wrk->thread_id != 0)
	{
		return_val = pthread_cancel(wrk->thread_id);
		if(return_val == 0)
		{
			wrk->thread_id = 0;
		}
		else if(return_val == ESRCH)
		{
			printf("Worker thread not found, thread may have exited prior to this call\n");
		}
		else
		{
			printf("error stopping worker %d\n", return_val);
		}
	}
	else
	{
		printf("Worker thread already killed\n");
	}
	return return_val;
}

void deinitialize_worker(worker* wrk)
{
	job* job_p = NULL;

	// pop all jobs from the queue, and delete them
	while(!is_empty_sync_queue(&(wrk->job_queue)))
	{
		job_p = (job*) pop_sync_queue_non_blocking(&(wrk->job_queue)); 

		// once the job is executed we delete the job, if worker is memory managing the job
		// i.e. it was a job submitted by the client as a function
		if(job_p->job_type == JOB_WITH_MEMORY_MANAGED_BY_WORKER)
		{
			delete_job(job_p);
		}
		else if(job_p->job_type == JOB_WITH_MEMORY_MANAGED_BY_CLIENT)
		{
			set_result(job_p, NULL);
		}
	}

	deinitialize_sync_queue(&(wrk->job_queue));
}

void delete_worker(worker* wrk)
{
	deinitialize_worker(wrk);
	free(wrk);
}

int submit_function_to_worker(worker* wrk, void* (*function_p)(void* input_p), void* input_p)
{
	int was_job_queued = 0;

	// create a new job with the given parameters
	job* job_p = get_job(function_p, input_p);
	job_p->job_type = JOB_WITH_MEMORY_MANAGED_BY_WORKER;

	// update job status, from CREATED to QUEUED
	// if this update of job status is successfull, then only we go forward and queue the job
	if(job_status_change(job_p, QUEUED))
	{
		was_job_queued = push_sync_queue_non_blocking(&(wrk->job_queue), job_p);
	}

	if(was_job_queued == 0)
	{
		delete_job(job_p);
	}

	return was_job_queued;
}

int submit_job_to_worker(worker* wrk, job* job_p)
{
	int was_job_queued = 0;

	job_p->job_type = JOB_WITH_MEMORY_MANAGED_BY_CLIENT;

	// update job status, from CREATED to QUEUED
	// if this update of job status is successfull, then only we go forward and queue the job
	if(job_status_change(job_p, QUEUED))
	{
		was_job_queued = push_sync_queue_non_blocking(&(wrk->job_queue), job_p);
	}

	return was_job_queued;
}

// this is the function that will be continuously executed by the worker thread,
// dequeuing jobs continuously from the job_queue, to execute them
static void* worker_function(void* args)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	worker* wrk = ((worker*)(args));

	job* job_p = NULL;

	while(1)
	{
		while(1)
		{
			// pop a job from the queue, blocking as long as provided timeout,
			// if timedout, we exit
			if(wrk->policy == USE_CALLBACK)
			{
				job_p = (job*) pop_sync_queue_non_blocking(&(wrk->job_queue));
			}
			else
			{
				job_p = (job*) pop_sync_queue_blocking(&(wrk->job_queue));
			}

			if(job_p != NULL)
			{
				// A worker thread can not be cancelled while it is executing a job
				pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

				// execute the job that has been popped
				execute(job_p);

				// once the job is executed we delete the job, if worker is memory managing the job
				// i.e. it was a job submitted by the client as a function
				if(job_p->job_type == JOB_WITH_MEMORY_MANAGED_BY_WORKER)
				{
					delete_job(job_p);
				}

				// Turn on cancelation of the worker thread once the job it was executing has been completed and deleted
				pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			}
			else
			{
				break;
			}
		}

		// Behaviour according to policy, if we waited on the job queue and timedout
		if(wrk->policy == WAIT_ON_TIMEDOUT)
		{
			continue;
		}
		else if(wrk->policy == KILL_ON_TIMEDOUT)
		{
			break;
		}
		else if(wrk->policy == USE_CALLBACK)
		{
			if(wrk->job_queue_empty_callback != NULL)
			{
			 	wrk->job_queue_empty_callback(wrk, wrk->additional_params);
			}

			if(is_empty_sync_queue(&(wrk->job_queue)))
			{
				break;
			}
			else
			{
				continue;
			}
		}
	}

	// thread will no longer be running, so just mark it KILLED
	wrk->thread_id = 0;

	return NULL;
}