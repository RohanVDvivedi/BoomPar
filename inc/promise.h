#ifndef PROMISE_H
#define PROMISE_H

#include<pthread.h>
#include<stdlib.h>

#include<sync_queue.h>

typedef struct promise promise;
struct promise
{
	// signifies whether output result is set by the producer threads
	// this value is initialized as 0 and can only be fliped from 0 -> 1
	// IT SHOULD/WILL NOT BE FLIPPED BACK AND FROTH
	int output_result_ready:1;

	void* output_result;

	pthread_mutex_t promise_lock;

	// threads wait on this conditional variable until promise value could be obtained
	pthread_cond_t promise_wait;

	// push this promise to the below sync_queue, after it is completed
	// this is not set by default, it is an optional parameter
	sync_queue* promise_completed_queue;
};

promise* new_promise();

void initialize_promise(promise* p);

// set_result must be called only once after the promise has been initialized or newly created with get_promise function
// it will fail with a 0, if the promised result was already set
int set_promised_result(promise* p, void* res);

// get result will not block, if the promise is already ready
void* get_promised_result(promise* p);

// to check if a promised result is ready, kind of non blocking way to access a promise
int is_promised_result_ready(promise* p);

// it sets the promise_completed_queue, if it is not alread set
// if the result is fulfilled then it calls the callback right away, else it queues the callback to callbacks_requested queue
int set_promise_completed_queue(promise* p, sync_queue* promise_completed_queue);

void deinitialize_promise(promise* p);

void delete_promise(promise* p);

#endif