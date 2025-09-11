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
	if(rul_p->shutdown_requested == 1) // fail update if the shutdown was called
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

int request_resources_from_resource_usage_limiter(resource_usage_limiter* rul_p, uint64_t requested_resource_count, uint64_t timeout_in_microseconds, break_resource_waiting* break_out);

uint64_t request_atmost_resources_from_resource_usage_limiter(resource_usage_limiter* rul_p, uint64_t requested_resource_count, uint64_t timeout_in_microseconds, break_resource_waiting* break_out);

int give_back_resources_to_resource_usage_limiter(resource_usage_limiter* rul_p, uint64_t granted_resource_count)
{
	pthread_mutex_lock(&(rul_p->resource_limiter_lock));

	int res = 0;
	if(rul_p->resource_granted_count >= granted_resource_count)
	{
		rul_p->resource_granted_count -= granted_resource_count;

		// if there are any resources left, then wake up all waiters
		if(rul_p->resource_count > rul_p->resource_granted_count)
			pthread_cond_broadcast(&(rul_p->resource_limiter_wait));
	}

	pthread_mutex_unlock(&(rul_p->resource_limiter_lock));

	return 0;
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
			pthread_mutex_wait(&(rul_p->resource_limiter_wait), &(rul_p->resource_limiter_lock));

		pthread_mutex_unlock(&(rul_p->resource_limiter_lock));
	}

	pthread_mutex_destroy(&(rul_p->resource_limiter_lock));
	pthread_cond_destroy(&(rul_p->resource_limiter_wait));

	free(rul_p);
}