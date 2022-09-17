#include<stdio.h>
//#include<unistd.h> //only for usleep

#include<job.h>
#include<sync_queue.h>

void* my_job_function_simple(void* my_int)
{
	int i = (*((int*)my_int));
	printf("%d ided Job simple prints => [%d]\n", (int)pthread_self(), i);
	//usleep(3 * 1000 * 1000);
	(*((int*)my_int)) += 1000;
	return my_int;
}

void* job_waiting(void* job_to_wait)
{
	promise* simple_promise_p = job_to_wait;
	int* result = get_promised_result(simple_promise_p);
	printf("%d waiting thread prints => [%d]\n", (int)pthread_self(), (*result));
	return NULL;
}

#define WATING_JOB_COUNT 10

//#define WAIT_ANXIOUSLY
#define WAIT_USING_PROMISE_QUEUE

int main()
{
	printf("----Testing---- Job async from thread %d\n", (int)pthread_self());

	int input = 5054;
	promise simple_promise;	initialize_promise(&simple_promise);
	job simple_job;			initialize_job(&simple_job, my_job_function_simple, &input, &simple_promise);

	promise* simple_promise_p = &simple_promise;
	job* simple_job_p = &simple_job;

	printf("Queuing threaded async jobs to wait for the result of the main job\n");
	// submitting multiple jobs to wait for result of the simple job
	job waiting_jobs[WATING_JOB_COUNT];
	for(int i = 0; i < WATING_JOB_COUNT; i++)
	{
		initialize_job(&(waiting_jobs[i]), job_waiting, simple_promise_p, NULL);
		execute_async(&(waiting_jobs[i]));
	}

	execute_async(simple_job_p);

	#if defined WAIT_ANXIOUSLY

		unsigned long long int result_cyles_anxiously_waiting = 0;
		while(is_promised_result_ready(simple_promise_p) == 0)
			result_cyles_anxiously_waiting++;
		printf("Anxiously awaited result for %llu clocks\n", result_cyles_anxiously_waiting);

	#elif defined WAIT_USING_PROMISE_QUEUE

		sync_queue promise_completed_queue;
		initialize_sync_queue(&promise_completed_queue, 1, 1);
		set_promise_completed_queue(simple_promise_p, &promise_completed_queue);
		simple_promise_p = (promise*) pop_sync_queue_blocking(&promise_completed_queue, 0);
		deinitialize_sync_queue(&promise_completed_queue);
		printf("Awaited for promised to be completed using sync queue\n");

	#endif

	printf("Parent thread going to get result\n");
	int* result = get_promised_result(simple_promise_p);
	printf("%d parent thread prints => [%d]\n", (int)pthread_self(), (*result));

	return 0;
}