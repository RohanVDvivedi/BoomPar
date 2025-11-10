#include<boompar/sync_ll_queue.h>

sync_ll_queue* new_sync_ll_queue(cy_uint node_offset);

int initialize_sync_ll_queue(sync_ll_queue* sq, cy_uint node_offset);

void deinitialize_sync_ll_queue(sync_ll_queue* sq);

void delete_sync_ll_queue(sync_ll_queue* sq);

int is_empty_sync_ll_queue(sync_ll_queue* sq);

int push_sync_ll_queue(sync_ll_queue* sq, const void* data_p);

const void* pop_sync_ll_queue(sync_ll_queue* sq, uint64_t timeout_in_microseconds);

void close_sync_ll_queue(sync_ll_queue* sq);

uint64_t get_threads_waiting_on_empty_sync_ll_queue(sync_ll_queue* sq);