#include<boompar/periodic_job.h>

#include<posixutils/pthread_cond_utils.h>

#include<stdlib.h>

// below are the events that can be placed in the event_vector, in the priority of their processing
// this macros are for internal use
#define SHUTDOWN_CALLED    (1<<0) // can be called on all states except PJ_SHUTDOWN itself
#define PAUSE_CALLED       (1<<1) // can be called on only PJ_RUNNING, PJ_WAITING and PJ_SINGLE_SHOT_ON_WAITING states
#define RESUME_CALLED      (1<<2) // can be called on only PJ_PAUSED and PJ_SINGLE_SHOT_ON_PAUSED states
#define SINGLE_SHOT_CALLED (1<<3) // can be called on only PJ_PAUSED and PJ_WAITING states

static inline void consume_events_and_update_state(periodic_job* pjob, uint64_t total_time_waited_for)
{
	switch(pjob->state)
	{
		case PJ_PAUSED :
		{
			if(pjob->event_vector & SHUTDOWN_CALLED)
				pjob->state = PJ_SHUTDOWN;
			else if(pjob->event_vector & RESUME_CALLED)
				pjob->state = PJ_RUNNING;
			else if(pjob->event_vector & SINGLE_SHOT_CALLED)
				pjob->state = PJ_SINGLE_SHOT_ON_PAUSED;
			else
				pjob->state = PJ_PAUSED;
			break;
		}
		case PJ_SINGLE_SHOT_ON_PAUSED :
		{
			if(pjob->event_vector & SHUTDOWN_CALLED)
				pjob->state = PJ_SHUTDOWN;
			else if(pjob->event_vector & RESUME_CALLED)
				pjob->state = PJ_WAITING;
			else
				pjob->state = PJ_PAUSED;
			break;
		}

		case PJ_RUNNING :
		{
			if(pjob->event_vector & SHUTDOWN_CALLED)
				pjob->state = PJ_SHUTDOWN;
			else if(pjob->event_vector & PAUSE_CALLED)
				pjob->state = PJ_PAUSED;
			else
				pjob->state = PJ_WAITING;
			break;
		}
		case PJ_WAITING :
		{
			if(pjob->event_vector & SHUTDOWN_CALLED)
				pjob->state = PJ_SHUTDOWN;
			else if(pjob->event_vector & PAUSE_CALLED)
				pjob->state = PJ_PAUSED;
			else if(pjob->event_vector & SINGLE_SHOT_CALLED)
				pjob->state = PJ_SINGLE_SHOT_ON_WAITING;
			else if(total_time_waited_for >= pjob->period_in_microseconds) // if the total time you waited for is greater than or equal to period_in_microseconds, then start running
				pjob->state = PJ_RUNNING;
			else
				pjob->state = PJ_WAITING;
			break;
		}
		case PJ_SINGLE_SHOT_ON_WAITING :
		{
			if(pjob->event_vector & SHUTDOWN_CALLED)
				pjob->state = PJ_SHUTDOWN;
			else if(pjob->event_vector & PAUSE_CALLED)
				pjob->state = PJ_PAUSED;
			else
				pjob->state = PJ_WAITING;
			break;
		}

		case PJ_SHUTDOWN : // in shutdown state you accept no events
		{
			break;
		}
	}

	// zero out the event vector, to avoid rereading the events
	pjob->event_vector = 0;
}

// internaly function for the periodic job that gets called to run the user's function at fixed intervals
static void* periodic_job_runner(void* pj)
{
	periodic_job* pjob = pj;

	while(1) // keep on looping
	{
		pthread_mutex_lock(&(pjob->job_lock));

			uint64_t total_time_waited_for = 0;
			while(1)
			{
				consume_events_and_update_state(pjob, total_time_waited_for);

				if(pjob->state == PJ_SHUTDOWN) // exit as quickly as possible
				{
					pthread_cond_broadcast(&(pjob->stop_wait)); // broadcast stopping
					pthread_mutex_unlock(&(pjob->job_lock)); // release lock and return early
					return NULL;
				}
				else if(pjob->state == PJ_PAUSED)
				{
					pthread_cond_broadcast(&(pjob->stop_wait)); // broadcast stopping
					pthread_cond_wait(&(pjob->job_wait), &(pjob->job_lock)); // go into a BLOCKING wait
				}
				else if(pjob->state == PJ_WAITING)
				{
					const uint64_t time_to_wait_for = pjob->period_in_microseconds - total_time_waited_for; // this surely is positive, consume_events_and_update_state() ensures that
					uint64_t time_to_wait_for_FINAL = time_to_wait_for;

					pthread_cond_timedwait_for_microseconds(&(pjob->job_wait), &(pjob->job_lock), &time_to_wait_for_FINAL);

					uint64_t wait_time_elapsed = (time_to_wait_for - time_to_wait_for_FINAL);
					total_time_waited_for += wait_time_elapsed;
				}
				else // this is PJ_RUNNING, SINGLE_SHOT_* states and we are meant to now run the periodic_job_function
				{
					break;
				}
			}

		pthread_mutex_unlock(&(pjob->job_lock));

		pjob->periodic_job_function(pjob->input_p);
	}

	return NULL;
}

