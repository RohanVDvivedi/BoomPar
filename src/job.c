#include<job.h>

#include<stdlib.h>

job* new_job(void* (*job_function)(void* input_p), void* input_p, promise* promise_for_output, void (*cancellation_callback)(void* input_p))
{
	if(job_function == NULL)
		return NULL;
	job* job_p = malloc(sizeof(job));
	if(job_p != NULL)
		initialize_job(job_p, job_function, input_p, promise_for_output, cancellation_callback);
	return job_p;
}

void initialize_job(job* job_p, void* (*job_function)(void* input_p), void* input_p, promise* promise_for_output, void (*cancellation_callback)(void* input_p))
{
	job_p->state = CREATED;
	job_p->input_p = input_p;
	job_p->job_function = job_function;
	job_p->promise_for_output = promise_for_output;
	job_p->cancellation_callback = cancellation_callback;
}

static void* execute_wrapper(void* job_v_p)
{
	job* job_p = ((job*)job_v_p);
	execute_job(job_p);
	return NULL;
}

int execute_job_async(job* job_p, pthread_t* thread_id)
{
	// make sure that the job is in right state to start with execution
	if(job_p->state != CREATED)
		return INVALID_JOB_STATE;

	int error = pthread_create(thread_id, NULL, execute_wrapper, job_p);
	if(!error)
		pthread_detach(*thread_id);

	return error;
}

int execute_job(job* job_p)
{
	// we can not run a job that is already running or completed
	if(job_p->state != CREATED)
		return 0;

	job_p->state = RUNNING;

	// execute the job with its input, and result is stored at output
	void* output_pointer = job_p->job_function(job_p->input_p);

	// fulfill the promised output of the job
	if(job_p->promise_for_output)
		set_promised_result(job_p->promise_for_output, output_pointer);

	job_p->state = COMPLETED;

	return 1;
}

int cancel_job(job* job_p)
{
	// we can only cancel a created job
	if(job_p->state != CREATED)
		return 0;

	// if a cancellation_callback exists, then call it
	if(job_p->cancellation_callback)
		job_p->cancellation_callback(job_p->input_p);

	// if a promise_for_output exists, then set NULL, as the job was never executed
	if(job_p->promise_for_output)
		set_promised_result(job_p->promise_for_output, NULL);

	job_p->state = CANCELLED;

	return 1;
}

void deinitialize_job(job* job_p)
{
	job_p->promise_for_output = NULL;
	job_p->input_p = NULL;
	job_p->job_function = NULL;
	job_p->cancellation_callback = NULL;
}

void delete_job(job* job_p)
{
	deinitialize_job(job_p);
	free(job_p);
}