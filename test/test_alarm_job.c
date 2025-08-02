#include<boompar/alarm_job.h>

#include<posixutils/pthread_cond_utils.h>
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

uint64_t alarm_function(void* t)
{
	static uint64_t next_run_period = 0;
	printf("Hello from alarm function at %"PRIu64"\n", millis_now());
	next_run_period += 200000ULL;

	if(next_run_period > 2 * 1000000ULL)
		return BLOCKING;
	return next_run_period;
}

int main()
{
	millis_begin = millis();
	printf("Test begins at %"PRIu64"\n", millis_begin);
	alarm_job* ajob = new_alarm_job(alarm_function, NULL);

	sleep(2);

	printf("Waking up alarm job at %"PRIu64" => %d\n", millis_now(), wake_up_alarm_job(ajob));

	sleep(2);

	printf("Pausing alarm job at %"PRIu64" => %d\n", millis_now(), pause_alarm_job(ajob));

	sleep(5);

	printf("Waking up alarm job at %"PRIu64" => %d\n", millis_now(), wake_up_alarm_job(ajob));

	sleep(2);

	printf("Pausing alarm job at %"PRIu64" => %d\n", millis_now(), pause_alarm_job(ajob));
	wait_for_pause_or_shutdown_of_alarm_job(ajob);

	sleep(5);

	printf("Shutdown alarm job at %"PRIu64" => %d\n", millis_now(), shutdown_alarm_job(ajob));
	wait_for_pause_or_shutdown_of_alarm_job(ajob);

	sleep(5);

	delete_alarm_job(ajob);
}