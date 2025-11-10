#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include<boompar/sync_queue.h>
#include<boompar/sync_ll_queue.h>

#include<boompar/job.h>

// pass this as max_capacity of job_queue if you want it to be a linkedlist based
#define JOB_QUEUE_AS_LINKEDLIST 0

typedef enum job_queue_type job_queue_type;
enum job_queue_type
{
	LINKEDLIST_TYPE_JOB_QUEUE = 0,
	ARRAYLIST_TYPE_JOB_QUEUE,
};

typedef struct job_queue job_queue;
struct job_queue
{
	job_queue_type type;

	union
	{
		sync_ll_queue lsq;

		sync_queue sq;
	};
};

job_queue* new_job_queue(cy_uint max_capacity);

int initialize_job_queue(job_queue* jq, cy_uint max_capacity);

void deinitialize_job_queue(job_queue* jq);

void delete_job_queue(job_queue* jq);

cy_uint get_max_capacity_job_queue(job_queue* jq);

void update_max_capacity_job_queue(job_queue* jq, cy_uint new_max_capacity);

int is_full_job_queue(job_queue* jq);

int is_empty_job_queue(job_queue* jq);

int push_job_queue(job_queue* jq, const void* data_p, uint64_t timeout_in_microseconds);

const void* pop_job_queue(job_queue* jq, uint64_t timeout_in_microseconds);

void close_job_queue(job_queue* jq);

uint64_t get_threads_waiting_on_empty_job_queue(job_queue* jq);

uint64_t get_threads_waiting_on_full_job_queue(job_queue* jq);

#endif