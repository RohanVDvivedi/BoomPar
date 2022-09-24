#include<smart_pointer.h>

#include<pthread.h>

const smart_pointer_builder initialize_smart_pointer_builder(memory_allocator allocator, unsigned int data_size, void (*default_initializer)(void* data_p), void (*deinitializer)(void* data_p))
{
	// if default allocator is NULL then use STD_c_mem_allocator
	if(allocator == NULL)
		allocator = STD_C_mem_allocator;

	return (smart_pointer_builder){allocator, data_size, default_initializer, deinitializer};
}

static void* new_data(smart_pointer_builder const * sp_builder_p)
{
	// allocate data
	unsigned int data_size = sp_builder_p->data_size;
	void* data_p = allocate(sp_builder_p->allocator, &(data_size));

	// and run the default initializer
	sp_builder_p->default_initializer(data_p);
	return data_p;
}

static void delete_data(void* data_p, smart_pointer_builder const * sp_builder_p)
{
	// deinitialize and deallocate data
	sp_builder_p->deinitializer(data_p);
	deallocate(sp_builder_p->allocator, data_p, sp_builder_p->data_size);
}

typedef struct smart_pointer_internals smart_pointer_internals;
struct smart_pointer_internals
{
	const smart_pointer_builder* sp_builder_p;

	// this lock is only for the reference counter
	pthread_mutex_t reference_count_lock;

	// the actual reference counter of this pointer
	// it indicates the number of smart_pointers that point to data_p at any point in time
	unsigned int reference_count;
};

static const smart_pointer NULL_smart_pointer = {NULL, NULL};

static smart_pointer_internals* new_smart_pointer_internals(smart_pointer_builder const * sp_builder_p)
{
	// allocate smart_pointer internals
	unsigned int spnt_size = sizeof(smart_pointer_internals);
	smart_pointer_internals* spnt_p = allocate(sp_builder_p->allocator, &spnt_size);

	// initialize its attributes
	spnt_p->sp_builder_p = sp_builder_p;
	pthread_mutex_init(&(spnt_p->reference_count_lock), NULL);
	spnt_p->reference_count = 1;

	return spnt_p;
}

static void delete_smart_pointer_internals(smart_pointer_internals* spnt_p)
{
	pthread_mutex_destroy(&(spnt_p->reference_count_lock));
	deallocate(spnt_p->sp_builder_p->allocator, spnt_p, spnt_p->sp_builder_p->data_size);
}

int is_smart_pointer_NULL(const smart_pointer* sp_p)
{
	return (sp_p->data_p == NULL) || (sp_p->spnt_p == NULL);
}

smart_pointer create_smart_pointer(smart_pointer_builder const * sp_builder_p)
{
	smart_pointer sp;

	// create a new smart_pointer's internals
	sp.spnt_p = new_smart_pointer_internals(sp_builder_p);

	// allocate for the object and initialize it, using the given smart_pointer builder
	sp.data_p = new_data(sp.spnt_p->sp_builder_p);

	return sp;
}

smart_pointer duplicate_smart_pointer(smart_pointer const * sp_p)
{
	if(is_smart_pointer_NULL(sp_p))
		return NULL_smart_pointer;

	// increment the reference counter
	pthread_mutex_lock(&(sp_p->spnt_p->reference_count_lock));
	sp_p->spnt_p->reference_count++;
	pthread_mutex_unlock(&(sp_p->spnt_p->reference_count_lock));

	return *sp_p;
}

int destroy_smart_pointer(smart_pointer* sp_p)
{
	if(is_smart_pointer_NULL(sp_p))
		return 0;

	// decrement the reference counter, and cache the new value
	pthread_mutex_lock(&(sp_p->spnt_p->reference_count_lock));
	unsigned int reference_count = --sp_p->spnt_p->reference_count;
	pthread_mutex_unlock(&(sp_p->spnt_p->reference_count_lock));

	// use the cache value to check if you are the one that made the reference count 0
	if(reference_count == 0)
	{
		// if yes, then deinitialize the data and delete (free) it, using the smart_pointer_builder
		delete_data(sp_p->data_p, sp_p->spnt_p->sp_builder_p);

		// delete the smart_pointer's internals
		delete_smart_pointer_internals(sp_p->spnt_p);

		// set this pointer to NULL_smart_pointer, so that it may not be used any further
		*sp_p = NULL_smart_pointer;
		return 1;
	}

	return 0;
}

void reassign_smart_pointer(smart_pointer* sp_p, const smart_pointer* sp_from_p)
{
	destroy_smart_pointer(sp_p);
	*sp_p = duplicate_smart_pointer(sp_from_p);
}