#include<boompar/alarm_job.h>

#include<posixutils/pthread_cond_utils.h>

#include<stdlib.h>

// below are the events that can be placed in the event_vector, in the priority of their processing
// this macros are for internal use
#define SHUTDOWN_CALLED    (1<<0) // can be called on all states except AJ_SHUTDOWN itself
#define PAUSE_CALLED       (1<<1) // can be called on only AJ_RUNNING and AJ_WAITING states
#define WAKE_UP_CALLED     (1<<2) // can be called on only AJ_PAUSED and AJ_WAITING states

static inline void consume_events_and_update_state(alarm_job* ajob, uint64_t total_time_waited_for)
{
	switch(ajob->state)
	{
		case AJ_PAUSED :
		{
			if(ajob->event_vector & SHUTDOWN_CALLED)
				ajob->state = AJ_SHUTDOWN;
			else if(ajob->event_vector & WAKE_UP_CALLED)
				ajob->state = AJ_RUNNING;
			else
				ajob->state = AJ_PAUSED;
			break;
		}

		case AJ_RUNNING :
		{
			if(ajob->event_vector & SHUTDOWN_CALLED)
				ajob->state = AJ_SHUTDOWN;
			else if(ajob->event_vector & PAUSE_CALLED)
				ajob->state = AJ_PAUSED;
			else
				ajob->state = AJ_WAITING;
			break;
		}
		case AJ_WAITING :
		{
			if(ajob->event_vector & SHUTDOWN_CALLED)
				ajob->state = AJ_SHUTDOWN;
			else if(ajob->event_vector & PAUSE_CALLED)
				ajob->state = AJ_PAUSED;
			else if(ajob->event_vector & WAKE_UP_CALLED)
				ajob->state = AJ_RUNNING;
			else if(total_time_waited_for >= ajob->period_in_microseconds) // if the total time you waited for is greater than or equal to period_in_microseconds, then start running
				ajob->state = AJ_RUNNING;
			else
				ajob->state = AJ_WAITING;
			break;
		}

		case AJ_SHUTDOWN : // in shutdown state you accept no events
		{
			break;
		}
	}

	// zero out the event vector, to avoid rereading the events
	ajob->event_vector = 0;
}

// internaly function for the alarm job that gets called to run the user's function
static void* alarm_job_runner(void* aj)
{
	alarm_job* ajob = aj;

	while(1) // keep on looping
	{
		pthread_mutex_lock(&(ajob->job_lock));

			// in order to not miss updates we grab the next alarming microseconds, the microseconds after which we need to run the alarm_job_function again
			// we can just hope that this function is as short and non blocking as possible, otherwise it can halt the whole system
			ajob->period_in_microseconds = ajob->alarm_set_function(ajob->input_p);

			uint64_t total_time_waited_for = 0;
			while(1)
			{

				consume_events_and_update_state(ajob, total_time_waited_for);

				if(ajob->state == AJ_SHUTDOWN) // exit as quickly as possible
				{
					pthread_cond_broadcast(&(ajob->stop_wait)); // broadcast stopping
					pthread_mutex_unlock(&(ajob->job_lock)); // release lock and return early
					return NULL;
				}
				else if(ajob->state == AJ_PAUSED)
				{
					pthread_cond_broadcast(&(ajob->stop_wait)); // broadcast stopping
					pthread_cond_wait(&(ajob->job_wait), &(ajob->job_lock)); // go into a BLOCKING wait
				}
				else if(ajob->state == AJ_WAITING)
				{
					const uint64_t time_to_wait_for = ajob->period_in_microseconds - total_time_waited_for; // this surely is positive, consume_events_and_update_state() ensures that
					uint64_t time_to_wait_for_FINAL = time_to_wait_for;

					pthread_cond_timedwait_for_microseconds(&(ajob->job_wait), &(ajob->job_lock), &time_to_wait_for_FINAL);

					uint64_t wait_time_elapsed = (time_to_wait_for - time_to_wait_for_FINAL);
					total_time_waited_for += wait_time_elapsed;
				}
				else // this is AJ_RUNNING state and we are meant to now run the alarm_job_function
				{
					break;
				}
			}

		pthread_mutex_unlock(&(ajob->job_lock));

		ajob->alarm_job_function(ajob->input_p);
	}

	return NULL;
}

