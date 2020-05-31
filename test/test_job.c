#include<job.h>

void* my_job_function_simple(void* my_int)
{
	int i = (*((int*)my_int));
	printf("%d ided Job simple prints => [%d]\n", (int)pthread_self(), i);
	//usleep(3 * 1000 * 1000);
	(*((int*)my_int)) += 1000;
	return my_int;
}

void* job_waiting(void* job_to_wait)
{
	job* job_p = (job*) job_to_wait;
	int* result = get_result(job_p);
	printf("%d waiting thread prints => [%d]\n", (int)pthread_self(), (*result));
	return NULL;
}

#define WATING_JOB_COUNT 10

// select only one of the below test modes
//#define RESULT_ANXIOUS_WAITING
#define PRINT_WAITING_THREAD_COUNT

int main()
{
	printf("----Testing---- Job async from thread %d\n", (int)pthread_self());printf("LOL\n");

	int input = 5054;
	job* simple_job_p = get_job(my_job_function_simple, &input);

	printf("Queuing threaded async jobs to wait for the result of the main job\n");
	
	job waiting_jobs[WATING_JOB_COUNT];
	for(int i = 0; i < WATING_JOB_COUNT; i++)
	{
		initialize_job(&(waiting_jobs[i]), job_waiting, simple_job_p);
		execute_async(&(waiting_jobs[i]));
	}

	execute_async(simple_job_p);

	#ifdef RESULT_ANXIOUS_WAITING
		unsigned long long int result_cyles_anxiously_waiting = 0;
		while(check_result_ready(simple_job_p) == 0)
		{
			result_cyles_anxiously_waiting++;
		}
		printf("Anxiously awaited result for %llu clocks\n", result_cyles_anxiously_waiting);
	#elif defined PRINT_WAITING_THREAD_COUNT
		unsigned long long int result_cyles_anxiously_waiting = 0;
		unsigned long long int waiting_threads = 0;
		while(check_result_ready(simple_job_p) == 0 || get_thread_count_waiting_for_result(simple_job_p) > 0)
		{
			result_cyles_anxiously_waiting++;
			unsigned long long int new_waiting_threads = get_thread_count_waiting_for_result(simple_job_p);
			if(waiting_threads != new_waiting_threads)
			{
				printf("At %llu clocks the number of threads waiting for result changed from %llu to %llu\n", result_cyles_anxiously_waiting, waiting_threads, new_waiting_threads);
				waiting_threads = new_waiting_threads;
			}
		}

			result_cyles_anxiously_waiting++;
			unsigned long long int new_waiting_threads = get_thread_count_waiting_for_result(simple_job_p);
			if(waiting_threads != new_waiting_threads)
			{
				printf("At %llu clocks the number of threads waiting for result changed from %llu to %llu\n", result_cyles_anxiously_waiting, waiting_threads, new_waiting_threads);
				waiting_threads = new_waiting_threads;
			}

		printf("Anxiously awaited result for %llu clocks\n", result_cyles_anxiously_waiting);
	#endif
		
	printf("Deleting/Deinitializing threaded async jobs that were asked to wait for the result\n");
	for(int i = 0; i < WATING_JOB_COUNT; i++)
	{
		// wait for waiting job to finish and then delete it
		printf("Deinitializing waiting job %d\n", i);
		get_result(&(waiting_jobs[i]));
		deinitialize_job(&(waiting_jobs[i]));
	}

	printf("Parent thread going to get result\n");
	int* result = get_result(simple_job_p);
	printf("%d parent thread prints => [%d]\n", (int)pthread_self(), (*result));
	
	delete_job(simple_job_p);

	return 0;
}