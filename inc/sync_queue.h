#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

#include<stdio.h>
#include<stdlib.h>

#include<pthread.h>

#include<queue.h>

typedef struct sync_queue sync_queue;
struct sync_queue
{
	// to access protect queue
	pthread_mutex_t q_lock;

	// conditional wait if the queue is empty
	pthread_cond_t q_empty_wait;

	// conditional wait of the queue is full
	pthread_cond_t q_full_wait;

	// this is the timeout, for which a blocking push/pop operation will wait
	// this is only applicable for blocking push pop operations, non-blocking operations will not be affected by this value
	// set this to -1, to to set timeout to infinite, other negative values and 0 are not recommended to use, and may elicit undefined behaviour
	long long int wait_time_out_in_microseconds;

	// queue is bounded in size or not
	int is_bounded;

	// queue for stroing the push/pop ed variables
	queue qp;
};

sync_queue* get_sync_queue(unsigned long long int size, int is_bounded, long long int wait_time_out_in_microseconds);

void initialize_sync_queue(sync_queue* sq, unsigned long long int size, int is_bounded, long long int wait_time_out_in_microseconds);

int is_full_sync_queue(sync_queue* sq);

int push_sync_queue_blocking(sync_queue* sq, const void* data_p);

// it will block for atmost wait_time_out_in_microseconds
int push_sync_queue_non_blocking(sync_queue* sq, const void* data_p);

// it will block for atmost wait_time_out_in_microseconds
const void* pop_sync_queue_blocking(sync_queue* sq);

const void* pop_sync_queue_non_blocking(sync_queue* sq);

int is_empty_sync_queue(sync_queue* sq);

// returns the number of elements that were transferred, it will transfer no more than max_elements
unsigned long long int transfer_elements_sync_queue(sync_queue* dst, sync_queue* src, unsigned long long int max_elements);

void deinitialize_sync_queue(sync_queue* sq);

void delete_sync_queue(sync_queue* sq);

#endif