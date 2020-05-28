#include<worker.h>

worker* get_worker(unsigned long long int size, int is_bounded_queue)
{
	worker* wrk = (worker*) malloc(sizeof(worker));
	initialize_worker(wrk, size, is_bounded_queue);
	return wrk;
}

void initialize_worker(worker* wrk, unsigned long long int size, int is_bounded_queue)
{
	wrk->thread_id = 0;
	initialize_sync_queue(&(wrk->job_queue), size, is_bounded_queue);
}

static void* worker_function(void* args);

int start_worker(worker* wrk)
{
	int return_val = pthread_create(&(wrk->thread_id), NULL, worker_function, wrk);
	if(return_val)
	{
		printf("error starting worker %d\n", return_val);
		wrk->thread_id = 0;
	}
	return return_val;
}

int stop_worker(worker* wrk)
{
	int return_val = pthread_cancel(wrk->thread_id);
	if(return_val == 0)
	{
		wrk->thread_id = 0;
	}
	else
	{
		printf("error stopping worker %d\n", return_val);
	}
	return return_val;
}

void deinitialize_worker(worker* wrk)
{
	deinitialize_sync_queue(&(wrk->job_queue));
}

void delete_worker(worker* wrk)
{
	deinitialize_worker(wrk);
	free(wrk);
}

typedef enum worker_job_type worker_job_type;
enum worker_job_type
{
	JOB_WITH_MEMORY_MANAGED_BY_CLIENT = 0,
	JOB_WITH_MEMORY_MANAGED_BY_WORKER = 1
};

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
	worker* wrk = ((worker*)(args));

	while(1)
	{
		// pop a job from the queue, to execute
		job* job_p = (job*) pop_sync_queue_blocking(&(wrk->job_queue));

		if(job_p != NULL)
		{
			// execute the job that has been popped
			execute(job_p);

			// once the job is executed we delete the job, if executor is memory managing the job
			// i.e. it was a job submitted by the client as a function
			if(job_p->job_type == JOB_WITH_MEMORY_MANAGED_BY_WORKER)
			{
				delete_job(job_p);
			}
		}
	}

	return NULL;
}