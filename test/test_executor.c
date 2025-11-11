#include<boompar/executor.h>

#include<boompar/promise_completion_default_callbacks.h>

#include<stdio.h>
#include<unistd.h>
#include<pthread.h>

void worker_startup(void* args)
{
	size_t stack_size;
	pthread_attr_t attr;
	pthread_getattr_np(pthread_self(), &attr);
	pthread_attr_getstacksize(&attr, &stack_size);
	pthread_attr_destroy(&attr);

	printf("Worker thread started : with stack size of %zu\n", stack_size);
}

void* my_job_function(void* my_int)
{
	int i = (*((int*)my_int));
	usleep(100);
	printf("c %d => [%d]\n", (int)pthread_self(), i);
	(*((int*)my_int)) += 100;
	return my_int;
}

void my_job_on_cancellation(void* my_int)
{
	int i = (*((int*)my_int));
	printf("job with input %d cancelled\n", i);
}

void worker_finish(void* args)
{
	printf("Worker at thread_id = %d FINISHED\n", (int)pthread_self());
}

#define EXECUTOR_TYPE			FIXED_THREAD_COUNT_EXECUTOR /*CACHED_THREAD_POOL_EXECUTOR*/
#define EXECUTOR_THREADS_COUNT 	4

#define WAIT_FOR_SHUTDOWN       1
#define SHUTDOWN_IMMEDIATELY 	0

#define TEST_JOBs_COUNT 120

#define MAX_JOB_QUEUE_CAPACITY  (TEST_JOBs_COUNT / 4)

// wait time out on empty job = 3 secs, 50 milli seconds
#define SECONDS 	3
#define MILLIS 		50
#define MICROS 		0
#define THREAD_TIME_OUT_in_microseconds (SECONDS * 1000000) + (MILLIS * 1000) + (MICROS)

int main()
{
	printf("Main thread : %d\n", (int)pthread_self());

	executor* executor_p = new_executor(EXECUTOR_TYPE, EXECUTOR_THREADS_COUNT, MAX_JOB_QUEUE_CAPACITY, THREAD_TIME_OUT_in_microseconds, worker_startup, worker_finish, NULL, 0);

	// to store the references to all the input integers that we create, for each and every job
	int jobs_input_param[TEST_JOBs_COUNT];

	// initialize job parameters
	for(int i = 0; i < TEST_JOBs_COUNT; i++)
		jobs_input_param[i] = i;

	// create a promise_completed_queue
	sync_queue* promise_completed_queue = new_sync_queue((TEST_JOBs_COUNT / 3) + 16);
	promise_completion_callback promise_completion_queue_callback = push_to_sync_queue_on_promise_completion_callback(promise_completed_queue);

	// submit jobs, one by one
	for(int i=0; i < TEST_JOBs_COUNT;i++)
	{
		promise* promised_result = NULL;
		if(i % 3 == 0)
		{
			promised_result = new_promise();
			set_promised_callback(promised_result, &promise_completion_queue_callback);
		}

		if(!submit_job_executor(executor_p, my_job_function, &jobs_input_param[i], promised_result, my_job_on_cancellation, BLOCKING))
			printf("Job submission failed with input %d\n", i);
	}
	printf("finished queueing all jobs\n");

	usleep(300);

	printf("Calling shutdown with SHUTDOWN_IMMEDIATELY = %d\n", SHUTDOWN_IMMEDIATELY);
	shutdown_executor(executor_p, SHUTDOWN_IMMEDIATELY);
	printf("Shutdown called\n");

	if(WAIT_FOR_SHUTDOWN)
	{
		printf("Going for waiting on the executor threads to finish\n");
		if(wait_for_all_executor_workers_to_complete(executor_p))
			printf("Looks like the executor threads finished\n");
		else
			printf("Looks like wait function threw error\n");
	}

	printf("is thread count 0 => %d\n", (executor_p->active_worker_count == 0));

	if(delete_executor(executor_p))
		printf("Deletion of executor succedded\n");
	else
		printf("Deletion of executor failed\n");

	for(int i = 0; i < TEST_JOBs_COUNT / 3; i++)
	{
		promise* promised_result = (promise*) pop_sync_queue(promise_completed_queue, BLOCKING);
		int* res = ((int*)get_promised_result(promised_result));
		printf("promised = %d\n", res == NULL ? -1 : *res);
		delete_promise(promised_result);
	}

	delete_sync_queue(promise_completed_queue);

	printf("printing output\n\n");
	for(int i = 0; i < TEST_JOBs_COUNT; i++)
		printf("OUTPUT[%d] = %d\n", i, jobs_input_param[i]);

	printf("Test completed\n\n");

	return 0;
}