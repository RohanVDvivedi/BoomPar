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

int main()
{
	printf("Worker built and initialized\n\n");
	worker* wrk = get_worker(WORKER_QUEUE_SIZE, BOUNDED_WORKER_QUEUE);

	int input_jobs_param[JOBs_COUNT];
	for(int i = 0; i < JOBs_COUNT; i++)
	{
		input_jobs_param[i] = i;
		if(submit_function_to_worker(wrk, simple_job_function, &(input_jobs_param[i])) == 0)
		{
			printf("Submitted %d jobs to worker\n", i);
			break;
		}
	}

	printf("Starting worker\n\n");
	start_worker(wrk);

	usleep(3 * 1000 * 1000);

	// This is where the worker starts working

	printf("Stopping worker\n\n");
	stop_worker(wrk);

	printf("Deleting worker\n\n");
	delete_worker(wrk);

	printf("Test completed\n\n");
	return 0;
}