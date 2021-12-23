#include<stdio.h>
#include<executor.h>
#include<unistd.h>

void worker_startup(void* args)
{
	printf("Worker at thread_id = %d STARTED\n", (int)pthread_self());
}

void* my_job_function(void* my_int)
{
	int i = (*((int*)my_int));
	usleep(100);
	printf("c %d => [%d]\n", (int)pthread_self(), i);
	(*((int*)my_int)) += 100;
	return my_int;
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

// wait time out on empty job = 3 secs, 50 milli seconds
#define SECONDS 	3
#define MILLIS 		50
#define MICROS 		0
#define THREAD_TIME_OUT_in_microseconds (SECONDS * 1000000) + (MILLIS * 1000) + (MICROS)

int main()
{
	printf("Main thread : %d\n", (int)pthread_self());

	executor* executor_p = get_executor(EXECUTOR_TYPE, EXECUTOR_THREADS_COUNT, THREAD_TIME_OUT_in_microseconds, worker_startup, worker_finish, NULL);

	// to store the references to all the input integers that we create, for each and every job
	int jobs_input_param[TEST_JOBs_COUNT];

	// create job parameters
	for(int i = 0; i < TEST_JOBs_COUNT; i++)
	{
		jobs_input_param[i] = i;
	}

	// create a promise to submit a resolvable job
	int l = 5001;
	promise test_promised;
	initialize_promise(&test_promised);

	// submit jobs, one by one
	for(int i=0; i < TEST_JOBs_COUNT;i++)
	{
		if(submit_job(executor_p, my_job_function, &jobs_input_param[i], NULL))
		{
			//printf("Successfully submitted job with input %d\n", i);
		}
		else
		{
			printf("Job submission failed with input %d\n", i);
		}

		// this is to test the functionality of a job than can return via a promise
		if(i == (TEST_JOBs_COUNT-1)/12)
		{
			submit_job(executor_p, my_job_function, &l, &test_promised);
		}
	}
	printf("finished queueing all jobs\n");

	int* l_p = get_promised_result(&test_promised);
	if(l_p != NULL)
	{
		printf("the result from &test_promised : %d, at address %p, while the address of l was %p\n", *l_p, l_p, &l);
	}
	else
	{
		printf("test_job_p could not be executed\n");
	}
	deinitialize_promise(&test_promised);

	printf("Calling shutdown with SHUTDOWN_IMMEDIATELY = %d\n", SHUTDOWN_IMMEDIATELY);
	shutdown_executor(executor_p, SHUTDOWN_IMMEDIATELY);
	printf("Shutdown called\n");

	if(WAIT_FOR_SHUTDOWN)
	{
		printf("Going for waiting on the executor threads to finish\n");
		int wait_error = wait_for_all_threads_to_complete(executor_p);
		if(wait_error)
		{
			printf("Looks like the executor threads finished\n");
		}
		else
		{
			printf("Looks like wait function threw error\n");
		}
	}

	printf("thread count %u\n", executor_p->active_worker_count);

	if(delete_executor(executor_p))
	{
		printf("Deletion of executor succedded\n");
	}
	else
	{
		printf("Deletion of executor failed\n");
	}

	printf("printing output\n\n");
	for(int i = 0; i < TEST_JOBs_COUNT; i++)
	{
		printf("OUTPUT[%d] = %d\n", i, jobs_input_param[i]);
	}

	printf("Test completed\n\n");

	return 0;
}