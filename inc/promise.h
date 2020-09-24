#ifndef PROMISE_H
#define PROMISE_H

#include<pthread.h>
#include<stdlib.h>

typedef struct promise promise;
struct promise
{
	// signifies whether output result is set by the producer threads
	int output_result_ready;

	void* output_result;

	pthread_mutex_t promise_lock;

	// threads wait on this conditional variable until promise value could be obtained
	pthread_cond_t promise_wait;
};

promise* get_promise();

void initialize_promise(promise* p);

// set_result must be called only once after the promise has been initialized or newly created with get_promise function
void set_promised_result(promise* p, void* res);

// get result will not block, if the promise is already ready
void* get_promised_result(promise* p);

// to check if a promised result is ready, kind of non blocking way to access a promise
int is_promised_result_ready(promise* p);

void deinitialize_promise(promise* p);

void delete_promise(promise* p);

#endif