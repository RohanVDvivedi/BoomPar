#ifndef STREAM_H
#define STREAM_H

typedef struct read_stream read_stream;
struct read_stream
{
	void* stream_context;

	// returns bytes read from data, (atmost data_size number of bytes will be touched)
	unsigned int (*read)(void* stream_context, void* data, unsigned int data_size);

	int (*close)(void* stream_context);

	int (*is_closed)(void* stream_context);
};

typedef struct write_stream write_stream;
struct write_stream
{
	void* stream_context;

	// returns bytes written from data (that consists of data_size number of bytes to be written)
	unsigned int (*write)(void* stream_context, const void* data, unsigned int data_size);

	int (*close)(void* stream_context);

	int (*is_closed)(void* stream_context);
};

#endif