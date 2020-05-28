#ifndef WORKER_H
#define WORKER_H

#include<job.h>
#include<sync_queue.h>

typedef struct worker worker;
struct worker
{
	pthread_t thread_id;
	sync_queue job_queue;
};

worker* get_worker(unsigned long long int size, int is_bounded_queue, long long int job_queue_wait_timeout_in_microseconds);

void initialize_worker(worker* wrk, unsigned long long int size, int is_bounded_queue, long long int job_queue_wait_timeout_in_microseconds);

void deinitialize_worker(worker* wrk);

void delete_worker(worker* wrk);

int start_worker(worker* wrk);

int stop_worker(worker* wrk);

int submit_function_to_worker(worker* wrk, void* (*function_p)(void* input_p), void* input_p);

int submit_job_to_worker(worker* wrk, job* job_p);

#endif