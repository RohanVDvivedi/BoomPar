#include<executor.h>
#include<array.h>
#include<unistd.h>

void* my_job_function(void* my_int)
{
	int i = (*((int*)my_int));
	//usleep(3 * 1000);
	printf("c %d => [%d]\n", (int)pthread_self(), i);
	(*((int*)my_int)) += 100;
	return my_int;
}

int main()
{
	printf("Main thread : %d\n", (int)pthread_self());

	// the number of jobs to submit
	int jobs_count = 120;

	// the number of threads for executor
	int threads_count = 4;

	// how do you want to control
	int wait_for_executors_threads_to_shutdown = 1;
	int shutdown_immediately = 0;

	// wait time out on empty job = 3 secs, 50 milli seconds
	unsigned long long int seconds = 3;
	unsigned long long int milliseconds = 50;
	unsigned long long int microseconds = 0;
	unsigned long long int empty_job_queue_wait_time_out_in_micro_seconds = (seconds * 1000000) + (milliseconds * 1000) + (microseconds);

	executor* executor_p = get_executor(FIXED_THREAD_COUNT_EXECUTOR /*NEW_THREAD_PER_JOB_SUBMITTED_EXECUTOR*/ /*CACHED_THREAD_POOL_EXECUTOR*/, threads_count, empty_job_queue_wait_time_out_in_micro_seconds);

	// to store the references to all the input integers that we create, for each and every job
	array* my_ints = get_array(jobs_count);

	// create jobs
	for(int i=0; i<jobs_count;i++)
	{
		int* input_p = ((int*)(malloc(sizeof(int))));

		(*input_p) = i;

		// store it in our array
		set_element(my_ints, input_p, i);
	}

	int l = 5001;
	job* test_submitted_job_p = get_job(my_job_function, &l);

	// submit jobs, one by one
	for(int i=0; i<jobs_count;i++)
	{
		if(submit_function(executor_p, my_job_function, (void*)get_element(my_ints, i)))
		{
			//printf("Successfully submitted job with input %d\n", i);
		}
		else
		{
			printf("Job submission failed with input %d\n", i);
		}

		// this is to test the functionality of submit_job function
		if(i == (jobs_count-1)/12)
		{
			submit_job(executor_p, test_submitted_job_p);
		}
	}
	printf("finished queueing all jobs\n");

	int* l_p = get_result(test_submitted_job_p);
	if(l_p != NULL)
	{
		printf("the result from test_submitted_job_p : %d, at address %p, while the address of l was %p\n", *l_p, l_p, &l);
	}
	else
	{
		printf("test_job_p could not be executed\n");
	}
	delete_job(test_submitted_job_p);

	if(wait_for_executors_threads_to_shutdown)
	{
		printf("Calling shutdown with shutdown_immediately = %d\n", shutdown_immediately);

		shutdown_executor(executor_p, shutdown_immediately);

		printf("Shutdown called\n");

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

	printf("thread count %llu\n", executor_p->thread_count);
	printf("unexecuted job count %llu\n", executor_p->job_queue.queue_size);

	if(delete_executor(executor_p))
	{
		printf("Deletion of executor succedded\n");
	}
	else
	{
		printf("Deletion of executor failed\n");
	}

	for(int i=0; i<jobs_count;i++)
	{
		int* input_p = (int*)get_element(my_ints, i);
		free(input_p);
	}

	delete_array(my_ints);
}