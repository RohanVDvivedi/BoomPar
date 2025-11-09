#ifndef TIBER_H
#define TIBER_H

#include<stdint.h>

#include<pthread.h>
#include<ucontext.h>

#include<cutlery/linkedlist.h>

#include<boompar/executor.h>

/*
	This is simple coroutine/fiber library built using the support of ucontext underneath
	I would humbly request you to use a UNBOUNDED_SYNC_QUEUE with a FIXED_THREAD_COUNT_EXECUTOR (thread pool size equal to number of hardware threads) with this runtime
	you can use mutex from pthread, but substitute pthread_cond_t with tiber_cond for condition variables while using this runtime
*/

typedef enum tiber_state tiber_state;
enum tiber_state
{
	TIBER_QUEUED,	// this is the initial state, with the tiber already pushed into the thread_pool
	TIBER_RUNNING,
	TIBER_WAITING,
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


// initializes tiber in queued state, and pushes it into the job queue for the thread_pool
int initialize_and_run_tiber(tiber* tb, executor* thread_pool, void (*entry_func)(void* input_p), void* input_p, uint64_t stack_size);

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

void tiber_cond_wait(tiber_cond* tc, pthread_mutex_t* m);

void tiber_cond_signal(tiber_cond* tc);

void tiber_cond_broadcast(tiber_cond* tc);

void tiber_cond_destroy(tiber_cond* tc);

#endif