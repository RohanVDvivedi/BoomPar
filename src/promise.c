#include<promise.h>

promise* new_promise()
{
	promise* p = malloc(sizeof(promise));
	if(p != NULL)
		initialize_promise(p);
	return p;
}

void initialize_promise(promise* p)
{
	p->output_result_ready = 0;
	p->output_result = NULL;
	pthread_mutex_init(&(p->promise_lock), NULL);
	pthread_cond_init(&(p->promise_wait), NULL);
	p->promise_completion_callback = NULL;
}

int set_promised_result(promise* p, void* res)
{
	int was_promised_result_set = 0;

	pthread_mutex_lock(&(p->promise_lock));

	// run this callback if it exists, without the lock of the promise
	promise_completed_callback* pcc_to_execute = NULL;

	if(!p->output_result_ready)
	{
		// set the result
		p->output_result = res;

		was_promised_result_set = 1;

		// set the result is ready and wake up all the threads that are waiting for the result
		p->output_result_ready = 1;
		pthread_cond_broadcast(&(p->promise_wait));

		// if the output is set, then also set the promise_completion_callback that we will have to run
		pcc_to_execute = p->promise_completion_callback;
	}

	pthread_mutex_unlock(&(p->promise_lock));

	// we run the callback after releasing the lock
	if(pcc_to_execute && was_promised_result_set)
		pcc_to_execute->callback_function(p, pcc_to_execute->callback_param);

	return was_promised_result_set;
}

void* get_promised_result(promise* p)
{
	pthread_mutex_lock(&(p->promise_lock));

	// wait while the result is not ready
	while(p->output_result_ready == 0)
		pthread_cond_wait(&(p->promise_wait), &(p->promise_lock));

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

int set_promise_completion_callback(promise* p, promise_completed_callback* promise_completion_callback)
{
	int was_promise_completion_callback_set = 0;

	pthread_mutex_lock(&(p->promise_lock));

	// set the promise completed queue
	if(p->promise_completion_callback == NULL)
	{
		p->promise_completion_callback = promise_completion_callback;
		was_promise_completion_callback_set = 1;
	}

	int is_result_ready = p->output_result_ready;

	pthread_mutex_unlock(&(p->promise_lock));

	// if the result was ready and the promise completion callback was set, then call that callback
	// we must run the callback only after we have release the lock
	if(is_result_ready && was_promise_completion_callback_set)
		promise_completion_callback->callback_function(p, promise_completion_callback->callback_param);

	return was_promise_completion_callback_set;
}

void deinitialize_promise(promise* p)
{
	p->promise_completion_callback = NULL;
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