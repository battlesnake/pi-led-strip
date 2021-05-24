#pragma once

#include "led.h"
#include "timing.h"

struct launch
{
	size_t num_leds;
	struct led *leds;
	struct timing *timing;
};

struct launch *launch_init(size_t num_leds, struct led *leds, struct timing *timing);
void launch_run(struct launch *this);
void launch_free(struct launch *this);
