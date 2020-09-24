#include<promise.h>

promise* get_promise()
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
}

void set_promised_result(promise* p, void* res)
{
	pthread_mutex_lock(&(p->promise_lock));

	// set the result
	p->output_result = res;

	// set the result is ready and wake up all the threads that are waiting for the result
	p->output_result_ready = 1;
	pthread_cond_broadcast(&(p->promise_wait));

	pthread_mutex_unlock(&(p->promise_lock));
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

void deinitialize_promise(promise* p)
{
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