#ifndef WORKER_H
#define WORKER_H

#include<job.h>
#include<sync_queue.h>

// A worker thread is detached thread
// It will keep on dequeuing jobs from the job_queue and executing them
// It provides a call back for 2 events : start_up on starting up, and clean_up on exiting
// DO NOT CANCEL A WORKER THREAD, THIS MAY LOCK THE JOB_QUEUE PERMANENTLY
// even if you cancel we assure you there will not be partial execution of jobs, 
// or memory leaks from unfinished, unreleased job memory
// please use submit_stop_worker to asynchronously stopping the worker

pthread_t start_worker(
							sync_queue* job_queue, 							// ** the worker thread uses this sync_queue to receive jobs to finish
							unsigned long long int job_queue_empty_timeout_in_microseconds, // ** timeout for the job_queue while we blockingly wait on it 
							void(*start_up)(void* additional_params), 		// * start_up function is called by the thread on start up 
							void(*clean_up)(void* additional_params), 		// * clean_up function is called by the thread before returning
							void* additional_params							// * this parameters will be passed to the start_up and clean_up functions
						);
// note
// ** mandatory parameters to the function
// * optional parameters to the function

// create and submit job (with/without promise), returns 1 if the job was successfully submitted to the worker
// function fails and returns 0 if, the job_queue is blocking and it is full 
int submit_job_worker(sync_queue* job_queue, void* (*function_p)(void* input_p), void* input_p, promise* promise_for_output);

// The function below will submit a NULL in the job_queue, this kill any one worker that dequeues this NULL job
// It returns 1, if NULL was pushed, else it will return NULL
int submit_stop_worker(sync_queue* job_queue);

// to discard jobs that are still remaining, in the job queue, even after the worker thread has been killed
// there would not be any jobs remaining in the queue after this call
void discard_leftover_jobs(sync_queue* job_queue);

#endif