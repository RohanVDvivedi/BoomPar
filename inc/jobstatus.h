#ifndef JOB_STATUS_H
#define JOB_STATUS_H

// a job can be in any of the three states
typedef enum job_status job_status;
enum job_status
{
	CREATED   = 0,
	QUEUED    = 1,
	RUNNING   = 2,
	COMPLETED = 3
};

// updates the job_status pointed to by the status pointer
void set_to_next_status(job_status* status_p);

// gets the status that comes after the current status
// acts as a state machine
job_status get_next_status(job_status status);

#endif