#include <stdio.h>
#include <sys/time.h>
#include "helper.h"

uint64_t start_timer(void)
{
	struct timeval stime;
	uint64_t start;
	gettimeofday( &stime, NULL );
	start = stime.tv_sec * 1000 + stime.tv_usec / 1000;
	return start;
}

uint64_t check_timer(uint64_t start)
{
	struct timeval etime;
	uint64_t end;
	gettimeofday( &etime, NULL );
	end = etime.tv_sec * 1000 + etime.tv_usec / 1000;
	return (end - start);
}

