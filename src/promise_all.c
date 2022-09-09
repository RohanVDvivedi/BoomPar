#include<promise_all.h>

promise_all* new_promise_all(unsigned int promises_count, promise** promises)
{
	promise_all* pa = new_sync_queue((promises_count / 16) + 16, 0);

	for(unsigned int i = 0; i < promises_count; i++)
		register_promise(pa, promises[i]);

	return pa;
}

void initialize_promise_all(promise_all* pa, unsigned int promises_count, promise** promises)
{
	initialize_sync_queue(pa, (promises_count / 16) + 16, 0);

	for(unsigned int i = 0; i < promises_count; i++)
		register_promise(pa, promises[i]);
}

void promise_all_callback_func(void* promise_all_p, const void* promised_result)
{
	promise_all* pa = promise_all_p;
	push_sync_queue_blocking(pa, promised_result, 0);
}

void register_promise(promise_all* pa, promise* p)
{
	callback cb = initialize_callback(pa, promise_all_callback_func);
	add_result_ready_callback(p, cb);
}

array get_promised_results(promise_all* pa, unsigned int result_count)
{
	array results_array;
	initialize_array(&results_array, result_count);

	for(unsigned int i = 0; i < result_count; i++)
	{
		const void* result = pop_sync_queue_blocking(pa, 0);
		set_in_array(&results_array, result, i);
	}

	return results_array;
}

void deinitialize_promise_all(promise_all* pa)
{
	deinitialize_sync_queue(pa);
}

void delete_promise_all(promise_all* pa)
{
	delete_sync_queue(pa);
}