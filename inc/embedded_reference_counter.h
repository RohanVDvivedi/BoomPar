#ifndef EMBEDDED_REFERENCE_COUNTER_H
#define EMBEDDED_REFERENCE_COUNTER_H

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

// this call returns the same pointer but after incrementing its reference counter
void* get_shareable_reference_copy(void* data_p);

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

	Your new function must look like this

	your_type* new_your_type()
	{
		your_type* data_p = malloc(sizeof(your_type));

		// you must initialize your counter, right after you allocate it
		initialize_reference_counter(data_p);

		initialize_your_type(data_p);
		return data_p;
	}

	Your delete functions must look like this

	void delete_data(void* data)
	{
		// add this to check if we are the last one to hold this reference
		if(decrement_reference_counter(data_p)) // check if alive
			return;

		deinitialize_data(data_p);
		free(data_p);
	}

	// before inserting your_type into any datastructure you must do this

	your_type* new_ref = get_shareable_reference_copy(data_p);

	and then insert this new_ref in the desired data structure
	and upon removal, delete the data after it is not in use
	this ensures that every data structure has its reference counted
*/

#endif