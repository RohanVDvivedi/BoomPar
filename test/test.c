#include<executor.h>
#include <unistd.h>

void* my_job_function(void* my_int)
{
	printf("thread id => %d, this is a job printing => [%d]\n", (int)pthread_self(), *((int*)my_int));
	//(*((int*)my_int))++;
	//return my_int;
	free(my_int);
	return NULL;
}

int main()
{
	executor* executor_p = get_executor(FIXED_THREAD_COUNT_EXECUTOR /*CACHED_THREAD_POOL_EXECUTOR*/, 2);

	for(int i=0; i<10; i++)
	{
		int* input_p = ((int*)(malloc(sizeof(int))));
		(*input_p) = i;
		job* job_p = get_job(my_job_function, input_p);
		submit(executor_p, job_p);
	}

	//usleep(20 * 1000 * 1000);

	delete_executor(executor_p);
	return 0;
}