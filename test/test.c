#include<executor.h>

void* my_job_function(void* my_int)
{
	printf("here, this is a job printing => %d\n", *((int*)my_int));
	(*((int*)my_int))++;
	return my_int;
}

int main()
{
	executor* executor_p = get_executor(FIXED_THREAD_COUNT_EXECUTOR /*CACHED_THREAD_POOL_EXECUTOR*/, 10);

	delete_executor(executor_p);
	return 0;
}