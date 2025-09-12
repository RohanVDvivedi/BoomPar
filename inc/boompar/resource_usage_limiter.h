#ifndef RESOURCE_USAGE_LIMITER_H
#define RESOURCE_USAGE_LIMITER_H

#include<stdint.h>
#include<pthread.h>

typedef struct resource_usage_limiter resource_usage_limiter;
struct resource_usage_limiter
{
	// global lock of the resource_usage_lock
	pthread_mutex_t resource_limiter_lock;

	// anyone waiting on a particular resource waits here, so as to be granted the resource
	pthread_cond_t resource_limiter_wait;

	// total number of resources of a type that are maintained
	uint64_t resource_count;

	// total number of resources that have been granted to the grantees up until now
	uint64_t resource_granted_count;

	// once shutdown_requested is set to 1, no one can wait on the resource_limiter_wait
	int shutdown_requested;
};

// create a new resource_usage_limiter
resource_usage_limiter* new_resource_usage_limiter(uint64_t resource_count);

// get the total number of resources managed by the resource_usage_limiter
uint64_t get_resource_count(resource_usage_limiter* rul_p);

// set the total number of resources managed by the resource_usage_limiter
// also wakes up all the waiters
// returns 0 and fails if (new_resource_count == 0) || (shutdown_requested == 1)
int set_resource_count(resource_usage_limiter* rul_p, uint64_t new_resource_count);

// an int typedef-ed, for the number of resources waiting
typedef int break_resource_waiting;
#define INIT_BREAK_OUT 0

/*
	You generally need to wait for a resource to be available, but there are times when the wake up is needed in cases like when an external user input needs to quit
	this is when you call the break_out_from_resource_usage_limiter() to set a flag which forces all waiters on a resource (waiting on that same break_out variable) to quit forcibly
*/

// request a fixed number of resources from the resource_usage_limiter, with the timeout, that can also be BLOCKING or NON_BLOCKING
// invalid min-max ranges will result in a NOOP operation
// return value is the number of resources granted
uint64_t request_resources_from_resource_usage_limiter(resource_usage_limiter* rul_p, uint64_t min_resource_count, uint64_t max_resource_count, uint64_t timeout_in_microseconds, break_resource_waiting* break_out);

// give up resources back to the resource_usage_limiter, and wake up all the waiters (only if resource_count > resource_granted_count)
// granted_resource_count == 0, resoult in a NOOP
int give_back_resources_to_resource_usage_limiter(resource_usage_limiter* rul_p, uint64_t granted_resource_count);

// sets the break_out variable to 1, and wakes up all waiters so they can check who needs to break_out of waiting
void break_out_from_resource_usage_limiter(resource_usage_limiter* rul_p, break_resource_waiting* break_out);

// shutsdown the resource_usage_limiter, by setting the shutdown_requested
// and wakes up all waiters
void shutdown_resource_usage_limiter(resource_usage_limiter* rul_p);

// shutdowns the resource_usage_limiter, waits for all the resources to be given back and then shuts down
void delete_resource_usage_limiter(resource_usage_limiter* rul_p, int wait_for_resources_to_be_returned);

#endif