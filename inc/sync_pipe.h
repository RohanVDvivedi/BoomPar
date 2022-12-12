#ifndef SYNC_PIPE_H
#define SYNC_PIPE_H

#include<dpipe.h>

#include<pthread.h>

typedef struct sync_pipe sync_pipe;
struct sync_pipe
{
	pthread_mutex_t sync_pipe_lock;

	pthread_cond_t sync_pipe_empty;

	pthread_cond_t sync_pipe_full;

	// the capacity of pyp at any instance may not exceed max_capacity
	unsigned int max_capacity;

	dpipe pyp;
};

sync_pipe* new_sync_pipe(unsigned int max_capacity);

void initialize_sync_pipe(sync_pipe* spyp, unsigned int max_capacity);

void deinitialize_sync_pipe(sync_pipe* spyp);

void delete_sync_pipe(sync_pipe* spyp);

// returns number of bytes written from data (always < data_size)
unsigned int write_to_sync_pipe(sync_pipe* spyp, const void* data, unsigned int data_size);

// returns number of bytes read into data (always < data_size)
unsigned int read_from_sync_pipe(sync_pipe* spyp, void* data, unsigned int data_size);

// close sync_pipe
// a closed sync pipe, can be read from, but can't be written to
void close_sync_pipe(sync_pipe* spyp);

#endif