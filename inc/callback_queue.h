#ifndef CALLBACK_QUEUE_H
#define CALLBACK_QUEUE_H

#include<dstream.h>

#include<callback.h>

/*
	This queue is essentially a wrapper over dstream of cutlery to hold callbacks
	It is not thread safe, you need your own locks to make its access thread safe
	It can only hold callback structs (as defined in callback.h), It is flat structure.
*/

// dstream is itself a callback queue
typedef dstream callback_queue;

void initialize_callback_queue(callback_queue* cbq, unsigned int capacity);

int is_empty_callback_queue(const callback_queue* cbq);

void push_callback(callback_queue* cbq, callback cb);

callback pop_callback(callback_queue* cbq);

void deinitialize_callback_queue(callback_queue* cbq);

#endif