#ifndef WORKER_H
#define WORKER_H

#include<boompar/job.h>
#include<boompar/sync_queue.h>

// A worker thread is detached thread
// It will keep on dequeuing jobs from the job_queue and executing them
// It provides a call back for 2 events : start_up on starting up, and clean_up on exiting
// DO NOT CANCEL A WORKER THREAD, THIS MAY LOCK THE JOB_QUEUE PERMANENTLY
// even if you cancel we assure you there will not be partial execution of jobs, 
// or memory leaks from unfinished, unreleased job memory
// please use submit_stop_worker to asynchronously stopping the worker

// returns error code returned by pthread_create
// 0 return value is a success
int start_worker(
					pthread_t* thread_id,							// ** thread_id returned on success
					sync_queue* job_queue, 							// ** the worker thread uses this sync_queue to receive jobs to finish
					uint64_t job_queue_empty_timeout_in_microseconds, // ** timeout for the job_queue while we blockingly wait on it, BLOCKING is also an acceptable value, but fails on NON_BLOCKING
					void(*start_up)(void* additional_params), 		// * start_up function is called by the thread on start up 
					void(*clean_up)(void* additional_params), 		// * clean_up function is called by the thread before returning
					void* additional_params							// * this parameters will be passed to the start_up and clean_up functions
				);
// note
// ** mandatory parameters to the function
// * optional parameters to the function

// For the below submit_ job/stop _worker functions, the timeout is the time you intend to wait until submission is successfull
// it can also be BLOCKING or NON_BLOCKING

// create and submit job (with/without promise), returns 1 if the job was successfully submitted to the worker
// function fails and returns 0 if, the job_queue is blocking and it is full
// if this function fails with a 0, then the corresponding promise (if any) will never be fulfilled, because
// a return of 0 from this function, implies that a corresponding job with the given parameters never existed
int submit_job_worker(sync_queue* job_queue, void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output, void (*cancellation_callback)(void* input_p), uint64_t submission_timeout_in_microseconds);

// The function below will submit a NULL in the job_queue, this will stop 1 worker listening on the queue
// and this will kill any one worker that dequeues this NULL job
// to stop all the workers listening on this very specific job_queue, then you may instead call close_sync_queue
// It returns 1, if NULL was pushed, else it will return 0
int submit_stop_worker(sync_queue* job_queue, uint64_t submission_timeout_in_microseconds);

// to discard jobs that are still remaining, in the job queue, even after the worker thread has been killed
// there would not be any jobs remaining in the queue after this call
// all the jobs will be CANCELLED, i.e. their promises will be fulfilled as NULL
void discard_leftover_jobs(sync_queue* job_queue);

#endif