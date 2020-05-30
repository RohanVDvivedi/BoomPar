#include<worker.h>

typedef struct worker_thread_params worker_thread_params;
struct worker_thread_params
{
	sync_queue* job_queue;
	worker_policy policy;
	unsigned long long int job_queue_empty_timeout_in_microseconds;
};

worker_thread_params* get_worker_thread_params(sync_queue* job_queue, worker_policy policy, unsigned long long int job_queue_empty_timeout_in_microseconds)
{
	worker_thread_params* wtp = (worker_thread_params*) malloc(sizeof(worker_thread_params));
	wtp->job_queue = job_queue;
	wtp->policy = policy;
	wtp->job_queue_empty_timeout_in_microseconds = job_queue_empty_timeout_in_microseconds;
	return wtp;
}

static void* worker_function(void* args);

pthread_t start_worker(sync_queue* job_queue, worker_policy policy, unsigned long long int job_queue_empty_timeout_in_microseconds)
{
	pthread_t thread_id;
	worker_thread_params* wtp = get_worker_thread_params(job_queue, policy, job_queue_empty_timeout_in_microseconds);
	int return_val = pthread_create(&thread_id, NULL, worker_function, wtp);
	if(return_val)
	{
		printf("Error starting worker : %d\n", return_val);
	}
	return thread_id;
}

int stop_worker(pthread_t thread_id)
{
	int return_val = pthread_cancel(thread_id);
	if(return_val == ESRCH)
	{
		printf("Worker thread not found, thread may have exited prior to this call\n");
	}
	else
	{
		printf("error stopping worker %d\n", return_val);
	}
	return return_val;
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

	unsigned long long int job_queue_empty_timeout_in_microseconds;

	if(wtp.policy == WAIT_FOREVER_ON_JOB_QUEUE)
	{
		job_queue_empty_timeout_in_microseconds = 0;
	}
	else if(wtp.policy == KILL_ON_TIMEDOUT)
	{
		job_queue_empty_timeout_in_microseconds = wtp.job_queue_empty_timeout_in_microseconds;
	}
	else
	{
		return NULL;
	}

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	while(1)
	{
		// we blocking wait while the job queue is empty, when we are outside the cancel disabled statements
		if(wait_while_empty_sync_queue(wtp.job_queue, job_queue_empty_timeout_in_microseconds))
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

		// Turn on cancelation of the worker thread once the job has been executed and deleted
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	}

	return NULL;
}