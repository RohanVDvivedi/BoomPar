#define JOB_STATUS_H
#ifndef JOB_STATUS_H

// a job can be in any of the three states
typedef enum job_status job_status;
{
	QUEUED,
	RUNNING,
	COMPLETED
};

#endif