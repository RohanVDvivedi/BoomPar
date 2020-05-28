#include<sync_queue.h>

sync_queue* get_sync_queue(unsigned long long int size, int is_bounded, long long int wait_time_out_in_microseconds)
{
	sync_queue* sq = (sync_queue*) malloc(sizeof(sync_queue));
	initialize_sync_queue(sq, size, is_bounded, wait_time_out_in_microseconds);
	return sq;
}

void initialize_sync_queue(sync_queue* sq, unsigned long long int size, int is_bounded, long long int wait_time_out_in_microseconds)
{
	pthread_mutex_init(&(sq->q_lock), NULL);
	pthread_cond_init(&(sq->q_empty_wait), NULL);
	pthread_cond_init(&(sq->q_full_wait), NULL);
	initialize_queue(&(sq->qp), size);
	sq->is_bounded = is_bounded;
	sq->wait_time_out_in_microseconds = wait_time_out_in_microseconds;
}

void deinitialize_sync_queue(sync_queue* sq)
{
	pthread_mutex_destroy(&(sq->q_lock));
	pthread_cond_destroy(&(sq->q_empty_wait));
	pthread_cond_destroy(&(sq->q_full_wait));
	deinitialize_queue(&(sq->qp));
}

void delete_sync_queue(sync_queue* sq)
{
	deinitialize_sync_queue(sq);
	free(sq);
}

int is_full_sync_queue(sync_queue* sq)
{
	pthread_mutex_lock(&(sq->q_lock));
		int is_full = isQueueHolderFull(&(sq->qp));
	pthread_mutex_unlock(&(sq->q_lock));
	return is_full;
}

int is_empty_sync_queue(sync_queue* sq)
{
	pthread_mutex_lock(&(sq->q_lock));
		int is_empty = isQueueEmpty(&(sq->qp));
	pthread_mutex_unlock(&(sq->q_lock));
	return is_empty;
}

static int timed_conditional_waiting_in_microseconds(pthread_cond_t* cond_wait_p, pthread_mutex_t* mutex_p, long long int wait_time_out_in_microseconds)
{
	return pthread_cond_wait(cond_wait_p, mutex_p);
}

void push_sync_queue_blocking(sync_queue* sq, const void* data_p)
{
	pthread_mutex_lock(&(sq->q_lock));

		if(sq->is_bounded)
		{
			while(isQueueHolderFull(&(sq->qp)))
			{
				timed_conditional_waiting_in_microseconds(&(sq->q_full_wait), &(sq->q_lock), sq->wait_time_out_in_microseconds);
			}
		}

		push_queue(&(sq->qp), data_p);
		pthread_cond_broadcast(&(sq->q_empty_wait));

	pthread_mutex_unlock(&(sq->q_lock));
}

int push_sync_queue_non_blocking(sync_queue* sq, const void* data_p)
{
	int is_pushed = 0;
	pthread_mutex_lock(&(sq->q_lock));

		if(!(sq->is_bounded && isQueueHolderFull(&(sq->qp))))
		{
			push_queue(&(sq->qp), data_p);
			pthread_cond_broadcast(&(sq->q_empty_wait));
			is_pushed = 1;
		}

	pthread_mutex_unlock(&(sq->q_lock));
	return is_pushed;
}

const void* pop_sync_queue_blocking(sync_queue* sq)
{
	const void* data_p = NULL;
	pthread_mutex_lock(&(sq->q_lock));

		while(isQueueEmpty(&(sq->qp)))
		{
			timed_conditional_waiting_in_microseconds(&(sq->q_empty_wait), &(sq->q_lock), sq->wait_time_out_in_microseconds);
		}

		data_p = get_top_queue(&(sq->qp));
		pop_queue(&(sq->qp));
		pthread_cond_broadcast(&(sq->q_full_wait));

	pthread_mutex_unlock(&(sq->q_lock));
	return data_p;
}

const void* pop_sync_queue_non_blocking(sync_queue* sq)
{
	const void* data_p = NULL;
	pthread_mutex_lock(&(sq->q_lock));

		if(!isQueueEmpty(&(sq->qp)))
		{
			data_p = get_top_queue(&(sq->qp));
			pop_queue(&(sq->qp));
			pthread_cond_broadcast(&(sq->q_full_wait));
		}

	pthread_mutex_unlock(&(sq->q_lock));
	return data_p;
}