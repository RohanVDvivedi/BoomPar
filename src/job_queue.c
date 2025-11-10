#include<boompar/job_queue.h>

job_queue* new_job_queue(cy_uint max_capacity)
{
	job_queue* jq = malloc(sizeof(job_queue));
	if(jq == NULL)
		return NULL;
	if(!initialize_job_queue(jq, max_capacity))
	{
		free(jq);
		return NULL;
	}
	return jq;
}

int initialize_job_queue(job_queue* jq, cy_uint max_capacity)
{
	if(max_capacity == JOB_QUEUE_AS_LINKEDLIST)
	{
		jq->type = LINKEDLIST_TYPE_JOB_QUEUE;
		initialize_sync_ll_queue(&(jq->lsq), offsetof(job, embed_node_to_link_jobs_in_sync_ll_queue));
		return 1;
	}
	else
	{
		jq->type = ARRAYLIST_TYPE_JOB_QUEUE;
		return initialize_sync_queue(&(jq->sq), max_capacity);
	}
}

void deinitialize_job_queue(job_queue* jq)
{
	switch(jq->type)
	{
		case LINKEDLIST_TYPE_JOB_QUEUE :
		{
			deinitialize_sync_ll_queue(&(jq->lsq));
			break;
		}
		case ARRAYLIST_TYPE_JOB_QUEUE :
		{
			deinitialize_sync_queue(&(jq->sq));
			break;
		}
	}
}

void delete_job_queue(job_queue* jq)
{
	deinitialize_job_queue(jq);
	free(jq);
}

cy_uint get_max_capacity_job_queue(job_queue* jq)
{
	switch(jq->type)
	{
		case LINKEDLIST_TYPE_JOB_QUEUE :
		{
			return CY_UINT_MAX;
		}
		case ARRAYLIST_TYPE_JOB_QUEUE :
		{
			return get_max_capacity_sync_queue(&(jq->sq));
		}
		default :
		{
			return 0;
		}
	}
}

void update_max_capacity_job_queue(job_queue* jq, cy_uint new_max_capacity)
{
	switch(jq->type)
	{
		case LINKEDLIST_TYPE_JOB_QUEUE :
		{
			break;
		}
		case ARRAYLIST_TYPE_JOB_QUEUE :
		{
			update_max_capacity_sync_queue(&(jq->sq), new_max_capacity);
			break;
		}
	}
}

int is_full_job_queue(job_queue* jq)
{
	switch(jq->type)
	{
		case LINKEDLIST_TYPE_JOB_QUEUE :
		{
			return 0;
		}
		case ARRAYLIST_TYPE_JOB_QUEUE :
		{
			return is_full_sync_queue(&(jq->sq));
		}
		default :
		{
			return 1;
		}
	}
}

int is_empty_job_queue(job_queue* jq)
{
	switch(jq->type)
	{
		case LINKEDLIST_TYPE_JOB_QUEUE :
		{
			return is_empty_sync_ll_queue(&(jq->lsq));
		}
		case ARRAYLIST_TYPE_JOB_QUEUE :
		{
			return is_empty_sync_queue(&(jq->sq));
		}
		default :
		{
			return 0;
		}
	}
}

int push_job_queue(job_queue* jq, const job* job_p, uint64_t timeout_in_microseconds)
{
	switch(jq->type)
	{
		case LINKEDLIST_TYPE_JOB_QUEUE :
		{
			return push_sync_ll_queue(&(jq->lsq), job_p);
		}
		case ARRAYLIST_TYPE_JOB_QUEUE :
		{
			return push_sync_queue(&(jq->sq), job_p, timeout_in_microseconds);
		}
		default :
		{
			return 0;
		}
	}
}

const job* pop_job_queue(job_queue* jq, uint64_t timeout_in_microseconds)
{
	switch(jq->type)
	{
		case LINKEDLIST_TYPE_JOB_QUEUE :
		{
			return pop_sync_ll_queue(&(jq->lsq), timeout_in_microseconds);
		}
		case ARRAYLIST_TYPE_JOB_QUEUE :
		{
			return pop_sync_queue(&(jq->sq), timeout_in_microseconds);
		}
		default :
		{
			return NULL;
		}
	}
}

void close_job_queue(job_queue* jq)
{
	switch(jq->type)
	{
		case LINKEDLIST_TYPE_JOB_QUEUE :
		{
			close_sync_ll_queue(&(jq->lsq));
			break;
		}
		case ARRAYLIST_TYPE_JOB_QUEUE :
		{
			close_sync_queue(&(jq->sq));
			break;
		}
	}
}

uint64_t get_threads_waiting_on_empty_job_queue(job_queue* jq)
{
	switch(jq->type)
	{
		case LINKEDLIST_TYPE_JOB_QUEUE :
		{
			return get_threads_waiting_on_empty_sync_ll_queue(&(jq->lsq));
		}
		case ARRAYLIST_TYPE_JOB_QUEUE :
		{
			return get_threads_waiting_on_empty_sync_queue(&(jq->sq));
		}
		default :
		{
			return 0;
		}
	}
}

uint64_t get_threads_waiting_on_full_job_queue(job_queue* jq)
{
	switch(jq->type)
	{
		case LINKEDLIST_TYPE_JOB_QUEUE :
		{
			return 0;
		}
		case ARRAYLIST_TYPE_JOB_QUEUE :
		{
			return get_threads_waiting_on_full_sync_queue(&(jq->sq));
		}
		default :
		{
			return 0;
		}
	}
}