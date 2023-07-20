#include<sync_queue.h>

#include<stdio.h>
#include<stdlib.h>

sync_queue* new_sync_queue(cy_uint max_capacity)
{
	sync_queue* sq = malloc(sizeof(sync_queue));
	if(sq != NULL)
		initialize_sync_queue(sq, max_capacity);
	return sq;
}

void initialize_sync_queue(sync_queue* sq, cy_uint max_capacity)
{
	pthread_mutex_init(&(sq->q_lock), NULL);
	pthread_cond_init(&(sq->q_empty_wait), NULL);
	pthread_cond_init(&(sq->q_full_wait), NULL);
	sq->q_empty_wait_thread_count = 0;
	sq->q_full_wait_thread_count = 0;
	sq->max_capacity = max_capacity;
	initialize_queue(&(sq->qp), 0);
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

cy_uint get_max_capacity_sync_queue(sync_queue* sq)
{
	pthread_mutex_lock(&(sq->q_lock));
		unsigned int max_capacity = sq->max_capacity;
	pthread_mutex_unlock(&(sq->q_lock));
	return max_capacity;
}

void update_max_capacity_sync_queue(sync_queue* sq, cy_uint new_max_capacity)
{
	pthread_mutex_lock(&(sq->q_lock));

		// if the queue is full with threads waiting for a full sync_queue, and the max_capacity is being increased, then wake up all the threads wait on full queue
		if(is_full_queue(&(sq->qp)) && sq->q_full_wait_thread_count > 0 && sq->max_capacity < new_max_capacity)
			pthread_cond_broadcast(&(sq->q_full_wait));

		sq->max_capacity = new_max_capacity;
	pthread_mutex_unlock(&(sq->q_lock));
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
	pthread_mutex_lock(&(sq->q_lock));

		// if a queue is full and it hasn't yet reached its max_capacity, then attempt to expand it
		if(is_full_queue(&(sq->qp)) && get_capacity_queue(&(sq->qp)) < sq->max_capacity)
			expand_queue(&(sq->qp));

		int is_pushed = push_to_queue(&(sq->qp), data_p);

		// signal other threads if an element was pushed
		if(is_pushed && sq->q_empty_wait_thread_count > 0)
			pthread_cond_signal(&(sq->q_empty_wait));

	pthread_mutex_unlock(&(sq->q_lock));
	return is_pushed;
}

const void* pop_sync_queue_non_blocking(sync_queue* sq)
{
	pthread_mutex_lock(&(sq->q_lock));

		const void* data_p = get_top_of_queue(&(sq->qp));
		int is_popped = pop_from_queue(&(sq->qp));

		// signal other threads, if an element was popped
		if(is_popped && sq->q_full_wait_thread_count > 0)
			pthread_cond_signal(&(sq->q_full_wait));

		// if the queue, occupies more than thrice the needed space, attempt to shrink it
		if(get_capacity_queue(&(sq->qp)) > 3 * get_element_count_queue(&(sq->qp)))
			shrink_queue(&(sq->qp));

	pthread_mutex_unlock(&(sq->q_lock));
	return is_popped ? data_p : NULL;	// if the data is not popped, we can/must not return it
}

static int timed_conditional_waiting_in_microseconds(pthread_cond_t* cond_wait_p, pthread_mutex_t* mutex_p, unsigned long long int wait_time_out_in_microseconds)
{
	int return_val = 0;

	// if timeout == 0, we will block indefinitely, on the condition variable
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
	pthread_mutex_lock(&(sq->q_lock));

		int wait_error = 0;

		// keep on looping while the bounded queue is full and has reached its max_capacity and there is no wait_error
		// note : timeout is also a wait error
		while(is_full_queue(&(sq->qp)) && get_capacity_queue(&(sq->qp)) >= sq->max_capacity && !wait_error)
		{
			sq->q_full_wait_thread_count++;
			wait_error = timed_conditional_waiting_in_microseconds(&(sq->q_full_wait), &(sq->q_lock), wait_time_out_in_microseconds);
			sq->q_full_wait_thread_count--;
		}

		// if a queue is full and it hasn't yet reached its max_capacity, then attempt to expand it
		if(is_full_queue(&(sq->qp)) && get_capacity_queue(&(sq->qp)) < sq->max_capacity)
			expand_queue(&(sq->qp));

		int is_pushed = push_to_queue(&(sq->qp), data_p);

		// signal other threads if an element was pushed
		if(is_pushed && sq->q_empty_wait_thread_count > 0)
			pthread_cond_signal(&(sq->q_empty_wait));

	pthread_mutex_unlock(&(sq->q_lock));
	return is_pushed;
}

const void* pop_sync_queue_blocking(sync_queue* sq, unsigned long long int wait_time_out_in_microseconds)
{
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
		const void* data_p = get_top_of_queue(&(sq->qp));
		int is_popped = pop_from_queue(&(sq->qp));

		// signal other threads, if an element was popped
		if(is_popped && sq->q_full_wait_thread_count > 0)
			pthread_cond_signal(&(sq->q_full_wait));

		// if the queue, occupies more than thrice the needed space, then attempt to shrink it
		if(get_capacity_queue(&(sq->qp)) > 3 * get_element_count_queue(&(sq->qp)))
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