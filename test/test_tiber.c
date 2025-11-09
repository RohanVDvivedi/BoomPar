#include<boompar/tiber.h>

#include<stdio.h>
#include<stdlib.h>

#define EXECUTOR_THREADS_COUNT 	1
#define MAX_JOB_QUEUE_CAPACITY  1

tiber tb1;
tiber tb2;

void tb1_func(void* p)
{

}

void tb2_func(void* p)
{

}

int main()
{
	executor* executor_p = new_executor(FIXED_THREAD_COUNT_EXECUTOR, EXECUTOR_THREADS_COUNT, MAX_JOB_QUEUE_CAPACITY, 1000, NULL, NULL, NULL);

	initialize_and_run_tiber(&tb1, executor_p, tb1_func, NULL, 4096);
	initialize_and_run_tiber(&tb2, executor_p, tb2_func, NULL, 4096);

	shutdown_executor(executor_p, 0);
	wait_for_all_executor_workers_to_complete(executor_p);
	delete_executor(executor_p);

	return 0;
}