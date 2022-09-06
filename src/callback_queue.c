#include<callback_queue.h>

void initialize_callback_queue(callback_queue* cbq, unsigned int capacity)
{
	initialize_dstream(cbq, capacity * sizeof(callback));
}

int is_empty_callback_queue(const callback_queue* cbq)
{
	return is_empty_dstream(cbq);
}

void push_callback(callback_queue* cbq, callback cb)
{
	int pushed = write_to_dstream(cbq, &cb, sizeof(cb), ALL_OR_NONE);
	if(!pushed)
	{
		resize_dstream(cbq, (get_capacity_dstream(cbq) + 3 * sizeof(cb)) * 2);
		write_to_dstream(cbq, &cb, sizeof(cb), ALL_OR_NONE);
	}
}

callback pop_callback(callback_queue* cbq)
{
	callback cb;
	read_from_dstream(cbq, &cb, sizeof(cb), ALL_OR_NONE);
	return cb;
}

void deinitialize_callback_queue(callback_queue* cbq)
{
	deinitialize_dstream(cbq);
}