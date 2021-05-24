#include <stdio.h>

#include "timing.h"

int timing_init(struct timing *this)
{
	if (clock_gettime(CLOCK_MONOTONIC, &this->start) != 0) {
		perror("failed to get time");
		return -1;
	}
	return 0;
}

double timing_get(struct timing *this)
{
	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
		perror("failed to get time");
		return -1;
	}
	return (now.tv_sec - this->start.tv_sec) + 1e-9 * (now.tv_nsec - this->start.tv_nsec);
}

void timing_free(struct timing *this)
{
	(void) this;
	/* Nothing */
}
