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

	// queue is bounded in size or not
	int is_bounded;

	// queue for stroing the push/pop ed variables
	queue qp;
};

sync_queue* get_sync_queue(unsigned long long int size, int is_bounded);

void initialize_sync_queue(sync_queue* sq, unsigned long long int size, int is_bounded);

int is_full_sync_queue(sync_queue* sq);

void push_sync_queue_blocking(sync_queue* sq, const void* data_p);

int push_sync_queue_non_blocking(sync_queue* sq, const void* data_p);

const void* pop_sync_queue_blocking(sync_queue* sq);

const void* pop_sync_queue_non_blocking(sync_queue* sq);

int is_empty_sync_queue(sync_queue* sq);

void deinitialize_sync_queue(sync_queue* sq);

void delete_sync_queue(sync_queue* sq);

#endif