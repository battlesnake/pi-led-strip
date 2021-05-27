#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "timing.h"

int timing_init(struct timing *this)
{
	if (clock_gettime(CLOCK_MONOTONIC, &this->prev) != 0) {
		perror("failed to get time");
		return -1;
	}
	return 0;
}

static float timespec_sub(const struct timespec *a, const struct timespec *b)
{
	return (a->tv_sec - b->tv_sec) + 
		((int32_t) a->tv_nsec - (int32_t) b->tv_nsec) * 1e-9f;
}

double timing_get(const struct timing *this)
{
	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
		perror("failed to get time");
		return NAN;
	}
	return timespec_sub(&now, &this->prev);
}

float timing_step(struct timing *this)
{
	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
		perror("failed to get time");
		return NAN;
	}
	float dt = timespec_sub(&now, &this->prev);
	this->prev = now;
	return dt;
}
