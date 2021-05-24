#pragma once
#include <time.h>

struct timing
{
	struct timespec start;
};

int timing_init(struct timing *this);
double timing_get(struct timing *this);
void timing_free(struct timing *this);
