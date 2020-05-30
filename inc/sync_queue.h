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




/*
*	INITIALIZATION AND DEINITIALIZATION FUNCTIONS FOR SYNC QUEUE
*/

sync_queue* get_sync_queue(unsigned long long int size, int is_bounded);

void initialize_sync_queue(sync_queue* sq, unsigned long long int size, int is_bounded);

void deinitialize_sync_queue(sync_queue* sq);

void delete_sync_queue(sync_queue* sq);




/*
*	NON BLOCKING ACCESS FUNCTIONS FOR SYNC QUEUE
*/

// returns 1, if the sync queue is full, else returns 0
int is_full_sync_queue(sync_queue* sq);

// returns 1, if the sync queue is empty, else returns 0
int is_empty_sync_queue(sync_queue* sq);

// it will not block, to wait until there is space in the sync queue to push the element
// returns 1, if the element was pushed, else returns 0
// returns 0, if the queue is bounded and it is full
int push_sync_queue_non_blocking(sync_queue* sq, const void* data_p);

// it will not block, to wait until there is any element in the sync queue to pop
// returns NULL, if the queue is empty
const void* pop_sync_queue_non_blocking(sync_queue* sq);




/*
*	BLOCKING ACCESS FUNCTIONS FOR SYNC QUEUE,
*	HERE wait_time_out_in_microseconds IS A MANDATORY PARAMETER, IF YOU PROVIDE 0,
*	YOU WOULD BLOCK INDEFINITELY ON THE CALL
*/

// returns non zero if waiting timed out, i.e. queue is still full
// returns 0, if the sync_queue was dequeued while we waited or if it was already not full
int wait_while_full_sync_queue(sync_queue* sq, unsigned long long int wait_time_out_in_microseconds);

// returns non zero if waiting timed out, i.e. queue is still empty
// returns 0, if the sync_queue was dequeued while we waited or if the sync_queue was already not empty
int wait_while_empty_sync_queue(sync_queue* sq, unsigned long long int wait_time_out_in_microseconds);

// it will block for atmost wait_time_out_in_microseconds, for someone to make space in the sync_queue, if it is bounded and full
// returns 1, if the element was pushed, else returns 0
int push_sync_queue_blocking(sync_queue* sq, const void* data_p, unsigned long long int wait_time_out_in_microseconds);

// it will block for atmost wait_time_out_in_microseconds
const void* pop_sync_queue_blocking(sync_queue* sq, unsigned long long int wait_time_out_in_microseconds);




/*
*	UTILITY FUNCTION TO TRANSFER ELEMENTS FROM src TO dst ATOMICALLY
*/

// returns the number of elements that were transferred, it will transfer no more than max_elements
unsigned long long int transfer_elements_sync_queue(sync_queue* dst, sync_queue* src, unsigned long long int max_elements);

#endif