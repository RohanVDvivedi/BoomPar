#include<boompar/periodic_job.h>

#include<posixutils/timespec_utils.h>

#include<stdio.h>
#include<inttypes.h>
#include<unistd.h>

uint64_t millis_begin = -1;

uint64_t millis()
{
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	return timespec_to_milliseconds(spec);
}

uint64_t millis_now()
{
	return millis() - millis_begin;
}

void periodic_function(void* t)
{
	printf("Hello from periodic function at %"PRIu64"\n", millis_now());
}

int main()
{
	millis_begin = millis();
	printf("Test begins at %"PRIu64" with 0.5 second period\n", millis_begin);
	periodic_job* pjob = new_periodic_job(periodic_function, NULL, 500000); // start with half a second

	sleep(5);

	printf("Resuming periodic job at %"PRIu64" => %d\n", millis_now(), resume_periodic_job(pjob));

	sleep(5);

	printf("Pausing periodic job at %"PRIu64" => %d\n", millis_now(), pause_periodic_job(pjob));

	sleep(5);

	printf("Resuming periodic job at %"PRIu64" => %d\n", millis_now(), resume_periodic_job(pjob));

	sleep(5);

	printf("Updating period to 1 second at %"PRIu64" => %d\n", millis_now(), update_period_for_periodic_job(pjob, 1000000));

	sleep(5);

	printf("Updating period to 0.3 second at %"PRIu64" => %d\n", millis_now(), update_period_for_periodic_job(pjob, 300000));

	sleep(6);

	printf("Updating period to 1 second at %"PRIu64" => %d\n", millis_now(), update_period_for_periodic_job(pjob, 1000000));

	sleep(5);

	printf("Pausing periodic job at %"PRIu64" => %d\n", millis_now(), pause_periodic_job(pjob));

	sleep(5);

	printf("Single shot periodic job at %"PRIu64" => %d\n", millis_now(), pause_periodic_job(pjob));

	sleep(5);

	delete_periodic_job(pjob);
}