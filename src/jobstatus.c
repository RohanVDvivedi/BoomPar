#include<jobstatus.h>

void set_to_next_status(job_status* status_p)
{
	(*status_p) = get_next_status(*status_p);
}

job_status get_initial_state_status()
{
	return CREATED;
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
		default :
		{
			next_status = CREATED;
			break;
		}
	}
	return next_status;
}

char* get_status_string(job_status status)
{
	switch(status)
	{
		case CREATED :
		{
			return "CREATED";
		}
		case QUEUED :
		{
			return "QUEUED";
		}
		case RUNNING :
		{
			return "RUNNING";
		}
		case COMPLETED :
		{
			return "COMPLETED";
		}
		default :
		{
			return "INVALID_JOB";
		}
	}
}