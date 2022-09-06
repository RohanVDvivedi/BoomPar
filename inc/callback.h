#ifndef CALLBACK_H
#define CALLBACK_H

typedef struct callback callback;
struct callback
{
	const void* callback_param;

	void (*callback_func)(void*, const void* callback_param);
};

callback* get_new_callback(const void* callback_param, void (*callback_func)(void*, const void* callback_param));

void call_callback(const callback* cb, void* result);

void delete_callback(callback* cb);

#endif