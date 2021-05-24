#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "particles.h"
#include "colour.h"
#include "util.h"

static float rand_range(float min, float max)
{
	return interp2f(min, max, rand(), 0, RAND_MAX);
}

struct particles *particles_init(size_t num_leds, struct led *leds, struct timing *timing, int num_particles, float min_velocity, float max_velocity, float min_size, float max_size)
{
	struct particles *this = malloc(sizeof(*this));
	if (!this) {
		perror("malloc");
		goto fail;
	}
	memset(this, 0, sizeof(*this));
	this->num_leds = num_leds;
	this->leds = leds;
	this->timing = timing;
	this->prev_time = timing_get(timing);
	this->num_particles = num_particles + 2;
	this->particles = malloc(sizeof(*this->particles) * this->num_particles);
	if (!this->particles) {
		perror("malloc");
		goto fail;
	}
	for (int i = 1; i < this->num_particles - 1; ++i) {
		struct particle *p = &this->particles[i];
		p->size = rand_range(min_size, max_size);
		p->mass = p->size;
		p->position = interp2f(1, this->num_leds - 2, i, 0, this->num_particles - 1);
		p->velocity = rand_range(min_velocity, max_velocity) * (rand_range(0, 2) < 1 ? -1 : +1);
		struct hsv hsv = {
			.h = interp2f(0, 1, i, 1, this->num_particles - 2),
			.s = 1,
			.v = 1
		};
		hsv2rgb(&hsv, &p->colour);
		p->immobile = 0;
	}
	/* Left wall */
	{
		struct particle *p = &this->particles[0];
		p->size = 0;
		p->mass = 1e9;
		p->position = 0;
		p->velocity = 0;
		p->colour = black;
		p->immobile = 1;
	}
	/* Right wall */
	{
		struct particle *p = &this->particles[this->num_particles - 1];
		p->size = 0;
		p->mass = 1e9;
		p->position = this->num_leds - 1;
		p->velocity = 0;
		p->colour = black;
		p->immobile = 1;
	}
	return this;
fail:
	particles_free(this);
	return NULL;
}

static float collision_post_velocity(const struct particle *p, const struct particle *q)
{
	/*
	 * Conserve momentum and also energy
	 *   sum_i(m_i . u_i)   = sum_i(m_i . v_i)
	 *   sum_i(m_i . u_i^2) = sum_i(m_i . v_i^2)
	 *
	 * v1 = [(m1 - m2)u1 + 2.m2.u2] / (m1 + m2)
	 * v2 = [(m2 - m1)u2 + 2.m1.u1] / (m1 + m2)
	 */
	const float m1 = p->mass;
	const float m2 = q->mass;
	const float u1 = p->velocity;
	const float u2 = q->velocity;
	if (q->immobile) {
		return 2 * u2 - u1;
	} else {
		return ((m1 - m2) * u1 + 2 * m2 * u2) / (m1 + m2);
	}
}

static void find_first_collision(struct particles *this, int index, const struct particle **collison_particle, float *collision_time)
{
	const struct particle *p = &this->particles[index];
	*collison_particle = NULL;
	/* Time initially specifies latest time collision will be returned for */
	for (int i = 0; i < this->num_particles; ++i) {
		if (i == index) {
			continue;
		}
		const struct particle *q = &this->particles[i];
		/*
		 * Calculate collision time
		 *      r_p + t.v_p = r_q + t.v_q
		 *  .'. t = (r_q - r_p) / (v_p - v_q)
		 */
		int sign = p->position < q->position ? 1 : -1;
		float r1 = p->position + sign * p->size / 2;
		float r2 = q->position - sign * q->size / 2;
		float v1 = p->velocity;
		float v2 = q->velocity;
		float t = (r2 - r1) / (v1 - v2);
		if (isfinite(t) && t >= 0 && t <= *collision_time) {
			*collison_particle = q;
			*collision_time = t;
		}
	}
}

void particles_physics(struct particles *this, double dt)
{
	/* Calculate changes */
	for (int i = 0; i < this->num_particles - 1; ++i) {
		struct particle *p = &this->particles[i];
		if (p->immobile) {
			continue;
		}
		/* Find earliest collision during time-step */
		const struct particle *cp = NULL;
		float ct = dt;
		find_first_collision(this, i, &cp, &ct);
		/* Propagate */
		p->next_velocity = p->velocity;
		p->next_position = p->position;
		if (cp != NULL) {
			/* Calculate post-collision velocity */
			p->next_velocity = collision_post_velocity(p, cp);
			/* Propagate */
			p->next_position += p->velocity * ct + p->next_velocity * (dt - ct);
		} else {
			/* Propagate */
			p->next_position += p->velocity * dt;
		}
	}
	/* Commit */
	const struct particle *pl = &this->particles[0];
	const struct particle *pr = &this->particles[this->num_particles - 1];
	for (int i = 0; i < this->num_particles - 1; ++i) {
		struct particle *p = &this->particles[i];
		if (p->immobile) {
			continue;
		}
		p->position = clampf(pl->position + p->size / 2 + 1e-3, pr->position - p->size / 2 - 1e-3, p->next_position);
		p->velocity = p->next_velocity;
		if (fabsf(p->velocity) < 0.1f) {
			//p->velocity = rand_range(-0.1f, 0.1f);
		}
	}
}

static void draw_gaussian(struct particles *this, float mean, float sigma, float halfwidth, const struct rgb *colour, float alpha)
{
	for (int x = clamp(0, this->num_leds - 1, mean - halfwidth), end = clamp(0, this->num_leds - 1, mean + halfwidth); x <= end; ++x) {
		float arg = (x - mean) / sigma;
		float value = expf(-1 * arg * arg);
		rgb_add(&this->leds[x].colour, colour, alpha * value);
	}
}

void particles_render(struct particles *this)
{
	for (struct led *it = this->leds, *end = it + this->num_leds; it != end; ++it) {
		it->brightness = 1;
		it->colour = black;
	}
	for (const struct particle *p = this->particles, *end = p + this->num_particles; p != end; ++p) {
		draw_gaussian(this, p->position, p->size / 2, p->size, &p->colour, 1);
	}
}

void particles_run(struct particles *this)
{
	double now = timing_get(this->timing);
	double dt = now - this->prev_time;
	this->prev_time = now;
	particles_physics(this, dt);
	particles_render(this);
}

void particles_free(struct particles *this)
{
	if (!this) {
		return;
	}
	if (this->particles) {
		free(this->particles);
	}
	free(this);
}
