#include<job.h>

#include<stdlib.h>

/*
** BELOW FUNCTIONS are the only ones to be used for manipulating the stare of a job status
*/

static job_status get_initial_state_status()
{
	return CREATED;
}

/*
 * all valid job status transitions
	CREATED -> QUEUED -> RUNNING -> COMPLETED
				^			 |
				|		     |
				+- WAITING <-+
*/

static int is_valid_job_status_transition(job_status curr, job_status next)
{
	switch(curr)
	{
		case CREATED :
		{
			if(next == QUEUED)
				return 1;
			break;
		}
		case QUEUED :
		{
			if(next == RUNNING)
				return 1;
			break;
		}
		case WAITING :
		{
			if(next == QUEUED)
				return 1;
			break;
		}
		case RUNNING :
		{
			if(next == WAITING || next == COMPLETED)
				return 1;
			break;
		}
		case COMPLETED :
			return 0;
	}
	return 0;
}

/*
** ABOVE FUNCTIONS are the only ones to be used for manipulating the state of a job status
*/

job* new_job(void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output)
{
	job* job_p = ((job*)(malloc(sizeof(job))));
	initialize_job(job_p, function_p, input_p, promise_for_output);
	return job_p;
}

void initialize_job(job* job_p, void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output)
{
	job_p->status = get_initial_state_status();
	job_p->input_p = input_p;
	job_p->function_p = function_p;
	job_p->promise_for_output = promise_for_output;
}

int job_status_change(job* job_p, job_status job_new_status)
{
	int has_job_status_changed = 0;
	if(is_valid_job_status_transition(job_p->status, job_new_status))
	{
		job_p->status = job_new_status;
		has_job_status_changed = 1;
	}
	return has_job_status_changed;
}

static void* execute_wrapper(void* job_v_p)
{
	job* job_p = ((job*)job_v_p);
	execute(job_p);
	return NULL;
}

pthread_t execute_async(job* job_p)
{
	// update the status of the job to QUEUED
	if(!job_status_change(job_p, QUEUED))
		goto ERROR;

	// the id to the new thread
	pthread_t thread_id = 0;

	// create a new thread that runs, with an executor, executor_p
	pthread_create(&thread_id, NULL, execute_wrapper, job_p);

	if(thread_id != 0)
		pthread_detach(thread_id);

	return thread_id;

	ERROR :;
	return 0;
}

int execute(job* job_p)
{
	// update the status of the job to RUNNING
	if(!job_status_change(job_p, RUNNING))
		goto ERROR;

	// execute the job with its input, and result is stored at output
	void* output_pointer = job_p->function_p(job_p->input_p);

	// update the status of the job to COMPLTED
	if(!job_status_change(job_p, COMPLETED))
		goto ERROR;

	// fulfill the promised output of the job
	// result is only set for a COMPLETED job or a job that gets destroyed
	if(job_p->promise_for_output != NULL)
		set_promised_result(job_p->promise_for_output, output_pointer);

	return 0;

	ERROR :;
	return 1;
}

void deinitialize_job(job* job_p)
{
	// a job deinitialized before reaching the state of completion is the one that failed
	// a failed job sets its promised result to NULL
	if(job_p->promise_for_output != NULL && job_p->status != COMPLETED)
		set_promised_result(job_p->promise_for_output, NULL); // this function succeeds only if the job hadn't yet submitted its promised result

	// reset all job attributes
	job_p->promise_for_output = NULL;
	job_p->status = get_initial_state_status();
	job_p->input_p = NULL;
	job_p->function_p = NULL;
}

void delete_job(job* job_p)
{
	deinitialize_job(job_p);
	free(job_p);
}