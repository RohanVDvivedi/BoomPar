#include<executor.h>
#include<array.h>
#include<unistd.h>

void* my_job_function(void* my_int)
{
	int i = (*((int*)my_int));
	printf("c %d => [%d]\n", (int)pthread_self(), i);
	(*((int*)my_int)) += 100;
	return my_int;
}

int main()
{
	// the number of jobs to submit
	int jobs_count = 20;

	// the number of threads for executor
	int threads_count = 4;

	// how do you want to control
	int wait_for_all_jobs_to_complete = 0;
	int wait_for_executors_threads_to_shutdown = 1;
	int shutdown_immediately = 1;

	// wait time out on empty job = 3 secs, 50 milli seconds
	unsigned long long int seconds = 3;
	unsigned long long int milliseconds = 50;
	unsigned long long int microseconds = 0;
	unsigned long long int empty_job_queue_wait_time_out_in_micro_seconds = (seconds * 1000000) + (milliseconds * 1000) + (microseconds);

	executor* executor_p = get_executor(/*FIXED_THREAD_COUNT_EXECUTOR*/ /*NEW_THREAD_PER_JOB_SUBMITTED_EXECUTOR*/ CACHED_THREAD_POOL_EXECUTOR, threads_count, empty_job_queue_wait_time_out_in_micro_seconds);

	// to store the references to all the jobs that we create
	array* my_jobs = get_array(jobs_count);

	// create jobs
	for(int i=0; i<jobs_count;i++)
	{
		void* (*funt)() = NULL;
		void* input_p = NULL;

		{
			funt = my_job_function;
			input_p = ((int*)(malloc(sizeof(int))));
			(*((int*)input_p)) = i;
		}

		// create a new job
		job* job_p = get_job(funt, input_p);

		// store it in out array
		set_element(my_jobs, job_p, i);
	}

	// submit jobs, one by one
	for(int i=0; i<jobs_count;i++)
	{
		if(submit(executor_p, (job*)get_element(my_jobs, i)))
		{
			//printf("Successfully submitted job with input %d\n", i);
		}
		else
		{
			printf("Job submission failed with input %d\n", i);
		}
	}

	//usleep(10 * 1000 * 1000);

	if(wait_for_all_jobs_to_complete)
	{
		printf("waiting for the jobs to finish\n");

		// wait for all the jobs, that we know off to finish
		for(int i=0; i<jobs_count;i++)
		{
			// get output_p of the i-th job even if we have to wait
			void* output_p = get_result((job*)get_element(my_jobs, i));

			// and print their results
			printf("p %d => [%d]\n", (int)pthread_self(), *((int*)output_p));
		}
	}

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

	printf("thread count %d\n", executor_p->thread_count);
	printf("unexecuted job count %llu\n", executor_p->job_queue->queue_size);

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
		job* job_p = (job*)get_element(my_jobs, i);
		free(job_p->output_p);
		delete_job(job_p);
	}

	delete_array(my_jobs);

	return 0;
}