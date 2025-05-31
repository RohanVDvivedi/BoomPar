#ifndef PERIODIC_JOB_H
#define PERIODIC_JOB_H

#include<stdint.h>
#include<pthread.h>

typedef enum periodic_job_state periodic_job_state;
enum periodic_job_state
{
	PAUSED, // job is paused and needs to be resumed -> periodic_job starts in this state
	RUNNING, // job is running state
	WAITING, // job is waiting for the period to elapse
	SINGLE_SHOT_ON_WAITING, // job is running a single shot after a waiting state -> next state would be WAITING
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
	// 1 bit each for SHUTDOWN_CALLED, RESUME_CALLED, PAUSE_CALLED and SINGLE_SHOT_CALLED
	int event_vector;

	pthread_cond_t stop_wait; // wait on this condition variable to wait for the job to change state to PAUSED or SHUTDOWN
};

// for any of the functions below, the period in microseconds can never be BLOCKING or NON_BLOCKING
// it has to be a fixed positive unsigned integer representing microseconds as the period of the job

// fails if the period_in_microseconds is BLOCKING or NON_BLOCKING
// remember the periodid_job starts in PAUSED state, you need to resume it after initialization
periodic_job* new_periodic_job(void (*periodic_job_function)(void* input_p), void* input_p, uint64_t period_in_microseconds);

// below 4 are the event that you send to the periodic job to make it transition into different states
// their return value only suggests if the event was sent, not that the requested transition will succeed

// tries to transition the periodic job into a RUNNING <-> WAITING loop
// may succeed only if the state is PAUSED or SINGLE_SHOT_ON_PAUSED
int resume_periodic_job(periodic_job* pjob);

// tries to put the periodic job in PAUSED state to wait until it is resumed
// may succeed only if the state is anything but PAUSED
int pause_periodic_job(periodic_job* pjob);

// tries to immediately transition a PAUSED or WAITING periodic job into SINGLE_SHOT_ON_PAUSED or SINGLE_SHOT_ON_WAITING states
// it allows the periodic job to quit waiting and attempt to run the periodic_job_fucntion jsut once, then it is again transitioned into its prior state
int single_shot_periodic_job(periodic_job* pjob);

// always succeeds
int shutdown_periodic_job(periodic_job* pjob);

// keeps you waiting forever until the periodic_job does nor reach PAUSED or SHUTDOWN state
int wait_for_pause_or_shutdown_of_periodic_job(periodic_job* pjob);

// delete explicitly calls shutdown_periodic_job and wait_for_pause_or_shutdown_of_periodic_job, before destroying and freeing the object
void delete_periodic_job(periodic_job* pjob);

#endif