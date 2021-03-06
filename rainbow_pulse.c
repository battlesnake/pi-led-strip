#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "rainbow_pulse.h"
#include "colour.h"

static const float brightness_time_wavelength = -0.4f;
static const float brightness_space_wavelength = 160.f;
static const float hue_time_wavelength = 1.0f;
static const float hue_space_wavelength = 60.f;

struct rainbow_pulse *rainbow_pulse_init(size_t num_leds, struct led *leds)
{
	struct rainbow_pulse *this = malloc(sizeof(*this));
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
	this->h_time_phase = 0;
	this->s_time_phase = 0;
	return this;
fail:
	rainbow_pulse_free(this);
	return NULL;
}

static void phase_step(float *phase, float dt, float wavelength)
{
	*phase = fmodf(*phase + dt / wavelength, M_PI * 2);
}

void rainbow_pulse_run(struct rainbow_pulse *this)
{
	float dt = timing_step(&this->timing);
	phase_step(&this->h_time_phase, dt, hue_time_wavelength);
	phase_step(&this->s_time_phase, dt, brightness_time_wavelength);
	for (size_t i = 0; i < this->num_leds; ++i) {
		struct led *led = this->leds + i;
		float rainbow_space = i;
		float pulse_space = powf(i * 1.0f / this->num_leds, 1.5f) * this->num_leds;
		led->brightness = 1;
		float pulse = expf(-2000 * (sinf(
						this->s_time_phase +
						pulse_space / brightness_space_wavelength
					       ) * 0.5f + 0.5f));
		struct hsv hsv = {
			.h = sinf(this->h_time_phase) +
				rainbow_space / hue_space_wavelength,
			.s = 1 - pulse
		};
		hsv.v = (1 + 19 * pulse) / 20;
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
