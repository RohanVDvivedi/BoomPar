#include<callback.h>

#include<stdlib.h>

callback* get_new_callback(const void* callback_param, void (*callback_func)(void*, const void* callback_param))
{
	callback* cb = malloc(sizeof(callback));
	cb->callback_param = callback_param;
	cb->callback_func = callback_func;
	return cb;
}

void call_callback(const callback* cb, void* result)
{
	cb->callback_func(result, cb->callback_param);
}

void delete_callback(callback* cb)
{
	free(cb);
}