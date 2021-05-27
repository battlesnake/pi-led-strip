#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "launch.h"
#include "colour.h"

static const float time_wavelength = -0.8;
static const float space_wavelength = 40;
static const float threshold = 0.8;

struct launch *launch_init(size_t num_leds, struct led *leds)
{
	struct launch *this = malloc(sizeof(*this));
	if (!this) {
		perror("malloc");
		goto fail;
	}
	memset(this, 0, sizeof(*this));
	if (timing_init(&this->timing) != 0) {
		perror("timing_init");
		goto fail;
	}
	this->num_leds = num_leds;
	this->leds = leds;
	this->time_phase = 0;
	return this;
fail:
	launch_free(this);
	return NULL;
}

static void step_phase(float *phase, float dt, float wavelength)
{
	*phase = fmodf(*phase + dt / wavelength, M_PI * 2);
}

void launch_run(struct launch *this)
{
	float dt = timing_step(&this->timing);
	step_phase(&this->time_phase, dt, time_wavelength);
	for (size_t i = 0; i < this->num_leds; ++i) {
		struct led *led = this->leds + i;
		float space = powf(i * 1.0f / this->num_leds, 0.1f) * this->num_leds;
		float arg = sinf(2 * M_PI * (this->time_phase + space / space_wavelength));
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
