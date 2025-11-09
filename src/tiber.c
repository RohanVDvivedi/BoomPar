#include<boompar/tiber.h>

#include<stdlib.h>

// the current tiber that this thread is executing get's stored here
__thread tiber* curr_tiber = NULL;

int initialize_tiber(tiber* tb, void (*entry_func)(void* input_p), void* input_p, uint64_t stack_size)
{
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

void run_tiber(tiber* tb);

void yield_tiber();

void kill_tiber();

void deinitialize_tiber(tiber* tb)
{
	free(tb->stack);
	tb->stack = NULL;
	pthread_spin_destroy(&(tb->tiber_context_lock));
	pthread_spin_destroy(&(tb->tiber_state_lock));
}