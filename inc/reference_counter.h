#ifndef REFERENCE_COUNTER_H
#define REFERENCE_COUNTER_H

#include<pthread.h>

// to make a reference counted type, embed this struct as the first attribute of the struct
typedef struct reference_counter reference_counter;
struct reference_counter
{
	// lock to be taken while incrementing or decrementing the reference counter
	pthread_spinlock_t reference_count_lock;

	// the actual reference counter
	unsigned int reference_count;
};

// initializes the embedded reference counter to 1
void initialize_reference_counter(void* data_p);

// this call increments the reference counter
void increment_reference_counter(void* data_p);

// decrements reference counter, and returns 1, if the reference counter is still greater than 0
// i.e. if this functions returns 0 then the data can be deinitialized and freed
// the return value suggests if anyone is still referencing it
int decrement_reference_counter(void* data_p);

/*
	USAGE GUIDELINES

	typedef struct your_type your_type;
	struct your_type
	{
		reference_counter embed_ref_cntr; // reference_counter must be the first attribute in your_type

		... (other attributes)
	};

	your_type* initialize_your_type(your_type* data_p)
	{
		initialize_reference_counter(data_p);

		... (initialize other attributes)
	}

	void delete_your_type(void* data)
	{
		if(decrement_reference_counter(data_p))
			return;

		deinitialize_data(data_p);
		free(data_p);
	}

	before inserting your_type into any datastructure you must do this

	increment_reference_counter(data_p);

	and then if this plausible insertion fails then

	decrement_reference_counter(data_p);

	upon removal of an element you may only call delete on the element

	this ensures that every data structure has its reference counted
*/

#endif