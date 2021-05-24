#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "launch.h"
#include "colour.h"

static const float time_wavelength = -0.3;
static const float space_wavelength = 30;
static const float threshold = 0.8;

struct launch *launch_init(size_t num_leds, struct led *leds, struct timing *timing)
{
	struct launch *this = malloc(sizeof(*this));
	if (!this) {
		perror("malloc");
		return NULL;
	}
	this->num_leds = num_leds;
	this->leds = leds;
	this->timing = timing;
	return this;
}

void launch_run(struct launch *this)
{
	double time = timing_get(this->timing);
	for (size_t i = 0; i < this->num_leds; ++i) {
		struct led *led = this->leds + i;
		float space = powf(i * 1.0f / this->num_leds, 0.1f) * this->num_leds;
		float arg = sinf(2 * M_PI * (time / time_wavelength + space / space_wavelength));
		arg = arg < threshold ? 0 : powf((arg - threshold) / (1 - threshold), 8);
		led->brightness = 1;
		struct hsv hsv = {
			.h = 0.6,
			.s = 1 - powf(arg, 4),
			.v = arg
		};
		hsv2rgb(&hsv, &led->colour);
	}
}

void launch_free(struct launch *this)
{
	if (!this) {
		return;
	}
	free(this);
}