periodic_job* new_periodic_job(void (*periodic_job_function)(void* input_p), void* input_p, uint64_t period_in_microseconds)
{
	// check the input parameters
	if(periodic_job_function == NULL)
		return NULL;
	if(period_in_microseconds == BLOCKING || period_in_microseconds == NON_BLOCKING)
		return NULL;

	// allocate
	periodic_job* pjob = malloc(sizeof(periodic_job));
	if(pjob == NULL)
		return NULL;

	// initialize all the attributes
	pthread_mutex_init(&(pjob->job_lock), NULL);
	pthread_cond_init_with_monotonic_clock(&(pjob->job_wait));
	pjob->state = PJ_PAUSED;
	pjob->period_in_microseconds = period_in_microseconds;
	pjob->input_p = input_p;
	pjob->periodic_job_function = periodic_job_function;
	pjob->event_vector = 0;
	pthread_cond_init_with_monotonic_clock(&(pjob->stop_wait));

	// start a detached thread
	pthread_t thread_id;
	int error = pthread_create(&thread_id, NULL, periodic_job_runner, pjob);
	if(error)
	{
		pthread_mutex_destroy(&(pjob->job_lock));
		pthread_cond_destroy(&(pjob->job_wait));
		pthread_cond_destroy(&(pjob->stop_wait));
		free(pjob);
		return NULL;
	}
	pthread_detach(thread_id);

	return pjob;
}

void delete_periodic_job(periodic_job* pjob)
{
	// call shutdown for the periodic job
	shutdown_periodic_job(pjob);

	// wait for it to shutdown
	wait_for_pause_or_shutdown_of_periodic_job(pjob);

	pthread_mutex_destroy(&(pjob->job_lock));
	pthread_cond_destroy(&(pjob->job_wait));
	pthread_cond_destroy(&(pjob->stop_wait));
	free(pjob);
}

int update_period_for_periodic_job(periodic_job* pjob, uint64_t period_in_microseconds)
{
	if(period_in_microseconds == BLOCKING || period_in_microseconds == NON_BLOCKING)
		return 0;

	int res = 0;

	pthread_mutex_lock(&(pjob->job_lock));

		res = 1;
		if(res)
		{
			pjob->period_in_microseconds = period_in_microseconds;
			pthread_cond_signal(&(pjob->job_wait)); // signal the periodic job that the event has come
		}

	pthread_mutex_unlock(&(pjob->job_lock));

	return res;
}

uint64_t get_period_for_periodic_job(periodic_job* pjob)
{
	pthread_mutex_lock(&(pjob->job_lock));

		uint64_t period_in_microseconds = pjob->period_in_microseconds;

	pthread_mutex_unlock(&(pjob->job_lock));

	return period_in_microseconds;
}

int resume_periodic_job(periodic_job* pjob)
{
	int res = 0;

	pthread_mutex_lock(&(pjob->job_lock));

		res = (pjob->state == PJ_PAUSED || pjob->state == PJ_SINGLE_SHOT_ON_PAUSED);
		if(res)
		{
			pjob->event_vector |= RESUME_CALLED;
			pthread_cond_signal(&(pjob->job_wait)); // signal the periodic job that the event has come
		}

	pthread_mutex_unlock(&(pjob->job_lock));

	return res;
}

int pause_periodic_job(periodic_job* pjob)
{
	int res = 0;

	pthread_mutex_lock(&(pjob->job_lock));

		res = (pjob->state == PJ_RUNNING || pjob->state == PJ_WAITING || pjob->state == PJ_SINGLE_SHOT_ON_WAITING);
		if(res)
		{
			pjob->event_vector |= PAUSE_CALLED;
			pthread_cond_signal(&(pjob->job_wait)); // signal the periodic job that the event has come
		}

	pthread_mutex_unlock(&(pjob->job_lock));

	return res;
}

int single_shot_periodic_job(periodic_job* pjob)
{
	int res = 0;

	pthread_mutex_lock(&(pjob->job_lock));

		res = (pjob->state == PJ_WAITING || pjob->state == PJ_PAUSED);
		if(res)
		{
			pjob->event_vector |= SINGLE_SHOT_CALLED;
			pthread_cond_signal(&(pjob->job_wait)); // signal the periodic job that the event has come
		}

	pthread_mutex_unlock(&(pjob->job_lock));

	return res;
}

int shutdown_periodic_job(periodic_job* pjob)
{
	int res = 0;

	pthread_mutex_lock(&(pjob->job_lock));

		res = (pjob->state != PJ_SHUTDOWN);
		if(res)
		{
			pjob->event_vector |= SHUTDOWN_CALLED;
			pthread_cond_signal(&(pjob->job_wait)); // signal the periodic job that the event has come
		}

	pthread_mutex_unlock(&(pjob->job_lock));

	return res;
}

void wait_for_pause_or_shutdown_of_periodic_job(periodic_job* pjob)
{
	pthread_mutex_lock(&(pjob->job_lock));

		while(pjob->state != PJ_SHUTDOWN && pjob->state != PJ_PAUSED) // keep on waiting while the state is not (PJ_SHUTDOWN or PJ_PAUSED)
			pthread_cond_wait(&(pjob->stop_wait), &(pjob->job_lock));

	pthread_mutex_unlock(&(pjob->job_lock));
}