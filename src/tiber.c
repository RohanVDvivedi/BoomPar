#include<boompar/tiber.h>

#include<stdio.h>
#include<stdlib.h>

// the current tiber that this thread is executing get's stored here
__thread tiber* curr_tiber = NULL;

/*
	puts tiber as thread local variable of this thread
	puts tiber in running state
	takes it's context lock
	swaps context running the tiber
	releases it's context lock
	returns

	** only this function takes the context_lock of the tiber
*/
static void* tiber_execute_wrapper(void* tb_v)
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
	curr_tiber->tiber_context.uc_link = &(curr_tiber->tiber_caller);
	if(-1 == swapcontext(&(curr_tiber->tiber_caller), &(curr_tiber->tiber_context)))
	{
		printf("TIBER BUG: tiber could not context switch into itself\n");
		exit(-1);
	}
	pthread_spin_unlock(&(curr_tiber->tiber_context_lock));

	// change curr_tiber's state if running to killed
	// because it returned from the entry function
	pthread_spin_lock(&(curr_tiber->tiber_state_lock));
	if(curr_tiber->state == TIBER_RUNNING)
		curr_tiber->state = TIBER_KILLED;
	pthread_spin_unlock(&(curr_tiber->tiber_state_lock));

	// reset the thread local
	curr_tiber = NULL;

	return NULL;
}

int initialize_and_run_tiber(tiber* tb, executor* thread_pool, void (*entry_func)(void* input_p), void* input_p, uint64_t stack_size)
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
	tb->tiber_context.uc_link = &(tb->tiber_caller);
	makecontext(&(tb->tiber_context), (void(*)(void))entry_func, 1, input_p);

	pthread_spin_init(&(tb->tiber_state_lock), PTHREAD_PROCESS_PRIVATE);
	tb->state = TIBER_QUEUED;

	initialize_llnode(&(tb->embed_node_for_tiber_cond_waiters));

	// do the actual queueing, pushing it into the thread pool
	submit_job_executor(tb->thread_pool, tiber_execute_wrapper, tb, NULL, NULL, BLOCKING);

	return 1;
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
		printf("TIBER BUG: tiber could not context switch into it's caller for yielding\n");
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
		printf("TIBER BUG: tiber could not context switch into it's caller for sucide\n");
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

void tiber_cond_wait(tiber_cond* tc, pthread_mutex_t* m)
{
	// put curr_tiber into waiting state
	pthread_spin_lock(&(tc->lock));
	pthread_spin_lock(&(curr_tiber->tiber_state_lock));
	if(curr_tiber->state == TIBER_RUNNING)
		curr_tiber->state = TIBER_WAITING;
	else
	{
		printf("TIBER BUG: tiber attempted waiting for tiber_cond but not in running state\n");
		exit(-1);
	}
	pthread_spin_unlock(&(curr_tiber->tiber_state_lock));
	insert_tail_in_linkedlist(&(tc->tiber_cond_waiters), curr_tiber);
	pthread_spin_unlock(&(tc->lock));

	// unlock the mutex
	pthread_mutex_unlock(m);

	// swap the context out
	if(-1 == swapcontext(&(curr_tiber->tiber_context), &(curr_tiber->tiber_caller)))
	{
		printf("TIBER BUG: tiber could not context switch into it's caller for waiting over tiber_cond\n");
		exit(-1);
	}

	// lock it back
	pthread_mutex_lock(m);
}

void tiber_cond_signal(tiber_cond* tc)
{
	tiber* twup = NULL;

	pthread_spin_lock(&(tc->lock));

	if(!is_empty_linkedlist(&(tc->tiber_cond_waiters)))
	{
		twup = (tiber*) get_head_of_linkedlist(&(tc->tiber_cond_waiters));
		remove_from_linkedlist(&(tc->tiber_cond_waiters), twup);

		pthread_spin_lock(&(twup->tiber_state_lock));
		if(twup->state == TIBER_WAITING)
			twup->state = TIBER_QUEUED;
		else
		{
			printf("TIBER BUG: tiber_cond has non waiting tiber in tiber_cond_waiters for signal\n");
			exit(-1);
		}
		pthread_spin_unlock(&(twup->tiber_state_lock));
	}

	pthread_spin_unlock(&(tc->lock));

	// do the actual queueing, pushing it into the thread pool
	submit_job_executor(twup->thread_pool, tiber_execute_wrapper, twup, NULL, NULL, BLOCKING);
}

void tiber_cond_broadcast(tiber_cond* tc)
{
	linkedlist twup;
	initialize_linkedlist(&twup, offsetof(tiber, embed_node_for_tiber_cond_waiters));

	pthread_spin_lock(&(tc->lock));

	while(!is_empty_linkedlist(&(tc->tiber_cond_waiters)))
	{
		tiber* ttwup = (tiber*) get_head_of_linkedlist(&(tc->tiber_cond_waiters));
		remove_from_linkedlist(&(tc->tiber_cond_waiters), ttwup);

		pthread_spin_lock(&(ttwup->tiber_state_lock));
		if(ttwup->state == TIBER_WAITING)
			ttwup->state = TIBER_QUEUED;
		else
		{
			printf("TIBER BUG: tiber_cond has non waiting tiber in tiber_cond_waiters for broadcast\n");
			exit(-1);
		}
		pthread_spin_unlock(&(ttwup->tiber_state_lock));

		insert_tail_in_linkedlist(&twup, ttwup);
	}

	pthread_spin_unlock(&(tc->lock));

	// do the actual queueing, pushing it all into the thread pool
	while(!is_empty_linkedlist(&twup))
	{
		tiber* ttwup = (tiber*) get_head_of_linkedlist(&twup);
		remove_from_linkedlist(&twup, ttwup);

		submit_job_executor(ttwup->thread_pool, tiber_execute_wrapper, ttwup, NULL, NULL, BLOCKING);
	}
}

void tiber_cond_destroy(tiber_cond* tc)
{
	pthread_spin_destroy(&(tc->lock));
}