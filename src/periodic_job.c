#include<boompar/periodic_job.h>

#include<posixutils/pthread_cond_utils.h>

#include<stdlib.h>

static inline void consume_events_and_update_state(periodic_job* pjob, uint64_t total_time_waited_for)
{
	switch(pjob->state)
	{
		case PAUSED :
		{
			if(pjob->event_vector & SHUTDOWN_CALLED)
				pjob->state = SHUTDOWN;
			else if(pjob->event_vector & RESUME_CALLED)
				pjob->state = RUNNING;
			else if(pjob->event_vector & SINGLE_SHOT_CALLED)
				pjob->state = SINGLE_SHOT_ON_PAUSED;
			else
				pjob->state = PAUSED;
			break;
		}
		case SINGLE_SHOT_ON_PAUSED :
		{
			if(pjob->event_vector & SHUTDOWN_CALLED)
				pjob->state = SHUTDOWN;
			else if(pjob->event_vector & RESUME_CALLED)
				pjob->state = WAITING;
			else
				pjob->state = PAUSED;
			break;
		}

		case RUNNING :
		{
			if(pjob->event_vector & SHUTDOWN_CALLED)
				pjob->state = SHUTDOWN;
			else if(pjob->event_vector & PAUSE_CALLED)
				pjob->state = PAUSED;
			else
				pjob->state = WAITING;
			break;
		}
		case WAITING :
		{
			if(pjob->event_vector & SHUTDOWN_CALLED)
				pjob->state = SHUTDOWN;
			else if(pjob->event_vector & PAUSE_CALLED)
				pjob->state = PAUSED;
			else if(pjob->event_vector & SINGLE_SHOT_CALLED)
				pjob->state = SINGLE_SHOT_ON_WAITING;
			else if(total_time_waited_for >= pjob->period_in_microseconds) // if the total time you waited for is greater than or equal to period_in_microseconds, then start running
				pjob->state = RUNNING;
			else
				pjob->state = WAITING;
			break;
		}
		case SINGLE_SHOT_ON_WAITING :
		{
			if(pjob->event_vector & SHUTDOWN_CALLED)
				pjob->state = SHUTDOWN;
			else if(pjob->event_vector & PAUSE_CALLED)
				pjob->state = PAUSED;
			else
				pjob->state = WAITING;
			break;
		}

		case SHUTDOWN : // in shutdown state you accept no events
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

				if(pjob->state == SHUTDOWN) // exit as quickly as possible
				{
					pthread_cond_broadcast(&(pjob->stop_wait)); // broadcast stopping
					pthread_mutex_unlock(&(pjob->job_lock)); // release lock and return early
					return NULL;
				}
				else if(pjob->state == PAUSED)
				{
					pthread_cond_broadcast(&(pjob->stop_wait)); // broadcast stopping
					pthread_cond_wait(&(pjob->job_wait), &(pjob->job_lock)); // go into a BLOCKING wait
				}
				else if(pjob->state == WAITING)
				{
					const uint64_t time_to_wait_for = pjob->period_in_microseconds - total_time_waited_for; // this surely is positive, consume_events_and_update_state() ensures that
					uint64_t time_to_wait_for_FINAL = time_to_wait_for;

					pthread_cond_timedwait_for_microseconds(&(pjob->job_wait), &(pjob->job_lock), &time_to_wait_for_FINAL);

					uint64_t wait_time_elapsed = (time_to_wait_for - time_to_wait_for_FINAL);
					total_time_waited_for += wait_time_elapsed;
				}
				else // this is RUNNING, SINGLE_SHOT_* events and we are meant to now run the periodic_job_function
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
	pjob->state = PAUSED;
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

int resume_periodic_job(periodic_job* pjob)
{
	int res = 0;

	pthread_mutex_lock(&(pjob->job_lock));

		res = (pjob->state == PAUSED || pjob->state == SINGLE_SHOT_ON_PAUSED);
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

		res = (pjob->state == RUNNING || pjob->state == WAITING || pjob->state == SINGLE_SHOT_ON_WAITING);
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

		res = (pjob->state == WAITING || pjob->state == PAUSED);
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

		res = (pjob->state != SHUTDOWN);
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

		while(pjob->state != SHUTDOWN && pjob->state != PAUSED) // keep on waiting while the state is not (SHUTDOWN or PAUSED)
			pthread_cond_wait(&(pjob->stop_wait), &(pjob->job_lock));

	pthread_mutex_unlock(&(pjob->job_lock));
}