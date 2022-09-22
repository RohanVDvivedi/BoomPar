#include<smart_pointer.h>

#include<pthread.h>

const smart_pointer_builder initialize_smart_pointer_builder(memory_allocator allocator, unsigned int data_size, void (*default_initializer)(void* data_p), void (*deinitializer)(void* data_p))
{
	return (smart_pointer_builder){allocator, data_size, default_initializer, deinitializer};
}

typedef struct smart_pointer_internals smart_pointer_internals;
struct smart_pointer_internals
{
	const smart_pointer_builder* sp_builder;

	// this lock is only for the reference counter
	pthread_mutex_t reference_counter_lock;

	// the actual reference counter of this pointer
	// it indicates the number of smart_pointers that point to data_p at any point in time
	unsigned int reference_counter;
};