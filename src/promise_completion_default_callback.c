#include<boompar/promise_completion_default_callbacks.h>

static void push_to_sync_queue_on_promise_completion_callback_function(promise* p, void* callback_param)
{
	sync_queue* sq = callback_param;
	push_sync_queue_blocking(sq, p, 0);
}

promise_completion_callback push_to_sync_queue_on_promise_completion_callback(sync_queue* sq)
{
	return (promise_completion_callback){.callback_param = sq, .callback_function = push_to_sync_queue_on_promise_completion_callback_function};
}