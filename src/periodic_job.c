#include<boompar/periodic_job.h>

#include<posixutils/pthread_cond_utils.h>

#include<stdlib.h>

static inline void consume_events_and_update_state(periodic_job* pjob)
{
	// TODO
}

// internaly function for the periodic job that gets called to run the user's function at fixed intervals
static void* periodic_job_runner(void* pj)
{
	// TODO
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

		pjob->period_in_microseconds = period_in_microseconds;
		res = 1;
		pthread_cond_signal(&(pjob->job_wait)); // signal the periodic job that the event has come

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

		res = (pjob->state != SHUTDOWN && pjob->state != PAUSED);
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
			pjob->event_vector |= RESUME_CALLED;
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