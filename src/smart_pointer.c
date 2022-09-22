#include<smart_pointer.h>

const smart_pointer_builder initialize_smart_pointer_builder(memory_allocator allocator, unsigned int data_size, void (*default_initializer)(void* data_p), void (*deinitializer)(void* data_p))
{
	return (smart_pointer_builder){allocator, data_size, default_initializer, deinitializer};
}