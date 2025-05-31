#include<boompar/periodic_job.h>

#include<posixutils/timespec_utils.h>

#include<stdio.h>
#include<inttypes.h>
#include<unistd.h>

uint64_t millis()
{
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return timespec_to_milliseconds(spec);
}

uint64_t millis_begin = -1;

void periodic_function(void* t)
{
	printf("Hello from periodic function at %"PRIu64"\n", (millis() - millis_begin));
}

int main()
{
	millis_begin = millis();
	printf("Test begins at %"PRIu64"\n", millis_begin);
	periodic_job* pjob = new_periodic_job(periodic_function, NULL, 3000000);

	sleep(10);

	delete_periodic_job(pjob);
}