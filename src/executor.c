#include<executor.h>

executor* get_executor(executor_type type, int maximum_threads)
{
	executor* executor_p = ((executor*)(malloc(sizeof(executor))));
	executor_p->type = type;
	executor_p->maximum_threads = maximum_threads;
	executor_p->job_queue = NULL;
	executor_p->idle_thread_queue = NULL;
	executor_p->running_threads = NULL;
	return executor_p;
}

void submit(executor* executor_p, job* job_p)
{
	// lock job_queue_mutex

	// push job_p to job_queue

	// unlock job_queue_mutex

	// notify any thread that is waiting to execute any job, on job_queue_wait
}

void assign_and_run(executor* executor_p, /*thread* thread_p,*/ job* job_p)
{
	// lock job_queue_mutex

	// wait while the job_queue is empty, on job_queue_wait and
	// release the mutex job_queue_mutex, while we wait

	// get top job_p, and then pop

	// unlock job_queue_mutex, if locked



	// lock idle_thread_queue_mutex

	// wait while the idle_thread_queue is empty, on idle_thread_queue_wait and
	// release the mutex job_queue_mutex, while we wait

	// get top idle_thread_p, and then pop

	// unlock job_queue_mutex, if locked



	// lock running_threads_mutex

	// store the thread object in running_threads hashmap keyed by its thread id

	// unlock running_threads_mutex



	// call the run on the thread object 
}

void completed_job(int thread_id)
{
	// lock running_threads_mutex

	// get the running_thread_p from running_threads hashmap keyed by its thread id

	// unlock running_threads_mutex




	// lock idle_thread_queue_mutex

	// push the thread in idle_thread_queue

	// notify any other thread that are waiting, in case when the idle_thread_queue is empty, on idle_thread_queue_wait

	// unlock job_queue_mutex, if locked
}

void delete_executor(executor* executor_p)
{
	if(executor_p->job_queue != NULL)
	{
		delete_queue(executor_p->job_queue);
		executor_p->job_queue = NULL;
	}

	if(executor_p->idle_thread_queue != NULL)
	{
		delete_queue(executor_p->idle_thread_queue);
		executor_p->idle_thread_queue = NULL;
	}

	if(executor_p->running_threads != NULL)
	{
		delete_hashmap(executor_p->running_threads);
		executor_p->running_threads = NULL;
	}
	
	free(executor_p);
}