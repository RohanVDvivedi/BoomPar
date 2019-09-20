#include<job.h>

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