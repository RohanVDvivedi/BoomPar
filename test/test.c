#include<executor.h>
#include<array.h>
#include<unistd.h>

typedef struct range range;
struct range
{
	unsigned long long int start;
	unsigned long long int end;
};

void* my_job_function(void* my_int)
{
	int i = (*((int*)my_int));
	usleep( (10 - i) * 1000 * 1000);
	printf("thread id => %d, this is a job printing => [%d], local task waiting %d secs\n", (int)pthread_self(), *((int*)my_int), 10-i);
	(*((int*)my_int)) += 100;
	return my_int;
}

int main()
{
	executor* executor_p = get_executor(FIXED_THREAD_COUNT_EXECUTOR /*CACHED_THREAD_POOL_EXECUTOR*/, 8);

	// to store the references to all the jobs that we create
	array* my_jobs = get_array(10);

	// create jobs
	for(int i=0; i<10;i++)
	{
		// create a new job
		job* job_p = get_job(my_job_function, ((int*)(malloc(sizeof(int)))) );

		// set its input
		(*((int*)(job_p->input_p))) = i;

		// store it in out array
		set_element(my_jobs, job_p, i);
	}

	// submit jobs, one by one
	for(int i=0; i<10;i++)
	{
		submit(executor_p, (job*)get_element(my_jobs, i));
	}

	// wait for all the jobs, that we know off to finish
	for(int i=0; i<10;i++)
	{
		// get output_p of the i-th job even if we have to wait
		void* output_p = get_result_or_wait_for_result((job*)get_element(my_jobs, i));

		// and print their results
		printf("thread %d waited for result, and received => [%d]\n", (int)pthread_self(), *((int*)output_p));
	}

	for(int i=0; i<10;i++)
	{
		job* job_p = (job*)get_element(my_jobs, i);
		free(job_p->output_p);
		delete_job(job_p);
	}

	delete_array(my_jobs);

	usleep(12 * 1000 * 1000);

	delete_executor(executor_p);

	return 0;
}