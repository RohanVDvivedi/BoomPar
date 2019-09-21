#include<job.h>

job* get_job(void* (*function_p)(void* input_p), void* input_p)
{
	job* job_p = ((job*)(malloc(sizeof(job))));
	job_p->status = CREATED;
	job_p->input_p = input_p;
	job_p->function_p = function_p;
	job_p->output_p = NULL;
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
		job_p->output_p = job_p->function_p(job_p->input_p);

		// suppossed to update the status of the job to COMPLTED
		set_to_next_status(&(job_p->status));

		// notify all the threads that are waiting for my result
		// job_wait.notifyAll();

		return 0;
	}
	else
	{
		return 1;
	}
}

void* get_result_or_wait_for_result(job* job_p)
{
	void* output_p = NULL;

	while(1)
	{
		int can_exit_loop = 0;
		switch(job_p->status)
		{
			case RUNNING :
			{
				// go to wait state
				// job_wait.wait();

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

	return output_p;
}

void delete_job(job* job_p)
{
	free(job_p);
}