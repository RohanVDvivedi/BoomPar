#include<embedded_reference_counter.h>

void initialize_reference_counter(void* data_p)
{
	reference_counter* rc_p = data_p;
	pthread_spin_init(&(rc_p->reference_count_lock), PTHREAD_PROCESS_PRIVATE);
	rc_p->reference_count = 1;
}

void* get_shareable_reference_copy(void* data_p)
{
	reference_counter* rc_p = data_p;
	pthread_spin_lock(&(rc_p->reference_count_lock));
	rc_p->reference_count++;
	pthread_spin_unlock(&(rc_p->reference_count_lock));
	return data_p;
}

int decrement_reference_counter(void* data_p)
{
	reference_counter* rc_p = data_p;
	pthread_spin_lock(&(rc_p->reference_count_lock));
	int is_alive = ((--rc_p->reference_count) > 0);			// data is alive/reachable if the reference count is more than 0 after a decrement
	pthread_spin_unlock(&(rc_p->reference_count_lock));
	return is_alive;
}