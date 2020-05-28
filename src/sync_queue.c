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
	int return_val = 0;

	// if < 1, blocking indefinitely
	if(wait_time_out_in_microseconds < 1)
	{
		return_val = pthread_cond_wait(cond_wait_p, mutex_p);
	}
	else
	{
		struct timespec current_time;
		
		clock_gettime(CLOCK_REALTIME, &current_time);

		unsigned long long int secs = wait_time_out_in_microseconds / 1000000;
		unsigned long long int nano_secs_extra = (wait_time_out_in_microseconds % 1000000) * 1000;

		struct timespec wait_till = {.tv_sec = (current_time.tv_sec + secs), .tv_nsec = (current_time.tv_nsec + nano_secs_extra)};
		
		// do timedwait on job_queue_empty_wait, while releasing job_queue_mutex, while we wait
		return_val = pthread_cond_timedwait(cond_wait_p, mutex_p, &wait_till);
	}

	return return_val;
}

int push_sync_queue_blocking(sync_queue* sq, const void* data_p)
{
	int is_pushed = 0;
	pthread_mutex_lock(&(sq->q_lock));

		int wait_error = 0;

		if(sq->is_bounded)
		{
			// keep on looping while the queue is full and there is no wait_error
			// note : timeout is also a wait error
			while(isQueueHolderFull(&(sq->qp)) && !wait_error)
			{
				wait_error = timed_conditional_waiting_in_microseconds(&(sq->q_full_wait), &(sq->q_lock), sq->wait_time_out_in_microseconds);
			}
		}

		// we can not push an element, if queue is bounded and it is full
		if(!(sq->is_bounded && isQueueHolderFull(&(sq->qp))))
		{
			push_queue(&(sq->qp), data_p);
			pthread_cond_broadcast(&(sq->q_empty_wait));
			is_pushed = 1;
		}

	pthread_mutex_unlock(&(sq->q_lock));
	return is_pushed;
}

int push_sync_queue_non_blocking(sync_queue* sq, const void* data_p)
{
	int is_pushed = 0;
	pthread_mutex_lock(&(sq->q_lock));

		// we can not push an element, if queue is bounded and it is full
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

		int wait_error = 0;

		// keep on looping while the queue is empty and there is no wait_error
		// note : timeout is also a wait error
		while(isQueueEmpty(&(sq->qp)) && !wait_error)
		{
			wait_error = timed_conditional_waiting_in_microseconds(&(sq->q_empty_wait), &(sq->q_lock), sq->wait_time_out_in_microseconds);
		}

		// if queue, is not empty, pop the top element
		if(!isQueueEmpty(&(sq->qp)))
		{
			data_p = get_top_queue(&(sq->qp));
			pop_queue(&(sq->qp));
			pthread_cond_broadcast(&(sq->q_full_wait));
		}

	pthread_mutex_unlock(&(sq->q_lock));
	return data_p;
}

const void* pop_sync_queue_non_blocking(sync_queue* sq)
{
	const void* data_p = NULL;
	pthread_mutex_lock(&(sq->q_lock));

		// if queue, is not empty, pop the top element
		if(!isQueueEmpty(&(sq->qp)))
		{
			data_p = get_top_queue(&(sq->qp));
			pop_queue(&(sq->qp));
			pthread_cond_broadcast(&(sq->q_full_wait));
		}

	pthread_mutex_unlock(&(sq->q_lock));
	return data_p;
}