alarm_job* new_alarm_job(uint64_t (*alarm_set_function)(void* input_p), void (*alarm_job_function)(void* input_p), void* input_p)
{
	// check the input parameters
	if(alarm_job_function == NULL)
		return NULL;

	// allocate
	alarm_job* ajob = malloc(sizeof(alarm_job));
	if(ajob == NULL)
		return NULL;

	// initialize all the attributes
	pthread_mutex_init(&(ajob->job_lock), NULL);
	pthread_cond_init_with_monotonic_clock(&(ajob->job_wait));
	ajob->state = AJ_PAUSED;
	ajob->period_in_microseconds = BLOCKING;
	ajob->input_p = input_p;
	ajob->alarm_set_function = alarm_set_function;
	ajob->alarm_job_function = alarm_job_function;
	ajob->event_vector = 0;
	pthread_cond_init_with_monotonic_clock(&(ajob->stop_wait));

	// start a detached thread
	pthread_t thread_id;
	int error = pthread_create(&thread_id, NULL, alarm_job_runner, ajob);
	if(error)
	{
		pthread_mutex_destroy(&(ajob->job_lock));
		pthread_cond_destroy(&(ajob->job_wait));
		pthread_cond_destroy(&(ajob->stop_wait));
		free(ajob);
		return NULL;
	}
	pthread_detach(thread_id);

	return ajob;
}

void delete_alarm_job(alarm_job* ajob)
{
	// call shutdown for the alarm job
	shutdown_alarm_job(ajob);

	// wait for it to shutdown
	wait_for_pause_or_shutdown_of_alarm_job(ajob);

	pthread_mutex_destroy(&(ajob->job_lock));
	pthread_cond_destroy(&(ajob->job_wait));
	pthread_cond_destroy(&(ajob->stop_wait));
	free(ajob);
}

int pause_alarm_job(alarm_job* ajob)
{
	int res = 0;

	pthread_mutex_lock(&(ajob->job_lock));

		res = (ajob->state == AJ_RUNNING || ajob->state == AJ_WAITING);
		if(res)
		{
			ajob->event_vector |= PAUSE_CALLED;
			pthread_cond_signal(&(ajob->job_wait)); // signal the alarm job that the event has come
		}

	pthread_mutex_unlock(&(ajob->job_lock));

	return res;
}

int wake_up_alarm_job(alarm_job* ajob)
{
	int res = 0;

	pthread_mutex_lock(&(ajob->job_lock));

		res = (ajob->state == AJ_PAUSED || ajob->state == AJ_WAITING);
		if(res)
		{
			ajob->event_vector |= WAKE_UP_CALLED;
			pthread_cond_signal(&(ajob->job_wait)); // signal the alarm job that the event has come
		}

	pthread_mutex_unlock(&(ajob->job_lock));

	return res;
}

int shutdown_alarm_job(alarm_job* ajob)
{
	int res = 0;

	pthread_mutex_lock(&(ajob->job_lock));

		res = (ajob->state != AJ_SHUTDOWN);
		if(res)
		{
			ajob->event_vector |= SHUTDOWN_CALLED;
			pthread_cond_signal(&(ajob->job_wait)); // signal the alarm job that the event has come
		}

	pthread_mutex_unlock(&(ajob->job_lock));

	return res;
}

void wait_for_pause_or_shutdown_of_alarm_job(alarm_job* ajob)
{
	pthread_mutex_lock(&(ajob->job_lock));

		while(ajob->state != AJ_SHUTDOWN && ajob->state != AJ_PAUSED) // keep on waiting while the state is not (AJ_SHUTDOWN or AJ_PAUSED)
			pthread_cond_wait(&(ajob->stop_wait), &(ajob->job_lock));

	pthread_mutex_unlock(&(ajob->job_lock));
}