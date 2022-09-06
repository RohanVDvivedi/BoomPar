#include<promise.h>

promise* new_promise()
{
	promise* p = malloc(sizeof(promise));
	initialize_promise(p);
	return p;
}

void initialize_promise(promise* p)
{
	p->output_result_ready = 0;
	p->output_result = NULL;
	pthread_mutex_init(&(p->promise_lock), NULL);
	pthread_cond_init(&(p->promise_wait), NULL);
	initialize_queue(&(p->callbacks_requested), 0);
}

// call this function only after the output_result_ready has been set
static void call_all_requested_callbacks(promise* p)
{
	while(1)
	{
		// use take the lock on to pop
		pthread_mutex_lock(&(p->promise_lock));

			callback* cb = (callback*) get_top_of_queue(&(p->callbacks_requested));
			pop_from_queue(&(p->callbacks_requested));

			int is_empty = is_empty_queue(&(p->callbacks_requested));

		pthread_mutex_unlock(&(p->promise_lock));

		// the actual callback function is called without any locks taken
		if(cb != NULL)
		{
			call_callback(cb, p->output_result);
			delete_callback(cb);
		}

		// if nothing was popped, then the  callback queue is probably empty
		if(is_empty)
			break;
	}
}

int set_promised_result(promise* p, void* res)
{
	int was_promised_result_set = 0;

	pthread_mutex_lock(&(p->promise_lock));

	if(!p->output_result_ready)
	{
		// set the result
		p->output_result = res;

		was_promised_result_set = 1;

		// set the result is ready and wake up all the threads that are waiting for the result
		p->output_result_ready = 1;
		pthread_cond_broadcast(&(p->promise_wait));
	}

	pthread_mutex_unlock(&(p->promise_lock));

	// call all the callbacks
	call_all_requested_callbacks(p);

	return was_promised_result_set;
}

void* get_promised_result(promise* p)
{
	pthread_mutex_lock(&(p->promise_lock));

	// wait while the result is not ready
	while(p->output_result_ready == 0)
	{
		pthread_cond_wait(&(p->promise_wait), &(p->promise_lock));
	}

	// get the result
	void* res = p->output_result;

	pthread_mutex_unlock(&(p->promise_lock));

	return res;
}

int is_promised_result_ready(promise* p)
{
	pthread_mutex_lock(&(p->promise_lock));

	int is_result_ready = p->output_result_ready;

	pthread_mutex_unlock(&(p->promise_lock));

	return is_result_ready;
}

void add_result_ready_callback(promise* p, const void* callback_param, void (*callback_func)(void*, const void* callback_param))
{
	// create a new callback object
	callback* cb = new_callback(callback_param, callback_func);

	pthread_mutex_lock(&(p->promise_lock));

	int is_result_ready = p->output_result_ready;

	// if the result is not ready then push the callback to the queue
	if(!is_result_ready)
	{
		// push to queue, expand if the push fails
		if(!push_to_queue(&(p->callbacks_requested), cb))
		{
			expand_queue(&(p->callbacks_requested));
			push_to_queue(&(p->callbacks_requested), cb);
		}
	}

	pthread_mutex_unlock(&(p->promise_lock));

	// else if the result was ready then we call the callbac right away
	if(is_result_ready)
	{
		call_callback(cb, p->output_result);
		delete_callback(cb);
	}
}

void deinitialize_promise(promise* p)
{
	// delete all callbacks
	while(!is_empty_queue(&(p->callbacks_requested)))
	{
		callback* cb = (callback*) get_top_of_queue(&(p->callbacks_requested));
		pop_from_queue(&(p->callbacks_requested));
		delete_callback(cb);
	}

	deinitialize_queue(&(p->callbacks_requested));
	p->output_result_ready = 0;
	p->output_result = NULL;
	pthread_mutex_destroy(&(p->promise_lock));
	pthread_cond_destroy(&(p->promise_wait));
}

void delete_promise(promise* p)
{
	deinitialize_promise(p);
	free(p);
}