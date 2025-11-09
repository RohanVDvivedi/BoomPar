#ifndef TIBER_H
#define TIBER_H

#include<stdint.h>

#include<pthread.h>
#include<ucontext.h>

#include<cutlery/linkedlist.h>

#include<boompar/executor.h>

typedef enum tiber_state tiber_state;
enum tiber_state
{
	TIBER_QUEUED,
	TIBER_RUNNING,
	TIBER_WAITING,	// this is the initial state
	TIBER_KILLED,
};

typedef struct tiber tiber;
struct tiber
{
	// this is the thread_pool it will run on
	executor* thread_pool;

	// free this stack memory on TIBER_KILLED state
	void* stack;

	// tiber's contexts
	pthread_spinlock_t tiber_context_lock;
	ucontext_t tiber_context;

	// ucontext of the thread that called this tiber
	// swap back to this when not able to run
	// this is the parent thread that called this tiber, so it is also protected by the tiber_context_lock
	ucontext_t tiber_caller;

	// tiber's state
	pthread_spinlock_t tiber_state_lock;
	tiber_state state;

	// protected by the tiber_cond_lock
	llnode embed_node_for_tiber_cond_waiters;
};


// initializes tiber in queued state
int initialize_tiber(tiber* tb, executor* thread_pool, void (*entry_func)(void* input_p), void* input_p, uint64_t stack_size);

/*
	puts tiber as thread local variale of this thread
	puts tiber in running state
	takes it's context lock
	swaps context running the tiber
	releases it's context lock
	returns

	** only this function takes the context_lock of the tiber
*/
void enqueue_tiber(tiber* tb);

// must be called from within the tiber
void yield_tiber();

// must be called from within the tiber
void kill_tiber();

void deinitialize_tiber(tiber* tb);

typedef struct tiber_cond tiber_cond;
struct tiber_cond
{
	pthread_spinlock_t lock;

	// all the tibers wait on this list
	linkedlist tiber_cond_waiters;
};

// has api very similar to the pthread_cond_t

void tiber_cond_init(tiber_cond* tc);

void tiber_cond_wait(tiber_cond* tc, pthread_mutex_t m);

void tiber_cond_signal(tiber_cond* tc);

void tiber_cond_broadcast(tiber_cond* tc);

void tiber_cond_destroy(tiber_cond* tc);

#endif