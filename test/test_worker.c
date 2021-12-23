#include<stdio.h>
#include<worker.h>
#include<unistd.h>

void start_up(void* additional_params)
{
	printf("Worker thread started : %s\n", ((char*)additional_params));
}

void* simple_job_function(void* input)
{
	int* input_int = (int*) input;
	printf("Thread[%lu] => %d\n", pthread_self(), (*input_int));
	*input_int = (*input_int) + 100;
	return input_int;
}

void clean_up(void* additional_params)
{
	printf("Worker thread completed : %s\n", ((char*)additional_params));
}

#define JOBs_COUNT				20
#define WORKER_QUEUE_SIZE 		3
#define WORKER_QUEUE_TIMEOUT	0 //1000000

#define SET_1_JOBS	6
#define SET_2_JOBS	6
#define SET_3_JOBS	JOBs_COUNT - SET_1_JOBS - SET_2_JOBS

int total_jobs_submitted = 0;

int function_params[JOBs_COUNT];
promise function_promises[JOBs_COUNT/2];


pthread_t thread_id;

int main()
{
	printf("Worker will be tested to execute %d jobs in all, half non-promised (no output) and half promised jobs\n\n", JOBs_COUNT);

	printf("Initializing input parameters and output promises\n\n");
	for(int i = 0; i < JOBs_COUNT; i++)
	{
		function_params[i] = i;
		if(i % 2 == 0)
			initialize_promise(&(function_promises[i/2]));
	}

	printf("Initializing job queue\n\n");
	sync_queue job_queue;
	initialize_sync_queue(&job_queue, WORKER_QUEUE_SIZE, 0);

	printf("Submitting initial 6 set of the jobs\n");
	for(int i = 0; i < SET_1_JOBS; i++, total_jobs_submitted++)
	{
		if(!submit_job_worker(&job_queue, simple_job_function, &(function_params[total_jobs_submitted]), total_jobs_submitted % 2 == 0 ? &(function_promises[total_jobs_submitted/2]) : NULL))
		{
			printf("Job submit error %d\n\n", total_jobs_submitted);
		}
		printf("%d jobs in total\n", total_jobs_submitted);
	}
	printf("Submitted %d jobs to worker\n", total_jobs_submitted);

	printf("Starting worker\n\n");
	thread_id = start_worker(&job_queue, WORKER_QUEUE_TIMEOUT, start_up, clean_up, "From Rohan");

	printf("Main thread id : %lu\n\n", thread_id);

	printf("Main thread will sleep for 1 second\n\n");
	usleep(1 * 1000 * 1000);

	printf("Submitting 6 other jobs\n");
	for(int i = 0; i < SET_2_JOBS; i++, total_jobs_submitted++)
	{
		printf("%d jobs in total\n", total_jobs_submitted);
		if(!submit_job_worker(&job_queue, simple_job_function, &(function_params[total_jobs_submitted]), total_jobs_submitted % 2 == 0 ? &(function_promises[total_jobs_submitted/2]) : NULL))
		{
			printf("Job submit error %d\n\n", total_jobs_submitted);
		}
	}
	printf("Submitted %d jobs in total\n\n", total_jobs_submitted);

	printf("Main thread will sleep for 2 second\n\n");
	usleep(2 * 1000 * 1000);

	printf("Submitting the rest of the jobs\n");
	for(int i = 0; i < SET_3_JOBS; i++, total_jobs_submitted++)
	{
		printf("%d jobs in total\n", total_jobs_submitted);
		if(!submit_job_worker(&job_queue, simple_job_function, &(function_params[total_jobs_submitted]), total_jobs_submitted % 2 == 0 ? &(function_promises[total_jobs_submitted/2]) : NULL))
		{
			printf("Job submit error %d\n\n", total_jobs_submitted);
		}
	}
	printf("Submitted %d jobs in total\n\n", total_jobs_submitted);

	printf("Submitting stop worker\n\n");
	submit_stop_worker(&job_queue);

	printf("Main thread will sleep for 0.5 second\n\n");
	usleep(500 * 1000);

	printf("Discarding all unfinished jobs\n\n");
	discard_leftover_jobs(&job_queue);

	printf("Printing result\n");
	for(int i = 0; i < JOBs_COUNT; i++)
	{
		if(i % 2 == 0)
		{
			int* res = ((int*)get_promised_result(&(function_promises[i/2])));
			printf("output[%d] as promised = %d\n", i, res == NULL ? -1 : *res);
			deinitialize_promise(&(function_promises[i/2]));
		}
	}
	printf("\n");

	deinitialize_sync_queue(&job_queue);

	printf("Test completed\n\n");
	return 0;
}