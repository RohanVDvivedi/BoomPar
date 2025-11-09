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
	tb->state = TIBER_QUEUED;

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

void enqueue_tiber(tiber* tb);

void yield_tiber();

void kill_tiber();

void deinitialize_tiber(tiber* tb)
{
	tb->thread_pool = NULL;
	free(tb->stack);
	tb->stack = NULL;
	pthread_spin_destroy(&(tb->tiber_context_lock));
	pthread_spin_destroy(&(tb->tiber_state_lock));
}