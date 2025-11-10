#include<boompar/sync_ll_queue.h>

#include<stdlib.h>

sync_ll_queue* new_sync_ll_queue(cy_uint node_offset)
{
	sync_ll_queue* sq = malloc(sizeof(sync_ll_queue));
	if(sq == NULL)
		return NULL;
	initialize_sync_ll_queue(sq, node_offset);
	return sq;
}

void initialize_sync_ll_queue(sync_ll_queue* sq, cy_uint node_offset)
{
	pthread_mutex_init(&(sq->q_lock), NULL);
	pthread_cond_init_with_monotonic_clock(&(sq->q_empty_wait));
	sq->q_empty_wait_thread_count = 0;
	sq->is_closed = 0;
	initialize_singlylist(&(sq->qp), node_offset);
}

void deinitialize_sync_ll_queue(sync_ll_queue* sq)
{
	pthread_mutex_destroy(&(sq->q_lock));
	pthread_cond_destroy(&(sq->q_empty_wait));
}

void delete_sync_ll_queue(sync_ll_queue* sq)
{
	deinitialize_sync_ll_queue(sq);
	free(sq);
}

int is_empty_sync_ll_queue(sync_ll_queue* sq)
{
	pthread_mutex_lock(&(sq->q_lock));
		int is_empty = is_empty_singlylist(&(sq->qp));
	pthread_mutex_unlock(&(sq->q_lock));
	return is_empty;
}

int push_sync_ll_queue(sync_ll_queue* sq, const void* data_p);

const void* pop_sync_ll_queue(sync_ll_queue* sq, uint64_t timeout_in_microseconds);

void close_sync_ll_queue(sync_ll_queue* sq)
{
	pthread_mutex_lock(&(sq->q_lock));

		// only if the sync_ll_queue was not closed,
		// then we wake up waiting threads to let them know about the closing of the sync_ll_queue
		if(!sq->is_closed)
		{
			// mark the queue as closed, no push calls will succeed after this point
			sq->is_closed = 1;

			if(sq->q_empty_wait_thread_count > 0)
				pthread_cond_broadcast(&(sq->q_empty_wait));
		}

	pthread_mutex_unlock(&(sq->q_lock));
}

uint64_t get_threads_waiting_on_empty_sync_ll_queue(sync_ll_queue* sq)
{
	pthread_mutex_lock(&(sq->q_lock));
		uint64_t return_val = sq->q_empty_wait_thread_count;
	pthread_mutex_unlock(&(sq->q_lock));
	return return_val;
}