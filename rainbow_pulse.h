#pragma once

#include "led.h"
#include "timing.h"

struct rainbow_pulse
{
	size_t num_leds;
	struct led *leds;
	struct timing *timing;
};

struct rainbow_pulse *rainbow_pulse_init(size_t num_leds, struct led *leds, struct timing *timing);
void rainbow_pulse_run(struct rainbow_pulse *this);
void rainbow_pulse_free(struct rainbow_pulse *this);
