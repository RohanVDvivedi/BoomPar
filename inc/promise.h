#ifndef PROMISE_H
#define PROMISE_H

#include<pthread.h>
#include<stdlib.h>

typedef struct promise promise;

typedef struct promise_completion_callback promise_completion_callback;
struct promise_completion_callback
{
	void* callback_param;
	void (*callback_function)(promise* p, void* callback_param);
};

struct promise
{
	// signifies whether output result, i.e. is set by the producer thread
	// this value is initialized as 0 and can only be fliped from 0 -> 1
	int output_result_ready:1;

	void* output_result;

	pthread_mutex_t promise_lock;

	// threads wait on this conditional variable until promise value could be obtained
	pthread_cond_t promise_wait;

	// callback to be executed upon completion of the promise
	promise_completion_callback* promised_callback;
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

// it sets the promise_completion_callback, if it is not alread set
// this can be done before or after the promise has been resolved
int set_promised_callback(promise* p, promise_completion_callback* promised_callback);

void deinitialize_promise(promise* p);

void delete_promise(promise* p);

#endif