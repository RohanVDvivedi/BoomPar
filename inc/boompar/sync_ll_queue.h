#ifndef SYNC_LL_QUEUE_H
#define SYNC_LL_QUEUE_H

#include<stdint.h>
#include<pthread.h>

#include<cutlery/singlylist.h>

#include<posixutils/pthread_cond_utils.h>

typedef struct sync_ll_queue sync_ll_queue;
struct sync_ll_queue
{
	// to access protected queue
	pthread_mutex_t q_lock;

	// conditional wait if the queue is empty
	pthread_cond_t q_empty_wait;

	// this is the number of threads waiting on the q_empty_wait
	uint64_t q_empty_wait_thread_count;

	// queue for storing the push/pop ed variables
	singlylist qp;

	// this attribute tells us if the queue is closed
	// you can only pop from a closed queue, all push calls after a close will fail
	int is_closed;
};

/*
*	INITIALIZATION AND DEINITIALIZATION FUNCTIONS FOR SYNC LL QUEUE
*/

sync_ll_queue* new_sync_ll_queue(cy_uint node_offset);

void initialize_sync_ll_queue(sync_ll_queue* sq, cy_uint node_offset);

void deinitialize_sync_ll_queue(sync_ll_queue* sq);

void delete_sync_ll_queue(sync_ll_queue* sq);

/*
*	NON BLOCKING ACCESS FUNCTIONS FOR SYNC LL QUEUE
*/

// returns 1, if the sync queue is empty, else returns 0
int is_empty_sync_ll_queue(sync_ll_queue* sq);

/*
*	NON BLOCKING PUSH
*/

// returns 1, if the element was pushed, else returns 0
int push_sync_ll_queue(sync_ll_queue* sq, const void* data_p);

/*
*	POSSIBLY BLOCKING ACCESS FUNCTIONS FOR SYNC QUEUE,
*	HERE timeout_in_microseconds COULD ALSO BE BLOCKING OR NON_BLOCKING
*/

// return NULL, only if NULL was pushed OR we timedout
const void* pop_sync_ll_queue(sync_ll_queue* sq, uint64_t timeout_in_microseconds);



/*
*	CLOSE FUNCTIONALITY
*	you can not push to a sync_queue after a close, all push calls after a close_sync_queue will fail
*	you can still pop from a closed sync ll queue
*/

void close_sync_ll_queue(sync_ll_queue* sq);



/*
*	TO CHECK THE NUMBER OF CONSUMERS AND PRODUCERS WAITING ON THE CONDITIONAL WAITS OF SYNC_QUEUE
*/
uint64_t get_threads_waiting_on_empty_sync_ll_queue(sync_ll_queue* sq);

#endif