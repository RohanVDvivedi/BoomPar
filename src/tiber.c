#include<boompar/tiber.h>

#include<stdio.h>
#include<stdlib.h>

// the current tiber that this thread is executing get's stored here
__thread tiber* curr_tiber = NULL;

int initialize_tiber(tiber* tb, executor* thread_pool, void (*entry_func)(void* input_p), void* input_p, uint64_t stack_size)
{
	tb->thread_pool = thread_pool;

	tb->stack = malloc(stack_size);
	if(tb->stack == NULL)
		return 0;

	pthread_spin_init(&(tb->tiber_context_lock), PTHREAD_PROCESS_PRIVATE);

	getcontext(&(tb->tiber_context));
	tb->tiber_context.uc_stack.ss_sp = tb->stack;
	tb->tiber_context.uc_stack.ss_size = stack_size;
	tb->tiber_context.uc_stack.ss_flags = 0;
	tb->tiber_context.uc_link = &(tb->tiber_caller);\
	makecontext(&(tb->tiber_context), (void(*)(void))entry_func, 1, input_p);

	pthread_spin_init(&(tb->tiber_state_lock), PTHREAD_PROCESS_PRIVATE);
	tb->state = TIBER_WAITING;

	initialize_llnode(&(tb->embed_node_for_tiber_cond_waiters));

	return 1;
}

/*
	puts tiber as thread local variable of this thread
	puts tiber in running state
	takes it's context lock
	swaps context running the tiber
	releases it's context lock
	returns

	** only this function takes the context_lock of the tiber
*/
void* tiber_execute_wrapper(void* tb_v)
{
	// set the thred local
	curr_tiber = (tiber*)tb_v;

	// change curr_tiber's state to running
	pthread_spin_lock(&(curr_tiber->tiber_state_lock));
	if(curr_tiber->state == TIBER_QUEUED)
		curr_tiber->state = TIBER_RUNNING;
	else
	{
		printf("TIBER BUG: tiber was queued to thread pool but is not in queued state\n");
		exit(-1);
	}
	pthread_spin_unlock(&(curr_tiber->tiber_state_lock));

	// run curr_tiber, we only use this lock here and nowhere else
	pthread_spin_lock(&(curr_tiber->tiber_context_lock));
	if(-1 == getcontext(&(curr_tiber->tiber_caller)))
	{
		printf("TIBER BUG: tiber_caller could not be populated\n");
		exit(-1);
	}
	if(-1 == swapcontext(&(curr_tiber->tiber_caller), &(curr_tiber->tiber_context)))
	{
		printf("TIBER BUG: tiber could not context switch into itself\n");
		exit(-1);
	}
	pthread_spin_unlock(&(curr_tiber->tiber_context_lock));

	// reset the thread local
	curr_tiber = NULL;

	return NULL;
}

void enqueue_tiber(tiber* tb)
{
	// change tb's state to running
	pthread_spin_lock(&(tb->tiber_state_lock));
	if(tb->state == TIBER_WAITING)
		tb->state = TIBER_QUEUED;
	else
	{
		printf("TIBER BUG: tiber not in waiting state was requested to be enqueued\n");
		exit(-1);
	}
	pthread_spin_unlock(&(tb->tiber_state_lock));

	// do the actual queueing, pushing it into the thread pool
	submit_job_executor(tb->thread_pool, tiber_execute_wrapper, tb, NULL, NULL, BLOCKING);
}

void yield_tiber()
{
	// change curr_tiber's state to QUEUED
	pthread_spin_lock(&(curr_tiber->tiber_state_lock));
	if(curr_tiber->state == TIBER_RUNNING)
		curr_tiber->state = TIBER_QUEUED;
	else
	{
		printf("TIBER BUG: tiber attempted to yield but not in running state\n");
		exit(-1);
	}
	pthread_spin_unlock(&(curr_tiber->tiber_state_lock));

	// do the actual queueing, pushing it into the thread pool
	submit_job_executor(curr_tiber->thread_pool, tiber_execute_wrapper, curr_tiber, NULL, NULL, BLOCKING);

	// swap the context out
	if(-1 == swapcontext(&(curr_tiber->tiber_context), &(curr_tiber->tiber_caller)))
	{
		printf("TIBER BUG: tiber could not context switch into itself\n");
		exit(-1);
	}
}

void kill_tiber()
{
	// change curr_tiber's state to KILLED
	pthread_spin_lock(&(curr_tiber->tiber_state_lock));
	if(curr_tiber->state == TIBER_RUNNING)
		curr_tiber->state = TIBER_KILLED;
	else
	{
		printf("TIBER BUG: tiber attempted suicide but not in running state\n");
		exit(-1);
	}
	pthread_spin_unlock(&(curr_tiber->tiber_state_lock));

	// swap the context out
	if(-1 == swapcontext(&(curr_tiber->tiber_context), &(curr_tiber->tiber_caller)))
	{
		printf("TIBER BUG: tiber could not context switch into itself\n");
		exit(-1);
	}
}

void deinitialize_tiber(tiber* tb)
{
	tb->thread_pool = NULL;
	free(tb->stack);
	tb->stack = NULL;
	pthread_spin_destroy(&(tb->tiber_context_lock));
	pthread_spin_destroy(&(tb->tiber_state_lock));
}

void tiber_cond_init(tiber_cond* tc)
{
	pthread_spin_init(&(tc->lock), PTHREAD_PROCESS_PRIVATE);
	initialize_linkedlist(&(tc->tiber_cond_waiters), offsetof(tiber, embed_node_for_tiber_cond_waiters));
}

void tiber_cond_wait(tiber_cond* tc, pthread_mutex_t m);

void tiber_cond_signal(tiber_cond* tc);

void tiber_cond_broadcast(tiber_cond* tc);

void tiber_cond_destroy(tiber_cond* tc)
{
	pthread_spin_destroy(&(tc->lock));
}