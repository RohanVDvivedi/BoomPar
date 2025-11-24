#ifndef ALARM_JOB_H
#define ALARM_JOB_H

/*
	alarm_job is a sibling work from periodic_job, but a simple one
	where the next time to wake it up is suggested by the user from the return value of the job function itself
	it does not have a fixed period and it can only be paused by returning BLOCKING from the alarm_job function itself OR using the pause_alarm_job() event
	it also has a wake_up event, that forcefully makes the job running (put into AJ_RUNNING state) from AJ_PAUSED or AJ_WAITING state
*/

#include<stdint.h>
#include<pthread.h>

typedef enum alarm_job_state alarm_job_state;
enum alarm_job_state
{
	AJ_PAUSED, // job is paused and needs to be resumed -> alarm_job starts in this state

	AJ_RUNNING, // job is in running state
	AJ_WAITING, // job is waiting for the period to elapse (suggested in the previous call)

	AJ_SHUTDOWN, // job is in shutdown state
};

typedef struct alarm_job alarm_job;
struct alarm_job
{
	// global lock for the alarm_job
	pthread_mutex_t job_lock;

	// wait for the alarm job itself (internal), to wait for its AJ_PAUSED and AJ_WAITING state
	pthread_cond_t job_wait;

	// current state of the job
	alarm_job_state state;

	// period_in_microseconds as per the suggestion of the prior alarm_job_function
	uint64_t period_in_microseconds;

	// pointer to the input pramameter, 
	void* input_p;

	// the function pointer that will be called to set the next number of microseconds after which alarm must sound i.e. to get the number of microseconds after which the alarm_job_function must be called
	// this function is called with the job_lock held, in-order to avoid missed wake_ups
	// so this function must be as short and simple as possible and must not block
	uint64_t (*alarm_set_function)(void* input_p);

	// the function pointer that will be executed with input parameter, when the alarm was set
	// this fucntion is called with the job_lock not held and can be doing any thing you desire to do
	void (*alarm_job_function)(void* input_p);

	// event_vector used to pass an event to the state machine, to change the state of the alarm_job
	// 1 bit each for SHUTDOWN_CALLED, PAUSE_CALLED and WAKE_UP_CALLED, (in decreasing order of their priority)
	int event_vector;

	pthread_cond_t stop_wait; // wait on this condition variable to wait for the job to change state to AJ_SHUTDOWN or AJ_PAUSED
};

// remember the alarm_job starts in AJ_PAUSED state, you need to wake_up_alarm_job() it after initialization
alarm_job* new_alarm_job(uint64_t (*alarm_set_function)(void* input_p), void (*alarm_job_function)(void* input_p), void* input_p);

// below 3 are the events that you send to the alarm job to make it transition into different states
// their return value only suggests if the event was sent, not that the requested transition will succeed

/* tries to transition the alarm job 
	from:
		AJ_RUNNING, AJ_WAITING
	to:
		AJ_PAUSED
*/
// may succeed only if the state is any of the "from" states mentioned above
int pause_alarm_job(alarm_job* ajob);

/* tries to transition the alarm job 
	from:
		AJ_PAUSED, AJ_WAITING,
	to:
		AJ_RUNNING
*/
// may succeed only if the state is any of the "from" states mentioned above
int wake_up_alarm_job(alarm_job* ajob);

// always succeeds, except when the job is in AJ_SHUTDOWN state
int shutdown_alarm_job(alarm_job* ajob);

// for the below 2 functions, do not hold any external lock, that hinders the execution of the alarm job's user provided function

// keeps you waiting forever until the alarm_job does not reach AJ_PAUSED or AJ_SHUTDOWN state
// 1. you may call this function only after calling shutdown_alarm_job()
// OR
// 2. after calling pause_alarm_job(), but do be sure that no one calls wake_up_alarm_job() after your call to pause_alarm_job()
void wait_for_pause_or_shutdown_of_alarm_job(alarm_job* ajob);

// delete explicitly calls shutdown_alarm_job and wait_for_pause_or_shutdown_of_alarm_job, before destroying and freeing the object
void delete_alarm_job(alarm_job* ajob);

#endif