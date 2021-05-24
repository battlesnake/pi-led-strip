#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "rainbow_pulse.h"
#include "colour.h"

static const float brightness_time_wavelength = -0.2f;
static const float brightness_space_wavelength = 800;
static const float hue_time_wavelength = 0.5f;
static const float hue_space_wavelength = 1000;

struct rainbow_pulse *rainbow_pulse_init(size_t num_leds, struct led *leds, struct timing *timing)
{
	struct rainbow_pulse *this = malloc(sizeof(*this));
	if (!this) {
		perror("malloc");
		return NULL;
	}
	this->num_leds = num_leds;
	this->leds = leds;
	this->timing = timing;
	return this;
}

void rainbow_pulse_run(struct rainbow_pulse *this)
{
	double time = timing_get(this->timing);
	for (size_t i = 0; i < this->num_leds; ++i) {
		struct led *led = this->leds + i;
		float space = powf(i * 1.0f, 1.5f);
		led->brightness = 1;
		struct hsv hsv = {
			.h = sinf(time / hue_time_wavelength) + space / hue_space_wavelength,
			.s = 1 - expf(-800 * (sinf(time / brightness_time_wavelength + space / brightness_space_wavelength) * 0.5f + 0.5f))
		};
		hsv.v = (20 - 19 * hsv.s) / 20;
		hsv2rgb(&hsv, &led->colour);
	}
}

void rainbow_pulse_free(struct rainbow_pulse *this)
{
	if (!this) {
		return;
	}
	free(this);
}
