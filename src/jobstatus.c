#include<jobstatus.h>

void set_to_next_status(job_status* status_p)
{
	(*status_p) = get_next_status(*status_p);
}

job_status get_next_status(job_status status)
{
	job_status next_status;
	switch(status)
	{
		case CREATED :
		{
			next_status = QUEUED;
			break;
		}
		case QUEUED :
		{
			next_status = RUNNING;
			break;
		}
		case RUNNING :
		{
			next_status = COMPLETED;
			break;
		}
		case COMPLETED :
		{
			next_status = COMPLETED;
			break;
		}
	}
	return next_status;
}