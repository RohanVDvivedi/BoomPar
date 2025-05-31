#ifndef PERIODIC_JOB_H
#define PERIODIC_JOB_H

#include<stdint.h>
#include<pthread.h>

typedef enum periodic_job_state periodic_job_state;
enum periodic_job_state
{
	RUNNING, // job is running state
	WAITING, // job is waiting for the period to elapse
	SINGLE_SHOT_ON_WAITING, // job is running a single shot after a waiting state -> next state would be WAITING
	PAUSED, // job is paused and needs to be resumed
	SINGLE_SHOT_ON_PAUSED, // job is running a single shot after a paused state -> next state would be PAUSED
	SHUTDOWN, // job is in shutdown states
};

typedef struct periodic_job periodic_job;
struct periodic_job
{
	// global lock for the periodic_job
	pthread_mutex_t job_lock;

	// wait for the periodic job, itself to wait for its PAUSED and WAITING state
	pthread_cond_t job_wait;

	// current state of the job
	periodic_job_state state;

	// period of the job in microseconds
	uint64_t period_in_microseconds;

	// pointer to the input pramameter, 
	void* input_p;

	// the function pointer that will be executed with input parameter
	void (*periodic_job_function)(void* input_p);

	// event_vector used to pass an event to the state machine, to change the state of the periodic_job
	int event_vector;

	pthread_cond_t stop_wait; // wait on this condition variable to wait for the job to change state to PAUSED or SHUTDOWN
};

#endif