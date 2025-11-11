#include<boompar/worker.h>

#include<stdlib.h>
#include<errno.h>

typedef struct worker_thread_params worker_thread_params;
struct worker_thread_params
{
	// this function is called before the worker thread tries to dequeue the first job
	void (*start_up)(void* additional_params);

	// the job_queue that the worker will dequeue jobs from
	job_queue* job_q;

	// the maximum timeout the thread blocks until while there are no jobs in the job_queue to work on
	uint64_t job_queue_empty_timeout_in_microseconds;

	// this function is called after the last job is executed and deleted, and the thread has decided to kill itself
	void (*clean_up)(void* additional_params);

	// the parameter that is passed in with start_up and the clean up job 
	void* additional_params;
};

worker_thread_params* get_worker_thread_params(job_queue* job_q, uint64_t job_queue_empty_timeout_in_microseconds, void(*start_up)(void* additional_params), void(*clean_up)(void* additional_params), void* additional_params)
{
	worker_thread_params* wtp = malloc(sizeof(worker_thread_params));
	if(wtp == NULL)
		return NULL;

	wtp->start_up = start_up;
	wtp->job_q = job_q;
	wtp->job_queue_empty_timeout_in_microseconds = job_queue_empty_timeout_in_microseconds;
	wtp->clean_up = clean_up;
	wtp->additional_params = additional_params;
	return wtp;
}

static void* worker_function(void* args);

int start_worker(pthread_t* thread_id, job_queue* job_q, uint64_t job_queue_empty_timeout_in_microseconds, void(*start_up)(void* additional_params), void(*clean_up)(void* additional_params), void* additional_params, size_t stack_size)
{
	// default return value is the insufficient resources to create a thread
	int return_val = EAGAIN;

	if(job_queue_empty_timeout_in_microseconds == NON_BLOCKING)
		return return_val;

	worker_thread_params* wtp = get_worker_thread_params(job_q, job_queue_empty_timeout_in_microseconds, start_up, clean_up, additional_params);
	if(wtp == NULL)
		return return_val;

	pthread_attr_t thread_attributes;
	pthread_attr_init(&thread_attributes);
	if(stack_size != 0)
		pthread_attr_setstacksize(&thread_attributes, stack_size);
	pthread_attr_setdetachstate(&thread_attributes, PTHREAD_CREATE_DETACHED);

	return_val = pthread_create(thread_id, &thread_attributes, worker_function, wtp);
	if(return_val)
	{
		free(wtp);
		return return_val;
	}

	pthread_attr_destroy(&thread_attributes);
	
	return return_val;
}

int submit_job_worker(job_queue* job_q, void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output, void (*cancellation_callback)(void* input_p), uint64_t submission_timeout_in_microseconds)
{
	int was_job_queued = 0;

	// create a new job with the given parameters
	job* job_p = new_job(function_p, input_p, promise_for_output, cancellation_callback);

	// if a job couldn't be created, then fail
	if(job_p == NULL)
		return 0;

	// forward and queue the job
	was_job_queued = push_job_queue(job_q, job_p, submission_timeout_in_microseconds);

	// if the job was not queued, then we just delete the job
	// the job here is in CREATED state, but since the job was not queued,
	// any assignment/commitment of the promise and the job is not respected.
	// i.e. a job not queued to the job_queue, is equivalent to a job that was never created
	// so we delete it in the CREATED state
	if(!was_job_queued)
		delete_job(job_p);

	return was_job_queued;
}

void discard_leftover_jobs(job_queue* job_q)
{
	while(!is_empty_job_queue(job_q))
	{
		job* job_p = (job*) pop_job_queue(job_q, NON_BLOCKING);
		if(job_p != NULL)
		{
			cancel_job(job_p);
			delete_job(job_p);
		}
	}
}

// this is the function that will be continuously executed by the worker thread,
// dequeuing jobs continuously from the job_queue, to execute them
static void* worker_function(void* args)
{
	worker_thread_params wtp = *((worker_thread_params*)(args));
	free(args);

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	if(wtp.start_up != NULL)
		wtp.start_up(wtp.additional_params);

	if(wtp.job_q != NULL)
	{
		while(1)
		{
			// A worker thread can not be cancelled while the worker is dequeuing and executing a job
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

			// pop a job from the queue blockingly with predertmined timeout
			job* job_p = (job*) pop_job_queue(wtp.job_q, wtp.job_queue_empty_timeout_in_microseconds);

			// a NULL job implies a stop worker was submitted
			if(job_p == NULL)
				break;

			// execute the job that has been popped
			execute_job(job_p);

			// once the job is executed we delete the job
			delete_job(job_p);

			// Turn on cancelation of the worker thread once the job has been executed and deleted
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		}
	}

	if(wtp.clean_up != NULL)
		wtp.clean_up(wtp.additional_params);

	return NULL;
}