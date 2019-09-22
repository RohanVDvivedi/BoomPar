#include<job.h>

job* get_job(void* (*function_p)(void* input_p), void* input_p)
{
	job* job_p = ((job*)(malloc(sizeof(job))));
	job_p->status = CREATED;
	job_p->input_p = input_p;
	job_p->function_p = function_p;
	job_p->output_p = NULL;
	pthread_mutex_init(&(job_p->job_output_mutex), NULL);
	pthread_cond_init(&(job_p->job_completion_wait), NULL);
	return job_p;
}

int execute(job* job_p)
{
	// execute only if it is queued for execution
	if(job_p->status == QUEUED)
	{
		// suppossed to update the status of the job to RUNNING
		pthread_mutex_lock(&(job_p->job_output_mutex));
		set_to_next_status(&(job_p->status));
		pthread_mutex_unlock(&(job_p->job_output_mutex));

		// execute the job with its input, and result is stored at output
		void* output_pointer = job_p->function_p(job_p->input_p);

		// lock the mutex, while we set the output_p of job, and update its status
		pthread_mutex_lock(&(job_p->job_output_mutex));
		job_p->output_p = output_pointer;

		// suppossed to update the status of the job to COMPLTED
		set_to_next_status(&(job_p->status));

		// notify all the threads that are waiting for my result
		// job_wait.notifyAll();
		pthread_cond_broadcast(&(job_p->job_completion_wait));
		pthread_mutex_unlock(&(job_p->job_output_mutex));

		return 0;
	}
	else
	{
		return 1;
	}
}

void* get_result_or_wait_for_result(job* job_p)
{
	void* output_p = job_p->output_p;

	pthread_mutex_lock(&(job_p->job_output_mutex));

	while(1)
	{
		int can_exit_loop = 0;
		switch(job_p->status)
		{
			case RUNNING :
			{
				// go to wait state
				// job_wait.wait();
				pthread_cond_wait(&(job_p->job_completion_wait), &(job_p->job_output_mutex));

				// we wake up here if someone calls nofify, 
				// but we can not exit wit result unless sure of completion, 
				// hence keep looping to pass switch cases
				can_exit_loop = 0;
				break;
			}

			// if completed, we exit loop with result
			case COMPLETED :
			{
				can_exit_loop = 1;
				output_p = job_p->output_p;
				break;
			}

			// for any other state of the job,
			// it is use less to wait, 
			// and we can not return result, so we exit
			default :
			{
				can_exit_loop = 1;
				break;
			}
		}

		// if we can exit loop we should
		if(can_exit_loop)
		{
			break;
		}
	}

	pthread_mutex_unlock(&(job_p->job_output_mutex));

	return output_p;
}

void delete_job(job* job_p)
{
	pthread_mutex_destroy(&(job_p->job_output_mutex));
	pthread_cond_destroy(&(job_p->job_completion_wait));
	free(job_p);
}