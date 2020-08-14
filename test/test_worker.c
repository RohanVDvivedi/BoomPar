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
	printf("Thread[%d] => %d\n", (int) pthread_self(), (*input_int));
	*input_int = (*input_int) + 100;
	return input_int;
}

void clean_up(void* additional_params)
{
	printf("Worker thread completed : %s\n", ((char*)additional_params));
}

#define JOBs_COUNT				10
#define WORKER_QUEUE_SIZE 		10
#define WORKER_QUEUE_TIMEOUT	0//1000000

#define SET_1_JOBS	3
#define SET_2_JOBS	3
#define SET_3_JOBS	JOBs_COUNT - SET_1_JOBS - SET_2_JOBS

int total_jobs_submitted = 0;

int input_function_params[JOBs_COUNT];

int input_job_params[JOBs_COUNT];
job input_jobs[JOBs_COUNT];

pthread_t thread_id;

int main()
{
	printf("Worker will be tested to execute %d jobs in all, half functional and half promised jobs\n\n", 2 * JOBs_COUNT);

	printf("Initializing input parameters\n\n");
	for(int i = 0; i < JOBs_COUNT; i++)
	{
		input_function_params[i] = 2 * i;
		input_job_params[i] = 2 * i + 1;
		initialize_job(&(input_jobs[i]), simple_job_function, &(input_job_params[i]));
	}

	printf("Initializing job queue\n\n");
	sync_queue job_queue;
	initialize_sync_queue(&job_queue, WORKER_QUEUE_SIZE, 0);

	printf("Submitting initial set of the jobs\n");
	for(int i = 0; i < SET_1_JOBS; i++, total_jobs_submitted++)
	{
		if(!submit_function_worker(&job_queue, simple_job_function, &(input_function_params[total_jobs_submitted])))
		{
			printf("Func submit error %d\n\n", i);
		}
		if(!submit_job_worker(&job_queue, &(input_jobs[total_jobs_submitted])))
		{
			printf("Job submit error %d\n\n", i);
		}
	}
	printf("Submitted %d jobs to worker\n", total_jobs_submitted);

	printf("Starting worker\n\n");
	thread_id = start_worker(&job_queue, WORKER_QUEUE_TIMEOUT, start_up, clean_up, "From Rohan");

	printf("Worker thread id : %lu\n\n", thread_id);

	printf("Main thread will sleep for 1 second\n\n");
	usleep(1 * 1000 * 1000);

	printf("Submitting the rest of the jobs\n");
	for(int i = 0; i < SET_2_JOBS; i++, total_jobs_submitted++)
	{
		if(!submit_function_worker(&job_queue, simple_job_function, &(input_function_params[total_jobs_submitted])))
		{
			printf("Func submit error %d\n\n", i);
		}
		if(!submit_job_worker(&job_queue, &(input_jobs[total_jobs_submitted])))
		{
			printf("Job submit error %d\n\n", i);
		}
	}
	printf("Submitted %d jobs in total\n\n", total_jobs_submitted);

	printf("Main thread will sleep for 2 second\n\n");
	usleep(2 * 1000 * 1000);

	printf("Submitting the rest of the jobs\n");
	for(int i = 0; i < SET_3_JOBS; i++, total_jobs_submitted++)
	{
		if(!submit_function_worker(&job_queue, simple_job_function, &(input_function_params[total_jobs_submitted])))
		{
			printf("Func submit error %d\n\n", i);
		}
		if(!submit_job_worker(&job_queue, &(input_jobs[total_jobs_submitted])))
		{
			printf("Job submit error %d\n\n", i);
		}
	}
	printf("Submitted %d jobs in total\n\n", total_jobs_submitted);

	printf("Submitting stop worker\n\n");
	submit_stop_worker(&job_queue);

	printf("Main thread will sleep for 0.5 second\n\n");
	usleep(500 * 1000);

	printf("Printing result\n");
	for(int i = 0; i < JOBs_COUNT; i++)
	{
		printf("Output_func[%d] = %d\n", i, input_function_params[i]);
		printf("Output_jobb[%d] = %d\n", i, input_job_params[i]);
	}
	printf("\n");

	printf("Discarding all unfinished jobs\n\n");
	discard_leftover_jobs(&job_queue);

	deinitialize_sync_queue(&job_queue);

	printf("Test completed\n\n");
	return 0;
}