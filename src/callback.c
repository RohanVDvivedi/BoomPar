#include<callback.h>

#include<stdlib.h>

callback initialize_callback(const void* callback_param, void (*callback_func)(void*, const void* callback_param))
{
	return (callback){.callback_param = callback_param, .callback_func = callback_func};
}

void call_callback(const callback* cb, void* result)
{
	cb->callback_func(result, cb->callback_param);
}