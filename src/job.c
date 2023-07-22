#include<job.h>

#include<stdlib.h>

job* new_job(void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output, void (*cancellation_callback)(void* input_p))
{
	job* job_p = malloc(sizeof(job));
	if(job_p != NULL)
		initialize_job(job_p, function_p, input_p, promise_for_output, cancellation_callback);
	return job_p;
}

void initialize_job(job* job_p, void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output, void (*cancellation_callback)(void* input_p))
{
	job_p->state = CREATED;
	job_p->input_p = input_p;
	job_p->function_p = function_p;
	job_p->promise_for_output = promise_for_output;
	job_p->cancellation_callback = cancellation_callback;
}

static void* execute_wrapper(void* job_v_p)
{
	job* job_p = ((job*)job_v_p);
	execute(job_p);
	return NULL;
}

int execute_async(job* job_p, pthread_t* thread_id)
{
	int error = pthread_create(thread_id, NULL, execute_wrapper, job_p);

	if(!error)
		pthread_detach(*thread_id);

	return error;
}

int execute(job* job_p)
{
	// we can not run a job that is already running or completed
	if(job_p->status != CREATED)
		return 0;

	job_p->status = RUNNING;

	// execute the job with its input, and result is stored at output
	void* output_pointer = job_p->function_p(job_p->input_p);

	// fulfill the promised output of the job
	if(job_p->promise_for_output != NULL)
		set_promised_result(job_p->promise_for_output, output_pointer);

	job_p->status = COMPLETED;

	return 1;
}

void deinitialize_job(job* job_p)
{
	// a job deinitialized before reaching the state of completion is the one that failed
	// a failed job sets its promised result to NULL
	if(job_p->promise_for_output != NULL && job_p->status != COMPLETED) // there will be a disaster if you deinitialize a job that is RUNNING
		set_promised_result(job_p->promise_for_output, NULL); // this function succeeds only if the job hadn't yet submitted its promised result

	job_p->promise_for_output = NULL;
	job_p->input_p = NULL;
	job_p->function_p = NULL;
}

void delete_job(job* job_p)
{
	deinitialize_job(job_p);
	free(job_p);
}