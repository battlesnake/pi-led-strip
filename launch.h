#pragma once

#include "led.h"
#include "timing.h"

struct launch
{
	size_t num_leds;
	struct led *leds;
	struct timing timing;
	float time_phase;
};

struct launch *launch_init(size_t num_leds, struct led *leds);
void launch_run(struct launch *this);
void launch_free(struct launch *this);
