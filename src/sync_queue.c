#include<sync_queue.h>

#include<stdio.h>
#include<stdlib.h>

sync_queue* get_sync_queue(unsigned long long int size, int is_bounded)
{
	sync_queue* sq = (sync_queue*) malloc(sizeof(sync_queue));
	initialize_sync_queue(sq, size, is_bounded);
	return sq;
}

void initialize_sync_queue(sync_queue* sq, unsigned long long int size, int is_bounded)
{
	pthread_mutex_init(&(sq->q_lock), NULL);
	pthread_cond_init(&(sq->q_empty_wait), NULL);
	pthread_cond_init(&(sq->q_full_wait), NULL);
	sq->q_empty_wait_thread_count = 0;
	sq->q_full_wait_thread_count = 0;
	initialize_queue(&(sq->qp), ((size == 0) ? 1 : size));	// if size == 0, then size = 1
	sq->is_bounded = is_bounded;
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
		int is_full = is_full_queue(&(sq->qp));
	pthread_mutex_unlock(&(sq->q_lock));
	return is_full;
}

int is_empty_sync_queue(sync_queue* sq)
{
	pthread_mutex_lock(&(sq->q_lock));
		int is_empty = is_empty_queue(&(sq->qp));
	pthread_mutex_unlock(&(sq->q_lock));
	return is_empty;
}

int push_sync_queue_non_blocking(sync_queue* sq, const void* data_p)
{
	int is_pushed = 0;
	pthread_mutex_lock(&(sq->q_lock));

		// if an unbounded queue is full, expand it
		if(!sq->is_bounded && is_full_queue(&(sq->qp)))
			expand_queue(&(sq->qp));

		is_pushed = push_queue(&(sq->qp), data_p);

		// signal other threads if an element was pushed
		if(is_pushed)
			pthread_cond_signal(&(sq->q_empty_wait));

	pthread_mutex_unlock(&(sq->q_lock));
	return is_pushed;
}

const void* pop_sync_queue_non_blocking(sync_queue* sq)
{
	const void* data_p = NULL;
	int is_popped = 0;
	pthread_mutex_lock(&(sq->q_lock));

		data_p = get_top_queue(&(sq->qp));
		is_popped = pop_queue(&(sq->qp));

		// signal other threads, if an element was popped
		if(is_popped)
			pthread_cond_signal(&(sq->q_full_wait));

		// if an unbounded queue, occupies more than thrice the needed space, shrink it
		if(!sq->is_bounded && (get_total_size_queue(&(sq->qp)) > 3 * get_element_count_queue(&(sq->qp))))
			shrink_queue(&(sq->qp));

	pthread_mutex_unlock(&(sq->q_lock));
	return is_popped ? data_p : NULL;	// if the data is not popped, we can/must not return it
}

static int timed_conditional_waiting_in_microseconds(pthread_cond_t* cond_wait_p, pthread_mutex_t* mutex_p, unsigned long long int wait_time_out_in_microseconds)
{
	int return_val = 0;

	// if == 0, we will block indefinitely, on the condition variable
	if(wait_time_out_in_microseconds == 0)
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

int push_sync_queue_blocking(sync_queue* sq, const void* data_p, unsigned long long int wait_time_out_in_microseconds)
{
	int is_pushed = 0;
	pthread_mutex_lock(&(sq->q_lock));

		int wait_error = 0;

		// keep on looping while the bounded queue is full and there is no wait_error
		// note : timeout is also a wait error
		while(sq->is_bounded && is_full_queue(&(sq->qp)) && !wait_error)
		{
			sq->q_full_wait_thread_count++;
			wait_error = timed_conditional_waiting_in_microseconds(&(sq->q_full_wait), &(sq->q_lock), wait_time_out_in_microseconds);
			sq->q_full_wait_thread_count--;
		}

		// if an unbounded queue is full, expand it
		if(!sq->is_bounded && is_full_queue(&(sq->qp)))
			expand_queue(&(sq->qp));

		is_pushed = push_queue(&(sq->qp), data_p);

		// signal other threads if an element was pushed
		if(is_pushed)
			pthread_cond_signal(&(sq->q_empty_wait));

	pthread_mutex_unlock(&(sq->q_lock));
	return is_pushed;
}

const void* pop_sync_queue_blocking(sync_queue* sq, unsigned long long int wait_time_out_in_microseconds)
{
	const void* data_p = NULL;
	int is_popped = 0;
	pthread_mutex_lock(&(sq->q_lock));

		int wait_error = 0;

		// keep on looping while the queue is empty and there is no wait_error
		// note : timeout is also a wait error
		while(is_empty_queue(&(sq->qp)) && !wait_error)
		{
			sq->q_empty_wait_thread_count++;
			wait_error = timed_conditional_waiting_in_microseconds(&(sq->q_empty_wait), &(sq->q_lock), wait_time_out_in_microseconds);
			sq->q_empty_wait_thread_count--;
		}

		// if queue, is not empty, pop the top element
		data_p = get_top_queue(&(sq->qp));
		is_popped = pop_queue(&(sq->qp));

		// signal other threads, if an element was popped
		if(is_popped)
			pthread_cond_signal(&(sq->q_full_wait));

		// if an unbounded queue, occupies more than thrice the needed space, shrink it
		if(!sq->is_bounded && (get_total_size_queue(&(sq->qp)) > 3 * get_element_count_queue(&(sq->qp))))
			shrink_queue(&(sq->qp));

	pthread_mutex_unlock(&(sq->q_lock));
	return is_popped ? data_p : NULL;
}

unsigned int get_threads_waiting_on_empty_sync_queue(sync_queue* sq)
{
	pthread_mutex_lock(&(sq->q_lock));
		unsigned int return_val = sq->q_empty_wait_thread_count;
	pthread_mutex_unlock(&(sq->q_lock));
	return return_val;
}

unsigned int get_threads_waiting_on_full_sync_queue(sync_queue* sq)
{
	pthread_mutex_lock(&(sq->q_lock));
		unsigned int return_val = sq->q_full_wait_thread_count;
	pthread_mutex_unlock(&(sq->q_lock));
	return return_val;
}