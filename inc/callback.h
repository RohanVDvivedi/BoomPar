#ifndef CALLBACK_H
#define CALLBACK_H

// This struct is nothing but a collection of 2 things 1 being the a callback function and the callback params
// that this callback function gets called with

typedef struct callback callback;
struct callback
{
	const void* callback_param;

	void (*callback_func)(void*, const void* callback_param);
};

callback initialize_callback(const void* callback_param, void (*callback_func)(void*, const void* callback_param));

void call_callback(const callback* cb, void* result);

#endif