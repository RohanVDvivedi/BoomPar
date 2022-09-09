#ifndef PROMISE_ALL_H
#define PROMISE_ALL_H

#include<array.h>

#include<sync_queue.h>

typedef sync_queue promise_all;

promise_all* new_promise_all(unsigned int promises_count, const promise** promises);

void initialize_promise_all(promise_all* pa, unsigned int promises_count, promise** promises);

void register_promise(promise_all* pa, promise* p);

array get_promised_results(promise_all* pa, unsigned int result_count);

void deinitialize_promise_all(promise_all* pa);

void delete_promise_all(promise_all* pa);

#endif