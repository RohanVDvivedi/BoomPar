#include<worker.h>

typedef struct worker_thread_params worker_thread_params;
struct worker_thread_params
{
	// this function is called before the worker thread tries to dequeue the first job
	void (*start_up)(void* additional_params);

	// the job_queue that the 
	sync_queue* job_queue;

	// the maximum timeout the thread blocks until while there are no jobs in the job_queue to work on
	unsigned long long int job_queue_empty_timeout_in_microseconds;

	// this function is called after the last job is executed and deleted, and the thread has decided to kill itself
	void (*clean_up)(void* additional_params);

	// the parameter that is passed in with start_up and the clean up job 
	void* additional_params;
};

worker_thread_params* get_worker_thread_params(sync_queue* job_queue, unsigned long long int job_queue_empty_timeout_in_microseconds, void(*start_up)(void* additional_params), void(*clean_up)(void* additional_params), void* additional_params)
{
	worker_thread_params* wtp = (worker_thread_params*) malloc(sizeof(worker_thread_params));
	wtp->start_up = start_up;
	wtp->job_queue = job_queue;
	wtp->job_queue_empty_timeout_in_microseconds = job_queue_empty_timeout_in_microseconds;
	wtp->clean_up = clean_up;
	wtp->additional_params = additional_params;
	return wtp;
}

static void* worker_function(void* args);

pthread_t start_worker(sync_queue* job_queue, unsigned long long int job_queue_empty_timeout_in_microseconds, void(*start_up)(void* additional_params), void(*clean_up)(void* additional_params), void* additional_params)
{
	pthread_t thread_id;
	worker_thread_params* wtp = get_worker_thread_params(job_queue, job_queue_empty_timeout_in_microseconds, start_up, clean_up, additional_params);
	int return_val = pthread_create(&thread_id, NULL, worker_function, wtp);
	if(return_val)
	{
		printf("Error starting worker : %d\n", return_val);
	}
	else
	{
		pthread_detach(thread_id);
	}
	return thread_id;
}

int submit_function_worker(sync_queue* job_queue, void* (*function_p)(void* input_p), void* input_p)
{
	int was_job_queued = 0;

	// create a new job with the given parameters
	job* job_p = get_job(function_p, input_p);
	job_p->job_type = JOB_WITH_MEMORY_MANAGED_BY_WORKER;

	// update job status, from CREATED to QUEUED
	// if this update of job status is successfull, then only we go forward and queue the job
	if(job_status_change(job_p, QUEUED))
	{
		was_job_queued = push_sync_queue_non_blocking(job_queue, job_p);
	}

	if(was_job_queued == 0)
	{
		delete_job(job_p);
	}

	return was_job_queued;
}

int submit_job_worker(sync_queue* job_queue, job* job_p)
{
	int was_job_queued = 0;

	job_p->job_type = JOB_WITH_MEMORY_MANAGED_BY_CLIENT;

	// update job status, from CREATED to QUEUED
	// if this update of job status is successfull, then only we go forward and queue the job
	if(job_status_change(job_p, QUEUED))
	{
		was_job_queued = push_sync_queue_non_blocking(job_queue, job_p);
	}

	return was_job_queued;
}

int submit_stop_worker(sync_queue* job_queue)
{
	return push_sync_queue_non_blocking(job_queue, NULL);
}

void discard_leftover_jobs(sync_queue* job_queue)
{
	while(!is_empty_sync_queue(job_queue))
	{
		job* job_p = (job*) pop_sync_queue_non_blocking(job_queue);

		if(job_p != NULL)
		{
			if(job_p->job_type == JOB_WITH_MEMORY_MANAGED_BY_WORKER)
			{
				delete_job(job_p);
			}
			else if(job_p->job_type == JOB_WITH_MEMORY_MANAGED_BY_CLIENT)
			{
				set_result(job_p, NULL);
			}
		}
	}
}

// this is the function that will be continuously executed by the worker thread,
// dequeuing jobs continuously from the job_queue, to execute them
static void* worker_function(void* args)
{
	worker_thread_params wtp = *((worker_thread_params*)(args));
	free(args);

	if(wtp.job_queue == NULL)
	{
		return NULL;
	}

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	if(wtp.start_up != NULL)
	{
		wtp.start_up(wtp.additional_params);
	}

	while(1)
	{
		// we blocking wait while the job queue is empty, when we are outside the cancel disabled statements
		if(wait_while_empty_sync_queue(wtp.job_queue, wtp.job_queue_empty_timeout_in_microseconds))
		{
			break;
		}

		// A worker thread can not be cancelled while the worker is dequeuing and executing a job
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

		// pop a job from the queue unblockingly, we can not block while we have disableded thread cancellation
		job* job_p = (job*) pop_sync_queue_non_blocking(wtp.job_queue);

		if(job_p != NULL)
		{
			// execute the job that has been popped
			execute(job_p);

			// once the job is executed we delete the job, if worker is memory managing the job
			// i.e. it was a job submitted by the client as a function
			if(job_p->job_type == JOB_WITH_MEMORY_MANAGED_BY_WORKER)
			{
				delete_job(job_p);
			}
		}
		else
		{
			break;
		}

		// Turn on cancelation of the worker thread once the job has been executed and deleted
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	}

	if(wtp.clean_up != NULL)
	{
		wtp.clean_up(wtp.additional_params);
	}

	return NULL;
}