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
#define WORKER_QUEUE_TIMEOUT	1000000
#define WORKER_POLICY			WAIT_FOREVER_ON_JOB_QUEUE /*KILL_ON_TIMEDOUT*/

#define SET_1_JOBS	3
#define SET_2_JOBS	3
#define SET_3_JOBS	JOBs_COUNT - SET_1_JOBS - SET_2_JOBS

#define USE_SYNC_QUEUE_TRANSFER_TO_REFILL_JOBS

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
	for(int i = 0; i < SET_1_JOBS; i++, total_jobs_submitted+=2)
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
	thread_id = start_worker(&job_queue, WORKER_POLICY, WORKER_QUEUE_TIMEOUT);

	printf("Worker thread id : %lu\n\n", thread_id);

	printf("Main thread will sleep for 1 second\n\n");
	usleep(1 * 1000 * 1000);

	printf("Submitting the rest of the jobs\n");
	for(int i = 0; i < SET_2_JOBS; i++, total_jobs_submitted+=2)
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
	for(int i = 0; i < SET_3_JOBS; i++, total_jobs_submitted+=2)
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

	printf("Main thread will sleep for 0.5 second\n\n");
	usleep(500 * 1000);

	printf("Stopping worker\n\n");
	stop_worker(thread_id);

	printf("Printing result\n");
	for(int i = 0; i < JOBs_COUNT; i++)
	{
		printf("Output_func[%d] = %d\n", i, input_function_params[i]);
		printf("Output_jobb[%d] = %d\n", i, input_job_params[i]);
	}
	printf("\n");

	deinitialize_sync_queue(&(job_queue));

	printf("Test completed\n\n");
	return 0;
}