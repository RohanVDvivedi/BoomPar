#ifndef YIELD_REASON_H
#define YIELD_REASON_H

typedef enum yield_reason_type yield_reason_type;
struct yield_reason_type
{
	TIMED_WAIT,
	STREAM_READ_WAIT,
	STREAM_WRITE_WAIT,
};

typedef struct yield_reason yield_reason;
struct yield_reason
{
	yield_reason_type type;
	union
	{
		struct
		{
			stream* wait_for;
			unsigned int byte_count;
		};
		struct
		{
			unsigned long long int microseconds_to_wait;
		};
	};
};

#endif