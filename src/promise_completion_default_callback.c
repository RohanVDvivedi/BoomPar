#include<promise_completion_default_callbacks.h>

static void push_to_sync_queue_on_promise_completion_callback_fundtion(promise* p, void* callback_param)
{
	sync_queue* sq = callback_param;
	push_sync_queue_blocking(sq, p, 0);
}

promise_completed_callback push_to_sync_queue_on_promise_completion(sync_queue* sq)
{
	return (promise_completed_callback){.callback_param = sq, .callback_function = push_to_sync_queue_on_promise_completion_callback_fundtion};
}