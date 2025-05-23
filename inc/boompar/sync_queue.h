#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

#include<stdint.h>
#include<pthread.h>

#include<cutlery/arraylist.h>

typedef struct sync_queue sync_queue;
struct sync_queue
{
	// to access protected queue
	pthread_mutex_t q_lock;

	// conditional wait if the queue is empty
	pthread_cond_t q_empty_wait;

	// conditional wait of the queue is full
	pthread_cond_t q_full_wait;

	// this is the number of threads waiting on the q_empty_wait
	uint64_t q_empty_wait_thread_count;

	// this is the number of threads waiting on the q_full_wait
	uint64_t q_full_wait_thread_count;

	// this value suggests the maximum capacity of this queue
	cy_uint max_capacity;

	// queue for storing the push/pop ed variables
	arraylist qp;

	// this attribute tells us if the queue is closed
	// you can only pop from a closed queue, all push calls after a close will fail
	int is_closed;
};



/*
*	INITIALIZATION AND DEINITIALIZATION FUNCTIONS FOR SYNC QUEUE
*/
// size must be > 0, if the provided size is equal to 0, the queue is initialized with initial size of 1

sync_queue* new_sync_queue(cy_uint max_capacity);

int initialize_sync_queue(sync_queue* sq, cy_uint max_capacity);

void deinitialize_sync_queue(sync_queue* sq);

void delete_sync_queue(sync_queue* sq);



/*
*	GET, UPDATE MAX_CAPACITY OF THE SYNC QUEUE
*/

cy_uint get_max_capacity_sync_queue(sync_queue* sq);

void update_max_capacity_sync_queue(sync_queue* sq, cy_uint new_max_capacity);



/*
*	NON BLOCKING ACCESS FUNCTIONS FOR SYNC QUEUE
*/

// returns 1, if the sync queue is full, else returns 0
int is_full_sync_queue(sync_queue* sq);

// returns 1, if the sync queue is empty, else returns 0
int is_empty_sync_queue(sync_queue* sq);


/*
*	POSSIBLY BLOCKING ACCESS FUNCTIONS FOR SYNC QUEUE,
*	HERE wait_time_out_in_microseconds COULD ALSO BE BLOCKING OR NON_BLOCKING
*/

// returns 1, if the element was pushed, else returns 0
int push_sync_queue(sync_queue* sq, const void* data_p, unsigned long long int timeout_in_microseconds);

// return NULL, only if NULL was pushed OR we timedout
const void* pop_sync_queue(sync_queue* sq, unsigned long long int timeout_in_microseconds);



/*
*	CLOSE FUNCTIONALITY
*	you can not push to a sync_queue after a close, all push calls after a close_sync_queue will fail
*	you can still pop from a closed sync queue
*/

void close_sync_queue(sync_queue* sq);



/*
*	TO CHECK THE NUMBER OF CONSUMERS AND PRODUCERS WAITING ON THE CONDITIONAL WAITS OF SYNC_QUEUE
*/
uint64_t get_threads_waiting_on_empty_sync_queue(sync_queue* sq);

uint64_t get_threads_waiting_on_full_sync_queue(sync_queue* sq);

#endif