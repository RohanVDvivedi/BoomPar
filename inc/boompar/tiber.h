#ifndef TIBER_H
#define TIBER_H

#include<pthread.h>
#include<ucontext.h>

#include<cutlery/linkedlist.h>

typedef enum tiber_state tiber_state;
enum tiber_state
{
	TIBER_QUEUED,
	TIBER_RUNNING,
	TIBER_WAITING,
	TIBER_KILLED,
};

typedef struct tiber tiber;
struct tiber
{
	// tiber's contexts
	pthread_spinlock_t tiber_context_lock;
	ucontext_t tiber_context;

	// ucontext of the thread that called this tiber
	// swap back to this when not able to run
	ucontext_t tiber_caller;

	// tiber's state
	pthread_spinlock_t tiber_state_lock;
	tiber_state state;

	// protected by the tiber_cond_lock
	llnode embed_node_for_tiber_cond_waiters;
};

typedef struct tiber_cond tiber_cond;
struct tiber_cond
{
	pthread_spinlock_t lock;

	// all the tibers wait on this list
	linkedlist tiber_cond_waiters;
};

#endif