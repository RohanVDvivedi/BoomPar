#include<executor.h>
#include<unistd.h>

void* my_job_function(void* my_int)
{
	int i = (*((int*)my_int));
	usleep(100);
	printf("c %d => [%d]\n", (int)pthread_self(), i);
	(*((int*)my_int)) += 100;
	return my_int;
}

#define EXECUTOR_TYPE			/*FIXED_THREAD_COUNT_EXECUTOR*/ CACHED_THREAD_POOL_EXECUTOR
#define EXECUTOR_THREADS_COUNT 	4

#define WAIT_FOR_SHUTDOWN       1
#define SHUTDOWN_IMMEDIATELY 	1

#define TEST_JOBs_COUNT 120

// wait time out on empty job = 3 secs, 50 milli seconds
#define SECONDS 	3
#define MILLIS 		50
#define MICROS 		0
#define THREAD_TIME_OUT_in_microseconds (SECONDS * 1000000) + (MILLIS * 1000) + (MICROS)

int main()
{
	printf("Main thread : %d\n", (int)pthread_self());

	executor* executor_p = get_executor(EXECUTOR_TYPE, EXECUTOR_THREADS_COUNT, THREAD_TIME_OUT_in_microseconds);

	// to store the references to all the input integers that we create, for each and every job
	int jobs_input_param[TEST_JOBs_COUNT];

	// create job parameters
	for(int i = 0; i < TEST_JOBs_COUNT; i++)
	{
		jobs_input_param[i] = i;
	}

	// create a submittable job
	int l = 5001;
	job test_submitted_job_p;
	initialize_job(&test_submitted_job_p, my_job_function, &l);

	// submit jobs, one by one
	for(int i=0; i < TEST_JOBs_COUNT;i++)
	{
		if(submit_function(executor_p, my_job_function, &jobs_input_param[i]))
		{
			//printf("Successfully submitted job with input %d\n", i);
		}
		else
		{
			printf("Job submission failed with input %d\n", i);
		}

		// this is to test the functionality of submit_job function
		if(i == (TEST_JOBs_COUNT-1)/12)
		{
			submit_job(executor_p, &test_submitted_job_p);
		}
	}
	printf("finished queueing all jobs\n");

	int* l_p = get_result(&test_submitted_job_p);
	if(l_p != NULL)
	{
		printf("the result from test_submitted_job_p : %d, at address %p, while the address of l was %p\n", *l_p, l_p, &l);
	}
	else
	{
		printf("test_job_p could not be executed\n");
	}
	deinitialize_job(&test_submitted_job_p);

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
			printf("Looks like wait function threw error %d\n", wait_error);
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