#ifndef SMART_POINTER_H
#define SMART_POINTER_H

#include<memory_allocator_interface.h> // we will use the memory allocator interface of Cutlery

typedef struct smart_pointer_builder smart_pointer_builder;
struct smart_pointer_builder
{
	// allocator to be used by every data pointed to by the smart_pointer built using this smart_pointer builder
	memory_allocator allocator;

	// this must be the size of the struct/int that you are using with the smart_pointer
	unsigned int data_size;

	// the function that can be used to initialize the data (that has been already allocated by the allocator)
	// in this function, you must only default initialize your data
	// things like set pointers to NULL, strings to empty string values, ints to 0 etc
	// you may perform parameter specific initialization later on your own
	void (*default_initializer)(void* data_p);

	// the function that can be used to deinitialize the data (do not free the memory here, only destroy its members)
	void (*deinitializer)(void* data_p);
};

const smart_pointer_builder initialize_smart_pointer_builder(memory_allocator allocator, unsigned int data_size, void (*default_initializer)(void* data_p), void (*deinitializer)(void* data_p));

typedef struct smart_pointer_internals smart_pointer_internals;

typedef struct smart_pointer smart_pointer;
struct smart_pointer
{
	// pointer to actual data value
	// this must be and must remain the first attribute of the smart_pointer for easy access to it, you will understand why later
	void* data_p;

	// the below attribute is an internal implementation of the smart_pointer
	// you may not access it
	struct smart_pointer_internals* spnt_p;

	// for a NULL smart_pointer both the above attributes will be NULL
};

// creates a new data using the provided smart_pointer_builder
smart_pointer create_smart_pointer(smart_pointer_builder const * sp_builder);

/* to access data_p you may do the following

	smart_pointer sp;

	if( (( <your_data_type> *)sp) == NULL )
		printf("smart_pointer sp is NULL\n");

	the above lines are perfectly valid because the data_p, i.e. pointer to your actual data is the first attribute of the smart_pointer
*/

// create another smart_pointer that points to the same data_p thet (*sp_p) points to, and return that smart_pointer
// it will increase the reference counter in sp_p->spnt_p and returns (*sp_p)
smart_pointer duplicate_smart_pointer(const smart_pointer* sp_p);

// NOTE:: A SMART_POINTER MUST ONLY BE CREATED THE ABOVE 2 METHODS, AND AFTER USE IT MUST BE DESTROYED BY THE METHOD BELOW

// destroy returns 1 if this call freed the data at data_p
// it will always return 0 for a NULL smart_pointer
// it will decrement the reference counter and if the reference count is 0 then the smart_pointer sp_p is set to NULL (i.e. data_p and spnt_p are made NULL)
int destroy_smart_pointer(smart_pointer* sp_p);

#endif