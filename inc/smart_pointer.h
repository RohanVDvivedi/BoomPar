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
	void* data_p;

	// the below attribute is an internal implementation of the smart_pointer
	// you may not access it
	struct smart_pointer_internals* spnt_p;
};

#endif