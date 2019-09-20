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
		return 0;
	}
	else
	{
		return 1;
	}
}

void delete_job(job* job_p)
{
	free(job_p);
}