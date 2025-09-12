#include<boompar/resource_usage_limiter.h>
#include<boompar/job.h>

#include<posixutils/pthread_cond_utils.h>

#include<stdio.h>
#include<unistd.h>
#include<inttypes.h>

resource_usage_limiter* rul_p = NULL;

uint64_t request_resources(int tid, resource_usage_limiter* rul_p, uint64_t min_resource_count, uint64_t max_resource_count, uint64_t timeout_in_microseconds, break_resource_waiting* break_out)
{
	printf("tid = %d, requesting %"PRIu64"-%"PRIu64" resources\n", tid, min_resource_count, max_resource_count);
	uint64_t res = request_resources_from_resource_usage_limiter(rul_p, min_resource_count, max_resource_count, timeout_in_microseconds, break_out);
	printf("tid = %d, granted %"PRIu64" resources\n", tid, res);
	return res;
}

int give_back_resources(int tid, resource_usage_limiter* rul_p, uint64_t granted_resource_count)
{
	printf("tid = %d, giving back %"PRIu64" resources\n", tid, granted_resource_count);
	int res = give_back_resources_to_resource_usage_limiter(rul_p, granted_resource_count);
	if(res)
		printf("tid = %d, gave back %"PRIu64" resources\n", tid, granted_resource_count);
	else
		printf("tid = %d, FAILED to give back %"PRIu64" resources\n", tid, granted_resource_count);
	return res;
}

uint64_t min_resource_count = 3;
uint64_t max_resource_count = 5;

break_resource_waiting jouts[3] = {INIT_BREAK_OUT, INIT_BREAK_OUT, INIT_BREAK_OUT};

void* job12(void* p)
{
	int tid = (long long int)p;
	uint64_t res;

	// part 1
	usleep((tid+1) * 500);
	res = request_resources(tid, rul_p, min_resource_count, max_resource_count, NON_BLOCKING, jouts+tid);
	usleep(4 * 500);
	give_back_resources(tid, rul_p, res);
	usleep((4-tid) * 500);

	// part 2
	usleep((tid+1) * 500);
	res = request_resources(tid, rul_p, min_resource_count, max_resource_count, BLOCKING, jouts+tid);
	usleep(4 * 500);
	give_back_resources(tid, rul_p, res);
	usleep((4-tid) * 500);

	return NULL;
}

void* job3_(void* p)
{
	int tid = (long long int)p;
	uint64_t res;

	// part 1
	usleep((tid+1) * 500);
	res = request_resources(tid, rul_p, min_resource_count, max_resource_count, NON_BLOCKING, jouts+tid);
	usleep(4 * 500);
	give_back_resources(tid, rul_p, res);
	usleep((4-tid) * 500);

	// part 2
	usleep((tid+1) * 500);
	res = request_resources(tid, rul_p, min_resource_count, max_resource_count, BLOCKING, jouts+tid);
	usleep(4 * 500);
	give_back_resources(tid, rul_p, res);
	usleep((4-tid) * 500);

	return NULL;
}

int main()
{
	rul_p = new_resource_usage_limiter(14);
	printf("started resource_usage_limiter with resource_count of 14\n");

	job job1; initialize_job(&job1, job12, (void*)((long long int)1), NULL, NULL);
	job job2; initialize_job(&job2, job12, (void*)((long long int)2), NULL, NULL);
	job job3; initialize_job(&job3, job3_, (void*)((long long int)3), NULL, NULL);
	pthread_t thread_id;
	if(execute_job_async(&job1, &thread_id))
		exit(-1);
	if(execute_job_async(&job2, &thread_id))
		exit(-1);
	if(execute_job_async(&job3, &thread_id))
		exit(-1);
	printf("started all 3 jobs\n");

	printf("\n\nPART 1\n\n");
	usleep(9 * 500);

	printf("\n\nPART 2\n\n");
	usleep(9 * 500);

	printf("\n\nPART 3\n\n");

	printf("\n\nPART 4\n\n");

	printf("shutting down resource_usage_limiter\n");
	delete_resource_usage_limiter(rul_p, 1);

	return 0;
}