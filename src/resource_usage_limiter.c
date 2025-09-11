#include<boompar/resource_usage_limiter.h>

#include<posixutils/pthread_cond_utils.h>

#include<stdlib.h>

resource_usage_limiter* new_resource_usage_limiter(uint64_t resource_count)
{
	if(resource_count == 0)
		return NULL;

	resource_usage_limiter* rul_p = malloc(sizeof(resource_usage_limiter));
	if(rul_p == NULL)
		return NULL;

	pthread_mutex_init(&(rul_p->resource_limiter_lock), NULL);
	pthread_cond_init_with_monotonic_clock(&(rul_p->resource_limiter_wait));
	rul_p->resource_count = resource_count;
	rul_p->resource_granted_count = 0;
	rul_p->shutdown_requested = 0;

	return rul_p;
}

uint64_t get_resource_count(resource_usage_limiter* rul_p)
{
	pthread_mutex_lock(&(rul_p->resource_limiter_lock));

	uint64_t resource_count = rul_p->resource_count;

	pthread_mutex_unlock(&(rul_p->resource_limiter_lock));

	return resource_count;
}

int set_resource_count(resource_usage_limiter* rul_p, uint64_t new_resource_count)
{
	// resource_count can never become 0
	if(new_resource_count == 0)
		return 0;

	pthread_mutex_lock(&(rul_p->resource_limiter_lock));

	int res = 0;
	if(!(rul_p->shutdown_requested)) // fail update if the shutdown was called
	{
		// if we end up incrementing the resource_count, then wake up potential requesters
		if(rul_p->resource_count < new_resource_count)
			pthread_cond_broadcast(&(rul_p->resource_limiter_wait));

		rul_p->resource_count = new_resource_count;
		res = 1;
	}

	pthread_mutex_unlock(&(rul_p->resource_limiter_lock));

	return res;
}

// simple thread unsafe utility to be used only with the locks held
static uint64_t get_resources_left(const resource_usage_limiter* rul_p)
{
	if(rul_p->resource_count >= rul_p->resource_granted_count)
		return rul_p->resource_count - rul_p->resource_granted_count;

	// there are more resources granted than the total resources left for some unknown reason, so return 0
	return 0;
}

uint64_t request_resources_from_resource_usage_limiter(resource_usage_limiter* rul_p, uint64_t min_resource_count, uint64_t max_resource_count, uint64_t timeout_in_microseconds, break_resource_waiting* break_out)
{
	// make sure that we have a valid range
	if(min_resource_count > max_resource_count)
		return 0;

	// this means no resources have been requested
	// min_resource_count by definition has to be zero
	if(0 == max_resource_count)
		return 0;

	uint64_t res = 0;

	pthread_mutex_lock(&(rul_p->resource_limiter_lock));

		// attempt to block only if you are allowed to
		if(timeout_in_microseconds != NON_BLOCKING)
		{
			int wait_error = 0;

			// keep on looping while
			while((!rul_p->shutdown_requested) && ((*break_out) == 0) &&  // there is no shutdown and no breakout requested
				(get_resources_left(rul_p) < min_resource_count) && // and as long as there are lesser resources than out minimum requirement
				!wait_error) // and there is no wait error
			{
				if(timeout_in_microseconds == BLOCKING)
					wait_error = pthread_cond_wait(&(rul_p->resource_limiter_wait), &(rul_p->resource_limiter_lock));
				else
					wait_error = pthread_cond_timedwait_for_microseconds(&(rul_p->resource_limiter_wait), &(rul_p->resource_limiter_lock), &timeout_in_microseconds);
			}
		}

		// first ensure that no shutdown was requested and there were no calls for us to break out
		if((!rul_p->shutdown_requested) && ((*break_out) == 0))
		{
			uint64_t resources_left = get_resources_left(rul_p);
			if(resources_left >= min_resource_count) // then ensure that there are more resources than what we want
			{
				res = max(min(resources_left, max_resource_count), min_resource_count); // take min with max value and max with min value, to figure out what we would grant this requested
				rul_p->resource_granted_count += res;
			}
		}

	pthread_mutex_unlock(&(rul_p->resource_limiter_lock));

	return res;
}

int give_back_resources_to_resource_usage_limiter(resource_usage_limiter* rul_p, uint64_t granted_resource_count)
{
	// this is a NOP call
	if(granted_resource_count == 0)
		return 1;

	pthread_mutex_lock(&(rul_p->resource_limiter_lock));

	int res = 0;
	if(rul_p->resource_granted_count >= granted_resource_count)
	{
		rul_p->resource_granted_count -= granted_resource_count;

		// if there are any resources left, then wake up all waiters
		if(rul_p->resource_count > rul_p->resource_granted_count) // this check could fail if the number of resource_count was recently decreased to a lower value
			pthread_cond_broadcast(&(rul_p->resource_limiter_wait));
	}

	pthread_mutex_unlock(&(rul_p->resource_limiter_lock));

	return res;
}

void break_out_from_resource_usage_limiter(resource_usage_limiter* rul_p, break_resource_waiting* break_out)
{
	pthread_mutex_lock(&(rul_p->resource_limiter_lock));

	if((*break_out) == 0) // if not broken out earlier, only then break_out now
	{
		(*break_out) = 1;
		pthread_cond_broadcast(&(rul_p->resource_limiter_wait));
	}

	pthread_mutex_unlock(&(rul_p->resource_limiter_lock));
}

void shutdown_resource_usage_limiter(resource_usage_limiter* rul_p)
{
	pthread_mutex_lock(&(rul_p->resource_limiter_lock));

	// set shutdown requested and wake up everyone
	rul_p->shutdown_requested = 1;
	pthread_cond_broadcast(&(rul_p->resource_limiter_wait));

	pthread_mutex_unlock(&(rul_p->resource_limiter_lock));
}

void delete_resource_usage_limiter(resource_usage_limiter* rul_p, int wait_for_resources_to_be_returned)
{
	shutdown_resource_usage_limiter(rul_p);

	// if we are required to wait for the resource
	// we wait until all the resources are returned
	if(wait_for_resources_to_be_returned)
	{
		pthread_mutex_lock(&(rul_p->resource_limiter_lock));

		while(rul_p->resource_granted_count > 0)
			pthread_cond_wait(&(rul_p->resource_limiter_wait), &(rul_p->resource_limiter_lock));

		pthread_mutex_unlock(&(rul_p->resource_limiter_lock));
	}

	pthread_mutex_destroy(&(rul_p->resource_limiter_lock));
	pthread_cond_destroy(&(rul_p->resource_limiter_wait));

	free(rul_p);
}