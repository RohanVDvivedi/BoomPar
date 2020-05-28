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
#define WORKER_QUEUE_TIMEOUT	3000000

int main()
{
	printf("Worker built and initialized\n\n");
	worker* wrk = get_worker(WORKER_QUEUE_SIZE, BOUNDED_WORKER_QUEUE, WORKER_QUEUE_TIMEOUT);

	int input_jobs_param[JOBs_COUNT];
	int i;
	for(i = 0; i < JOBs_COUNT; i++)
	{
		input_jobs_param[i] = i;
		if(submit_function_to_worker(wrk, simple_job_function, &(input_jobs_param[i])) == 0)
		{
			break;
		}
	}

	printf("Submitted %d jobs to worker\n", i);

	printf("Starting worker\n\n");
	start_worker(wrk);

	printf("Worker thread id : %d\n\n", (int)(wrk->thread_id));

	//usleep(3 * 1000 * 1000);

	// This is where the worker starts working
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