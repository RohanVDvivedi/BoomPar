#include<stdio.h>
#include<boompar/worker.h>
#include<unistd.h>

#include<boompar/promise_completion_default_callbacks.h>

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
#define WORKER_QUEUE_SIZE 		8
#define WORKER_QUEUE_TIMEOUT	BLOCKING //1000000

#define SET_1_JOBS	6
#define SET_2_JOBS	6
#define SET_3_JOBS	JOBs_COUNT - SET_1_JOBS - SET_2_JOBS

int total_jobs_submitted = 0;

int job_params[JOBs_COUNT];

int main()
{
	printf("Worker will be tested to execute %d jobs in all, half non-promised (no output) and half promised jobs\n\n", JOBs_COUNT);

	printf("Initializing input parameters and output promises\n\n");
	for(int i = 0; i < JOBs_COUNT; i++)
		job_params[i] = i;

	printf("Initializing job queue\n\n");
	job_queue job_q;
	initialize_job_queue(&job_q, WORKER_QUEUE_SIZE);

	printf("Initializing promise_completed queue\n\n");
	sync_queue promise_completed_queue;
	initialize_sync_queue(&promise_completed_queue, JOBs_COUNT);
	promise_completion_callback promise_completion_queue_callback = push_to_sync_queue_on_promise_completion_callback(&promise_completed_queue);

	printf("Submitting initial 6 set of the jobs\n");
	for(int i = 0; i < SET_1_JOBS; i++, total_jobs_submitted++)
	{
		if(total_jobs_submitted % 2)
		{
			if(!submit_job_worker(&job_q, simple_job_function, &(job_params[total_jobs_submitted]), NULL, NULL, BLOCKING))
			{
				printf("Job submit error %d\n\n", total_jobs_submitted);
			}
		}
		else
		{
			promise* promised_result = new_promise();
			set_promised_callback(promised_result, &promise_completion_queue_callback);
			if(!submit_job_worker(&job_q, simple_job_function, &(job_params[total_jobs_submitted]), promised_result, NULL, BLOCKING))
			{
				printf("Job submit error %d\n\n", total_jobs_submitted);
			}
		}
		
		printf("%d jobs in total\n", total_jobs_submitted);
	}

	printf("Submitted %d jobs to worker\n", total_jobs_submitted);

	printf("Starting worker\n\n");
	pthread_t thread_id;
	start_worker(&thread_id, &job_q, WORKER_QUEUE_TIMEOUT, start_up, clean_up, "From Rohan");

	printf("Main thread id : %lu\n\n", thread_id);

	printf("Main thread will sleep for 1 second\n\n");
	usleep(1 * 1000 * 1000);

	printf("Submitting 6 other jobs\n");
	for(int i = 0; i < SET_2_JOBS; i++, total_jobs_submitted++)
	{
		if(total_jobs_submitted % 2)
		{
			if(!submit_job_worker(&job_q, simple_job_function, &(job_params[total_jobs_submitted]), NULL, NULL, BLOCKING))
			{
				printf("Job submit error %d\n\n", total_jobs_submitted);
			}
		}
		else
		{
			promise* promised_result = new_promise();
			set_promised_callback(promised_result, &promise_completion_queue_callback);
			if(!submit_job_worker(&job_q, simple_job_function, &(job_params[total_jobs_submitted]), promised_result, NULL, BLOCKING))
			{
				printf("Job submit error %d\n\n", total_jobs_submitted);
			}
		}
		
		printf("%d jobs in total\n", total_jobs_submitted);
	}
	printf("Submitted %d jobs in total\n\n", total_jobs_submitted);

	printf("Main thread will sleep for 2 second\n\n");
	usleep(2 * 1000 * 1000);

	printf("Submitting the rest of the jobs\n");
	for(int i = 0; i < SET_3_JOBS; i++, total_jobs_submitted++)
	{
		if(total_jobs_submitted % 2)
		{
			if(!submit_job_worker(&job_q, simple_job_function, &(job_params[total_jobs_submitted]), NULL, NULL, BLOCKING))
			{
				printf("Job submit error %d\n\n", total_jobs_submitted);
			}
		}
		else
		{
			promise* promised_result = new_promise();
			set_promised_callback(promised_result, &promise_completion_queue_callback);
			if(!submit_job_worker(&job_q, simple_job_function, &(job_params[total_jobs_submitted]), promised_result, NULL, BLOCKING))
			{
				printf("Job submit error %d\n\n", total_jobs_submitted);
			}
		}
		
		printf("%d jobs in total\n", total_jobs_submitted);
	}
	printf("Submitted %d jobs in total\n\n", total_jobs_submitted);

	printf("Submitting stop worker\n\n");
	close_job_queue(&job_q);

	printf("Main thread will sleep for 0.5 second\n\n");
	usleep(500 * 1000);

	printf("Discarding all unfinished jobs\n\n");
	discard_leftover_jobs(&job_q);

	printf("Printing result\n");
	for(int i = 0; i < JOBs_COUNT / 2; i++)
	{
		promise* promised_result = (promise*) pop_sync_queue(&promise_completed_queue, BLOCKING);
		int* res = ((int*)get_promised_result(promised_result));
		printf("promised = %d\n", res == NULL ? -1 : *res);
		delete_promise(promised_result);
	}
	printf("\n");

	deinitialize_job_queue(&job_q);
	deinitialize_sync_queue(&promise_completed_queue);

	printf("Test completed\n\n");
	return 0;
}