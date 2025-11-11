#include<boompar/tiber.h>

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#define EXECUTOR_THREADS_COUNT 	2
#define MAX_JOB_QUEUE_CAPACITY  UNBOUNDED_SYNC_QUEUE // JOB_QUEUE_AS_LINKEDLIST
#define STACK_SIZE              24*1024

tiber tb1;
tiber tb2;

void tb1_func(void* p)
{
	printf("Hello 1\n");

	yield_tiber();

	printf("Rohan 1\n");

	yield_tiber();

	printf("Dvivedi 1\n");

	yield_tiber();

	kill_tiber();
}

void tb2_func(void* p)
{
	printf("Hello 2\n");

	yield_tiber();

	printf("Rohan 2\n");

	yield_tiber();

	printf("Dvivedi 2\n");

	yield_tiber();
}

int MAX_COUNT = 10;
int curr = 0;

tiber tb3;
tiber tb4;

pthread_mutex_t lock;
tiber_cond wait;

#define WAKEUP tiber_cond_signal // tiber_cond_broadcast

void tb3_func(void* p)
{
	while(1)
	{
		pthread_mutex_lock(&lock);

		while(((curr % 2) != 1) && curr < MAX_COUNT)
			tiber_cond_wait(&wait, &lock);

		if(curr >= MAX_COUNT)
		{
			WAKEUP(&wait);
			pthread_mutex_unlock(&lock);
			break;
		}

		printf("Printing %d from %p\n", curr, tb3_func);
		curr++;

		WAKEUP(&wait);

		pthread_mutex_unlock(&lock);
	}
}

void tb4_func(void* p)
{
	while(1)
	{
		pthread_mutex_lock(&lock);

		while(((curr % 2) != 0) && curr < MAX_COUNT)
			tiber_cond_wait(&wait, &lock);

		if(curr >= MAX_COUNT)
		{
			WAKEUP(&wait);
			pthread_mutex_unlock(&lock);
			break;
		}

		printf("Printing %d from %p\n", curr, tb4_func);
		curr++;

		WAKEUP(&wait);

		pthread_mutex_unlock(&lock);
	}
}

void start_up(void* additional_params)
{
	size_t stack_size;
	pthread_attr_t attr;
	pthread_getattr_np(pthread_self(), &attr);
	pthread_attr_getstacksize(&attr, &stack_size);
	pthread_attr_destroy(&attr);

	printf("Worker thread started : with stack size of %zu\n", stack_size);
}

void clean_up(void* additional_params)
{
	printf("Worker thread completed\n");
}

int main()
{
	pthread_mutex_init(&lock, NULL);
	tiber_cond_init(&wait);

	executor* executor_p = new_executor(FIXED_THREAD_COUNT_EXECUTOR, EXECUTOR_THREADS_COUNT, MAX_JOB_QUEUE_CAPACITY, 1000, start_up, clean_up, NULL, STACK_SIZE);

	initialize_and_run_tiber(&tb1, executor_p, tb1_func, NULL, 4096);
	initialize_and_run_tiber(&tb2, executor_p, tb2_func, NULL, 4096);
	initialize_and_run_tiber(&tb3, executor_p, tb3_func, NULL, 4096);
	initialize_and_run_tiber(&tb4, executor_p, tb4_func, NULL, 4096);

	// wait for 5 seconds
	sleep(5);

	shutdown_executor(executor_p, 0);
	wait_for_all_executor_workers_to_complete(executor_p);
	delete_executor(executor_p);

	deinitialize_tiber(&tb1);
	deinitialize_tiber(&tb2);
	deinitialize_tiber(&tb3);
	deinitialize_tiber(&tb4);

	pthread_mutex_destroy(&lock);
	tiber_cond_destroy(&wait);

	return 0;
}