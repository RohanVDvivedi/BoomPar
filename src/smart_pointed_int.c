#include<smart_pointed_int.h>

#include<stddef.h>
#include<memory_allocator_interface.h>

smart_pointer_builder const * smart_pointed_int_builder = &((const smart_pointer_builder){
																		&STD_C_memory_allocator,
																		sizeof(int),
																		NULL,
																		NULL,
																	});

