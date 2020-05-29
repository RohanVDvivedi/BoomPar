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
#define WORKER_POLICY			/*WAIT_ON_TIMEDOUT*/ /*KILL_ON_TIMEDOUT*/ USE_CALLBACK

#define USE_SYNC_QUEUE_TRANSFER_TO_REFILL_JOBS

int i = 0;
int input_jobs_param[JOBs_COUNT];

#if defined USE_SYNC_QUEUE_TRANSFER_TO_REFILL_JOBS
	job input_jobs[JOBs_COUNT];
	sync_queue input_jobs_queue;
#endif

void job_queue_empty_timedout_callback(worker* wrk, const void* additional_params)
{
	#if defined USE_SYNC_QUEUE_TRANSFER_TO_REFILL_JOBS
		printf("Using sync queue tranfer for submitting jobs, current jobs now %llu\n\n", input_jobs_queue.qp.queue_size);
		transfer_elements_sync_queue(&(wrk->job_queue), &input_jobs_queue, JOBs_COUNT);
	#else
		printf("Using input functions in callback function for submitting jobs\n\n");
		for(; i < JOBs_COUNT; i++)
		{
			if(submit_function_to_worker(wrk, simple_job_function, &(input_jobs_param[i])) == 0)
			{
				break;
			}
		}
		printf("Total jobs submitted %d\n\n", i);
	#endif
}

int main()
{
	worker* wrk = get_worker(WORKER_QUEUE_SIZE, BOUNDED_WORKER_QUEUE, WORKER_QUEUE_TIMEOUT, WORKER_POLICY, job_queue_empty_timedout_callback, NULL);
	printf("Worker built and initialized %d\n\n", WORKER_POLICY);

	printf("Worker will be tested to execute %d jobs in all\n\n", JOBs_COUNT);

	printf("Initializing input job parameters\n\n");
	for(int j = 0; j < JOBs_COUNT; j++)
	{
		input_jobs_param[j] = j;
	}

	printf("Input job parameters initialized\n\n");

	if(WORKER_POLICY != USE_CALLBACK)
	{
		printf("Submitting initial set of the jobs\n");
		for(i = 0; i < JOBs_COUNT/2; i++)
		{
			if(submit_function_to_worker(wrk, simple_job_function, &(input_jobs_param[i])) == 0)
			{
				break;
			}
		}
		printf("Submitted %d jobs to worker\n", i);
	}
	else
	{
		#if defined USE_SYNC_QUEUE_TRANSFER_TO_REFILL_JOBS
			printf("Main thread will not be submitting any worker jobs, but it will queue them in external input_jobs_queue\n\n");
			initialize_sync_queue(&(input_jobs_queue), JOBs_COUNT/2, 0, 3000000);
			for(i = 0; i < JOBs_COUNT; i++)
			{
				initialize_job(&(input_jobs[i]), simple_job_function, &(input_jobs_param[i]));
				push_sync_queue_non_blocking(&(input_jobs_queue), &(input_jobs[i]));
			}
			printf("The input job queue contains %llu elements\n\n", input_jobs_queue.qp.queue_size);
		#else 
			printf("Main thread will not be submitting any worker jobs\n\n");
		#endif
	}

	printf("Starting worker\n\n");
	start_worker(wrk);

	printf("Worker thread id : %d\n\n", (int)(wrk->thread_id));

	if(WORKER_POLICY != USE_CALLBACK)
	{
		printf("Main thread will sleep for 1 second\n\n");
		usleep(1 * 1000 * 1000);

		printf("Submitting the rest of the jobs\n");
		for(; i < JOBs_COUNT; i++)
		{
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