#include<boompar/resource_usage_limiter.h>
#include<boompar/job.h>

resource_usage_limiter* rul_p = NULL;

void* job12(void* p)
{
	int tid = (int)p;
}

void* job3(void* p)
{
	int tid = (int)p;
}

uint64_t request_resources(int tid, resource_usage_limiter* rul_p, uint64_t min_resource_count, uint64_t max_resource_count, uint64_t timeout_in_microseconds, break_resource_waiting* break_out)
{
	printf("tid = %d, requesting %"PRIu64"-%"PRIu64" resources\n", min_resource_count, max_resource_count);
	uint64_t res = request_resources_from_resource_usage_limiter(rul_p, min_resource_count, max_resource_count, timeout_in_microseconds, break_out);
	printf("tid = %d, granted %"PRIu64" resources\n", tid, res);
	return res;
}

int give_back_resources(int tid, resource_usage_limiter* rul_p, uint64_t granted_resource_count)
{
	printf("tid = %d, giving back %"PRIu64" resources\n", granted_resource_count);
	int res = give_back_resources_to_resource_usage_limiter(rul_p, granted_resource_count);
	if(res)
		printf("tid = %d, gave back %"PRIu64" resources\n", tid, granted_resource_count);
	else
		printf("tid = %d, FAILED to give back %"PRIu64" resources\n", tid, granted_resource_count);
	return res;
}

uint64_t min_resource_count = 3;
uint64_t max_resource_count = 5;

break_resource_waiting jout1 = INIT_BREAK_OUT;
break_resource_waiting jout2 = INIT_BREAK_OUT;
break_resource_waiting jout3 = INIT_BREAK_OUT;

int main()
{
	rul_p = new_resource_usage_limiter(14);
	printf("started resource_usage_limiter with resource_count of 14\n");

	job job1; initialize_job(&job1, my_job_function_simple, (void*)1, NULL, NULL);
	job job2; initialize_job(&job2, my_job_function_simple, (void*)2, NULL, NULL);
	job job3; initialize_job(&job3, my_job_function_simple, (void*)3, NULL, NULL);
	pthread_t thread_id;
	if(execute_job_async(&jobs1, &thread_id))
		exit(-1);
	if(execute_job_async(&jobs2, &thread_id))
		exit(-1);
	if(execute_job_async(&jobs3, &thread_id))
		exit(-1);
	printf("started all 3 jobs\n");


	printf("shutting down resource_usage_limiter\n");
	delete_resource_usage_limiter(rul_p, 1);

	return 0;
}