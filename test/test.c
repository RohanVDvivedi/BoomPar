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
	printf("thread id => %d, this is a job printing => [%d]\n", (int)pthread_self(), *((int*)my_int));
	(*((int*)my_int)) += 100;
	return my_int;
}

int main()
{
	executor* executor_p = get_executor(FIXED_THREAD_COUNT_EXECUTOR /*CACHED_THREAD_POOL_EXECUTOR*/, 8);

	array* my_jobs = get_array(10);

	for(int i=0; i<10;i++)
	{
		int* input_p = ((int*)(malloc(sizeof(int))));
		(*input_p) = i;
		job* job_p = get_job(my_job_function, input_p);
		set_element(my_jobs, job_p, i);
		submit(executor_p, job_p);
	}

	for(int i=0; i<10;i++)
	{
		job* job_p = (job*)get_element(my_jobs, i);
		void* output_p = get_result_or_wait_for_result(job_p);
		printf("thread %d waited for result, and received => [%d]\n", (int)pthread_self(), *((int*)output_p));
		free(output_p);
		delete_job(job_p);
	}

	delete_array(my_jobs);

	usleep(20 * 1000 * 1000);

	delete_executor(executor_p);

	return 0;
}