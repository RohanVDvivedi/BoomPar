#include<job.h>

job* get_job(void* (*function_p)(void* input_p), void* input_p)
{
	job* job_p = ((job*)(malloc(sizeof(job))));
	job_p->status = CREATED;
	job_p->input_p = input_p;
	job_p->function_p = function_p;
	job_p->output_p = NULL;
	job_p->result_ready_to_read = 0;
	pthread_mutex_init(&(job_p->result_ready_mutex), NULL);
	pthread_cond_init(&(job_p->result_ready_wait), NULL);
	return job_p;
}

int execute(job* job_p)
{
	// execute only if it is queued for execution
	if(job_p->status == QUEUED)
	{
		// suppossed to update the status of the job to RUNNING
		set_to_next_status(&(job_p->status));

		// execute the job with its input, and result is stored at output
		void* output_pointer = job_p->function_p(job_p->input_p);

		// suppossed to update the status of the job to COMPLTED, 
		// so uptill here the job is completed, but the result is not yet set
		set_to_next_status(&(job_p->status));

		// lock the mutex, while we set the output_p of job, and update its status
		pthread_mutex_lock(&(job_p->result_ready_mutex));

		// set the result pointed to by output_p, and set the flag denoting the result can be read
		// this that to be atomicly executed by only one thread
		job_p->output_p = output_pointer;
		job_p->result_ready_to_read = 1;

		// notify all the threads that are waiting for the result of the job
		pthread_cond_broadcast(&(job_p->result_ready_wait));

		// unlock the mutex that was held for making the result ready
		pthread_mutex_unlock(&(job_p->result_ready_mutex));

		return 0;
	}
	else
	{
		return 1;
	}
}

void* get_result_or_wait_for_result(job* job_p)
{
	pthread_mutex_lock(&(job_p->result_ready_mutex));

	// check of the result of the job is ready to be read, if not go to wait
	while(job_p->result_ready_to_read == 0)
	{
		// go to wait state, while releasing the lock
		pthread_cond_wait(&(job_p->result_ready_wait), &(job_p->result_ready_mutex));
	}

	pthread_mutex_unlock(&(job_p->result_ready_mutex));

	return job_p->output_p;
}

void delete_job(job* job_p)
{
	pthread_mutex_destroy(&(job_p->result_ready_mutex));
	pthread_cond_destroy(&(job_p->result_ready_wait));
	free(job_p);
}