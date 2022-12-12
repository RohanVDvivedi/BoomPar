#include<sync_pipe.h>

#include<stdlib.h>

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
unsigned int write_to_sync_pipe(sync_pipe* spyp, const void* data, unsigned int data_size);

// returns number of bytes read into data (always < data_size)
unsigned int read_from_sync_pipe(sync_pipe* spyp, void* data, unsigned int data_size);