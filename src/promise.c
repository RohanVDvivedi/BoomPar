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
	p->promise_completed_queue = NULL;
}

int set_promised_result(promise* p, void* res)
{
	int was_promised_result_set = 0;

	pthread_mutex_lock(&(p->promise_lock));

	// push this promise to this queue, if the promise is completed
	sync_queue* push_promise_to = NULL;

	if(!p->output_result_ready)
	{
		// set the result
		p->output_result = res;

		was_promised_result_set = 1;

		// set the result is ready and wake up all the threads that are waiting for the result
		p->output_result_ready = 1;
		pthread_cond_broadcast(&(p->promise_wait));

		// push this promise to this queue, if the promise is completed
		push_promise_to = p->promise_completed_queue;
	}

	pthread_mutex_unlock(&(p->promise_lock));

	// push the promise object if it was completed and the promise_completed_queue exists
	if(push_promise_to)
		push_sync_queue_blocking(push_promise_to, p, 0);

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

int set_promise_completed_queue(promise* p, sync_queue* promise_completed_queue)
{
	int was_promise_completed_queue_set = 0;

	pthread_mutex_lock(&(p->promise_lock));

	// set the promise completed queue
	if(p->promise_completed_queue == NULL)
	{
		p->promise_completed_queue = promise_completed_queue;
		was_promise_completed_queue_set = 1;
	}

	int is_result_ready = p->output_result_ready;

	pthread_mutex_unlock(&(p->promise_lock));

	// else if the result was ready then we push this promise to the promise_completed_queue
	if(is_result_ready && was_promise_completed_queue_set)
		push_sync_queue_blocking(promise_completed_queue, p, 0);

	// return if the promise_completed_queue was set
	return was_promise_completed_queue_set;
}

void deinitialize_promise(promise* p)
{
	p->promise_completed_queue = NULL;
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