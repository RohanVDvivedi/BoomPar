#ifndef PERIODIC_JOB_H
#define PERIODIC_JOB_H

#include<stdint.h>
#include<pthread.h>

typedef enum periodic_job_state periodic_job_state;
enum periodic_job_state
{
	PJ_PAUSED, // job is paused and needs to be resumed -> periodic_job starts in this state
	PJ_SINGLE_SHOT_ON_PAUSED, // job is running a single shot after a paused state -> next state would be PJ_PAUSED

	PJ_RUNNING, // job is running state
	PJ_WAITING, // job is waiting for the period to elapse
	PJ_SINGLE_SHOT_ON_WAITING, // job is running a single shot after a waiting state -> next state would be PJ_WAITING

	PJ_SHUTDOWN, // job is in shutdown states
};

typedef struct periodic_job periodic_job;
struct periodic_job
{
	// global lock for the periodic_job
	pthread_mutex_t job_lock;

	// wait for the periodic job, itself to wait for its PJ_PAUSED and PJ_WAITING state
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
	// 1 bit each for SHUTDOWN_CALLED, PAUSE_CALLED, RESUME_CALLED, and SINGLE_SHOT_CALLED
	int event_vector;

	pthread_cond_t stop_wait; // wait on this condition variable to wait for the job to change state to PJ_PAUSED or PJ_SHUTDOWN
};

// for any of the functions below, the period in microseconds can never be BLOCKING or NON_BLOCKING
// it has to be a fixed positive unsigned integer representing microseconds as the period of the job

// fails if the period_in_microseconds is BLOCKING or NON_BLOCKING
// remember the periodid_job starts in PJ_PAUSED state, you need to resume it after initialization
periodic_job* new_periodic_job(void (*periodic_job_function)(void* input_p), void* input_p, uint64_t period_in_microseconds);

// fails if the period_in_microseconds is BLOCKING or NON_BLOCKING
int update_period_for_periodic_job(periodic_job* pjob, uint64_t period_in_microseconds);

// returns instantaneous value period for the period of the periodic_job
uint64_t get_period_for_periodic_job(periodic_job* pjob);

// below 4 are the event that you send to the periodic job to make it transition into different states
// their return value only suggests if the event was sent, not that the requested transition will succeed

/* tries to transition the periodic job 
	from
		PJ_PAUSED, PJ_SINGLE_SHOT_ON_PAUSED,
	to
		PJ_RUNNING, PJ_WAITING, PJ_SINGLE_SHOT_ON_WAITING
*/
// may succeed only if the state is any of the from states mentioned above
int resume_periodic_job(periodic_job* pjob);

/* tries to transition the periodic job 
	from
		PJ_RUNNING, PJ_WAITING, PJ_SINGLE_SHOT_ON_WAITING
	to
		PJ_PAUSED, PJ_SINGLE_SHOT_ON_PAUSED,
*/
// may succeed only if the state is any of the from states mentioned above
int pause_periodic_job(periodic_job* pjob);

// tries to immediately transition a PJ_PAUSED or PJ_WAITING periodic job into PJ_SINGLE_SHOT_ON_PAUSED or PJ_SINGLE_SHOT_ON_WAITING states
// it allows the periodic job to quit waiting and attempt to run the periodic_job_fucntion jsut once, then it is again transitioned into its prior state
int single_shot_periodic_job(periodic_job* pjob);

// always succeeds, except when the job is in PJ_SHUTDOWN state
int shutdown_periodic_job(periodic_job* pjob);

// for the below 2 functions, do not hold any external lock, that hinders the execution of the periodic job's user provided function

// keeps you waiting forever until the periodic_job does nor reach PJ_PAUSED or PJ_SHUTDOWN state
// 1. you may call this function only after calling shutdown_periodic_job()
// OR
// 2. after calling pause_periodic_job(), but do be sure that no one calls resume_periodic_job() after your call to pause_periodic_job()
void wait_for_pause_or_shutdown_of_periodic_job(periodic_job* pjob);

// delete explicitly calls shutdown_periodic_job and wait_for_pause_or_shutdown_of_periodic_job, before destroying and freeing the object
void delete_periodic_job(periodic_job* pjob);

#endif