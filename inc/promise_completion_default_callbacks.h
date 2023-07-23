#ifndef PROMISE_COMPLETION_DEFAULT_CALLBACKS_H
#define PROMISE_COMPLETION_DEFAULT_CALLBACKS_H

#include<promise.h>
#include<sync_queue.h>

// This header and its source (promise_completion_default_callbacks.*) are just an illustration of how callback on the promise completion should be used
// This sources are for your reference only.

// It basically pushed a completion promise into a sync queue, so that it can be concurrently popped and processed.
// In real worls application, inside such a callback, you may also want to delete the promise and promised result, aswell as do some book keeping for your data structures, update a hashmap of promises or a write something to a connection, etc.

promise_completion_callback push_to_sync_queue_on_promise_completion_callback(sync_queue* sq);

#endif