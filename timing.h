#pragma once
#include <time.h>

struct timing
{
	struct timespec prev;
};

int timing_init(struct timing *this);
double timing_get(const struct timing *this);
float timing_step(struct timing *this);
