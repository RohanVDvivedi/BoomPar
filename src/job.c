#include<job.h>

job* get_job(void* (*function_p)(void* input_p), void* input_p)
{
	job* job_p = ((job*)(malloc(sizeof(job))));
	job_p->status = get_initial_state_status();

	job_p->input_p = input_p;
	job_p->function_p = function_p;
	job_p->output_p = NULL;

	job_p->threads_waiting_for_result = 0;
	job_p->result_ready_to_read = 0;
	pthread_mutex_init(&(job_p->result_ready_mutex), NULL);
	pthread_cond_init(&(job_p->result_ready_wait), NULL);

	job_p->job_type = 0;

	return job_p;
}

int job_status_change(job* job_p, job_status job_new_status)
{
	int has_job_status_changed = 0;
	job_status next_allowed_status = get_next_status(job_p->status);
	if(next_allowed_status == job_new_status)
	{
		set_to_next_status(&(job_p->status));
		has_job_status_changed = 1;
	}
	return has_job_status_changed;
}

void* execute_wrapper(void* job_v_p)
{
	job* job_p = ((job*)job_v_p);
	execute(job_p);
	return NULL;
}

pthread_t execute_async(job* job_p)
{
	// suppossed to update the status of the job to RUNNING
	if(!job_status_change(job_p, QUEUED))
	{
		goto ERROR;
	}

	// the id to the new thread
	pthread_t thread_id_p;

	// create a new thread that runs, with an executor, executor_p
	pthread_create(&thread_id_p, NULL, execute_wrapper, job_p);

	return thread_id_p;

	ERROR :;
	return 0;
}

int execute(job* job_p)
{
	// suppossed to update the status of the job to RUNNING
	if(!job_status_change(job_p, RUNNING))
	{
		goto ERROR;
	}

	// execute the job with its input, and result is stored at output
	void* output_pointer = job_p->function_p(job_p->input_p);

	// suppossed to update the status of the job to COMPLTED, 
	// so uptill here the job is completed, but the result is not yet set
	if(!job_status_change(job_p, COMPLETED))
	{
		goto ERROR;
	}

	// sets the output_p pointer of job_p, to point data pointed to by output_pointer
	set_result(job_p, output_pointer);

	return 0;

	ERROR :;
	return 1;
}

void set_result(job* job_p, void* output_pointer)
{
	// lock the mutex
	pthread_mutex_lock(&(job_p->result_ready_mutex));

	// set the result pointed to by output_p, and set the flag denoting the result can be read
	// this that to be atomicly executed by only one thread
	job_p->output_p = output_pointer;
	job_p->result_ready_to_read = 1;

	// notify all the threads that are waiting for the result of the job
	pthread_cond_broadcast(&(job_p->result_ready_wait));

	// unlock the mutex
	pthread_mutex_unlock(&(job_p->result_ready_mutex));
}

void* get_result(job* job_p)
{
	// lock the mutex
	pthread_mutex_lock(&(job_p->result_ready_mutex));

	// check of the result of the job is ready to be read, if not go to wait
	while(job_p->result_ready_to_read == 0)
	{
		// increment count of the waiting threads, before going to wait
		job_p->threads_waiting_for_result++;

		// go to wait state, while releasing the lock, so that the job running thread will come and wake it up when result is ready
		pthread_cond_wait(&(job_p->result_ready_wait), &(job_p->result_ready_mutex));

		// decrement count of the waiting threads, after waking up
		job_p->threads_waiting_for_result--;
	}

	// unlock the mutex
	pthread_mutex_unlock(&(job_p->result_ready_mutex));

	return job_p->output_p;
}

int check_result_ready(job* job_p)
{
	// lock the mutex
	pthread_mutex_lock(&(job_p->result_ready_mutex));

	// set variable if the result of the job is ready to be read
	int result_ready_to_read = job_p->result_ready_to_read;

	// unlock the mutex
	pthread_mutex_unlock(&(job_p->result_ready_mutex));

	return result_ready_to_read;
}

unsigned long long int get_thread_count_waiting_for_result(job* job_p)
{
	// lock the mutex
	pthread_mutex_lock(&(job_p->result_ready_mutex));

	// set variable to threads_waiting_for_result
	unsigned long long int threads_waiting_for_result = job_p->threads_waiting_for_result;

	// unlock the mutex
	pthread_mutex_unlock(&(job_p->result_ready_mutex));

	return threads_waiting_for_result;
}

void delete_job(job* job_p)
{
	pthread_mutex_destroy(&(job_p->result_ready_mutex));
	pthread_cond_destroy(&(job_p->result_ready_wait));
	free(job_p);
}