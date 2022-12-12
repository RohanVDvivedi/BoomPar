#include<sync_pipe.h>

#include<stdlib.h>

#include<cutlery_math.h>

sync_pipe* new_sync_pipe(unsigned int max_capacity)
{
	sync_pipe* spyp = malloc(sizeof(sync_pipe));
	initialize_sync_pipe(spyp, max_capacity);
	return spyp;
}

void initialize_sync_pipe(sync_pipe* spyp, unsigned int max_capacity)
{
	pthread_mutex_init(&(spyp->sync_pipe_lock), NULL);
	pthread_cond_init(&(spyp->sync_pipe_empty), NULL);
	pthread_cond_init(&(spyp->sync_pipe_full), NULL);
	spyp->max_capacity = max_capacity;
	initialize_dpipe(&(spyp->pyp), 0);
}

void deinitialize_sync_pipe(sync_pipe* spyp)
{
	pthread_mutex_destroy(&(spyp->sync_pipe_lock));
	pthread_cond_destroy(&(spyp->sync_pipe_empty));
	pthread_cond_destroy(&(spyp->sync_pipe_full));
	spyp->max_capacity = 0;
	deinitialize_dpipe(&(spyp->pyp));
}

void delete_sync_pipe(sync_pipe* spyp)
{
	deinitialize_sync_pipe(spyp);
	free(spyp);
}

// returns number of bytes written from data (always < data_size)
unsigned int write_to_sync_pipe(sync_pipe* spyp, const void* data, unsigned int data_size)
{
	pthread_mutex_lock(&(spyp->sync_pipe_lock));

	// wait while pyp is full and its capacity is max_capacity
	while(is_full_dpipe(&(spyp->pyp)) && get_capacity_dpipe(&(spyp->pyp)) == spyp->max_capacity)
		pthread_cond_wait(&(spyp->sync_pipe_full), &(spyp->sync_pipe_lock));

	// expand to a new higher capacity, if the capacity is lesser than max_capacity and there are more bytes to write than the space available in the pyp
	// you may expand pyp only if it is not yet closed, since after closing dpipe, all writes are suppossed to fail
	if(!is_dpipe_closed(&(spyp->pyp)) && get_capacity_dpipe(&(spyp->pyp)) < spyp->max_capacity && get_bytes_writable_in_dpipe(&(spyp->pyp)) < data_size)
	{
		unsigned int new_capacity = min(spyp->max_capacity, get_capacity_dpipe(&(spyp->pyp)) + (2 * data_size) - get_bytes_writable_in_dpipe(&(spyp->pyp)));
		resize_dpipe(&(spyp->pyp), new_capacity);
	}

	// perform write to pyp
	unsigned int bytes_written = write_to_dpipe(&(spyp->pyp), data, data_size, PARTIAL_ALLOWED);

	// broadcast to all the threads (waiting on empty pyp) that there is atleast bytes_written number of bytes still to be read
	if(bytes_written > 0)
		pthread_cond_broadcast(&(spyp->sync_pipe_empty));

	pthread_mutex_unlock(&(spyp->sync_pipe_lock));

	return bytes_written;
}

// returns number of bytes read into data (always < data_size)
unsigned int read_from_sync_pipe(sync_pipe* spyp, void* data, unsigned int data_size)
{
	pthread_mutex_lock(&(spyp->sync_pipe_lock));

	// wait while the pyp is empty
	while(is_empty_dpipe(&(spyp->pyp)))
		pthread_cond_wait(&(spyp->sync_pipe_empty), &(spyp->sync_pipe_lock));

	// perform read from the pyp
	unsigned int bytes_read = read_from_dpipe(&(spyp->pyp), data, data_size, PARTIAL_ALLOWED);

	// shrink the pyp if the capacity is 4 times or more larger than required
	if(get_capacity_dpipe(&(spyp->pyp)) >= 4 * get_bytes_readable_in_dpipe(&(spyp->pyp)))
		resize_dpipe(&(spyp->pyp), get_bytes_readable_in_dpipe(&(spyp->pyp)));

	// broadcast to all the threads (waiting on full pyp) that the pyp is now not full and has atleast bytes_read amount of space
	if(bytes_read > 0)
		pthread_cond_broadcast(&(spyp->sync_pipe_full));

	pthread_mutex_unlock(&(spyp->sync_pipe_lock));

	return bytes_read;
}

void close_sync_pipe(sync_pipe* spyp)
{
	pthread_mutex_lock(&(spyp->sync_pipe_lock));

	// if the dpipe is not already closed then close it and wake up any writers
	if(!is_dpipe_closed(&(spyp->pyp)))
	{
		close_dpipe(&(spyp->pyp));

		// wake up all threads to let them know about the close state of the pyp
		pthread_cond_broadcast(&(spyp->sync_pipe_full));
		pthread_cond_broadcast(&(spyp->sync_pipe_empty));
	}

	pthread_mutex_unlock(&(spyp->sync_pipe_lock));
}