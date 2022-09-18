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

job* new_job(void* (*function_p)(void* input_p), void* input_p, size_t stack_size, promise* promise_for_output)
{
	job* job_p = ((job*)(malloc(sizeof(job))));
	initialize_job(job_p, function_p, input_p, stack_size, promise_for_output);
	return job_p;
}

void initialize_job(job* job_p, void* (*function_p)(void* input_p), void* input_p, size_t stack_size, promise* promise_for_output)
{
	job_p->status = get_initial_state_status();
	job_p->input_p = input_p;
	job_p->function_p = function_p;
	job_p->stack_size = stack_size;
	job_p->promise_for_output = promise_for_output;
}

int job_status_change(job* job_p, job_status job_new_status)
{
	int job_status_changed = is_valid_job_status_transition(job_p->status, job_new_status);
	if(job_status_changed)
		job_p->status = job_new_status;
	return job_status_changed;
}

static void separate_stack_job_wrapper(job* job_p)
{
	// job must be in running state
	if(job_p->status != RUNNING)
		goto EXIT;

	// execute the job with its input, and result is stored at output
	void* output_pointer = job_p->function_p(job_p->input_p);

	// update the status of the job to COMPLETED
	if(!job_status_change(job_p, COMPLETED))
		goto EXIT;

	// fulfill the promised output of the job
	// result is only set for a COMPLETED job or a job that gets destroyed
	if(job_p->promise_for_output != NULL)
		set_promised_result(job_p->promise_for_output, output_pointer);

	EXIT:;

	// set context to start executing the calling thread context
	setcontext(&(job_p->thread_context));
}

int execute(job* job_p)
{
	// update the status of the job to RUNNING
	if(!job_status_change(job_p, RUNNING))
		goto ERROR;

	// populate job_context of the job
	job_p->job_context.uc_link = NULL;
	job_p->job_context.uc_stack.ss_sp = malloc(job_p->stack_size);
	job_p->job_context.uc_stack.ss_size = job_p->stack_size;
	makecontext(&(job_p->job_context), separate_stack_job_wrapper, 1, job_p);

	// swap context to start executing the job
	// this call will populate the context of this thread in thread_context and load the job_context aswell
	if(-1 == swapcontext(&(job_p->thread_context), &(job_p->job_context)))
		goto ERROR;

	return 1;

	ERROR :;
	return 0;
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