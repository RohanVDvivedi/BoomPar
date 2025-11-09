#include<boompar/tiber.h>

#include<stdio.h>
#include<stdlib.h>

#define EXECUTOR_THREADS_COUNT 	1
#define MAX_JOB_QUEUE_CAPACITY  1

int main()
{
	executor* executor_p = new_executor(FIXED_THREAD_COUNT_EXECUTOR, EXECUTOR_THREADS_COUNT, MAX_JOB_QUEUE_CAPACITY, 1000, NULL, NULL, NULL);

	return 0;
}