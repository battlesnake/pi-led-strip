#pragma once

#include "led.h"
#include "timing.h"
#include "colour.h"

struct particle
{
	float position;
	float velocity;
	float next_position;
	float next_velocity;
	float mass;
	float size;
	struct rgb colour;
	int immobile;
};

struct particles
{
	size_t num_leds;
	struct led *leds;
	struct timing *timing;
	int num_particles;
	struct particle *particles;
	double prev_time;
};

struct particles *particles_init(size_t num_leds, struct led *leds, struct timing *timing, int num_particles, float min_velocity, float max_velocity, float min_size, float max_size);
void particles_run(struct particles *this);
void particles_free(struct particles *this);
