#include<worker.h>
#include<unistd.h>

void* simple_job_function(void* input)
{
	int* input_int = (int*) input;
	printf("Thread[%d] => %d\n", (int) pthread_self(), (*input_int));
	*input_int = (*input_int) + 100;
	return input_int;
}

#define JOBs_COUNT				10
#define WORKER_QUEUE_SIZE 		10
#define BOUNDED_WORKER_QUEUE 	0
#define WORKER_QUEUE_TIMEOUT	500000//3000000 // 500000
#define WORKER_POLICY			/*WAIT_ON_TIMEDOUT*/ /*KILL_ON_TIMEDOUT*/ USE_CALLBACK_AFTER_TIMEDOUT

int i = 0;
int input_jobs_param[JOBs_COUNT];

void job_queue_empty_timedout_callback(worker* wrk, const void* additional_params)
{
	for(; i < JOBs_COUNT; i++)
	{
		input_jobs_param[i] = i;
		if(submit_function_to_worker(wrk, simple_job_function, &(input_jobs_param[i])) == 0)
		{
			break;
		}
	}
}

int main()
{
	worker* wrk = get_worker(WORKER_QUEUE_SIZE, BOUNDED_WORKER_QUEUE, WORKER_QUEUE_TIMEOUT, WORKER_POLICY, job_queue_empty_timedout_callback, NULL);
	printf("Worker built and initialized %d\n\n", WORKER_POLICY);

	printf("Worker will be tested to execute %d jobs in all\n\n", JOBs_COUNT);

	if(WORKER_POLICY != USE_CALLBACK_AFTER_TIMEDOUT)
	{
		printf("Submitting initial set of the jobs\n");
		for(i = 0; i < JOBs_COUNT/2; i++)
		{
			input_jobs_param[i] = i;
			if(submit_function_to_worker(wrk, simple_job_function, &(input_jobs_param[i])) == 0)
			{
				break;
			}
		}
		printf("Submitted %d jobs to worker\n", i);
	}
	else
	{
		printf("Main thread will not be submitting any worker jobs\n\n");
	}

	printf("Starting worker\n\n");
	start_worker(wrk);

	printf("Worker thread id : %d\n\n", (int)(wrk->thread_id));

	printf("Main thread will sleep for 1 second\n\n");
	usleep(1 * 1000 * 1000);

	if(WORKER_POLICY != USE_CALLBACK_AFTER_TIMEDOUT)
	{
		printf("Submitting the rest of the jobs\n");
		for(; i < JOBs_COUNT; i++)
		{
			input_jobs_param[i] = i;
			if(submit_function_to_worker(wrk, simple_job_function, &(input_jobs_param[i])) == 0)
			{
				break;
			}
		}
		printf("Submitted %d jobs in total\n\n", i);
	}
	else
	{
		printf("Main thread will not be submitting any worker jobs\n\n");
	}

	// This is where the worker starts working
	printf("Main thread waiting to join worker thread\n\n");
	pthread_join(wrk->thread_id, NULL);

	printf("\n");

	printf("Stopping worker\n\n");
	stop_worker(wrk);

	printf("Printing result\n");
	for(int i = 0; i < JOBs_COUNT; i++)
	{
		printf("Output[%d] = %d\n", i, input_jobs_param[i]);
	}
	printf("\n");

	printf("Deleting worker\n\n");
	delete_worker(wrk);

	printf("Test completed\n\n");
	return 0;
}