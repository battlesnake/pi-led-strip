#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "particles.h"
#include "colour.h"
#include "util.h"

#define FOREACH_CONST_PARTICLE(it) for (struct particle *it = this->particles, *it##_end = it + this->num_particles; it != it##_end; ++it)
#define FOREACH_PARTICLE(it) for (struct particle *it = this->particles, *it##_end = it + this->num_particles; it != it##_end; ++it)

static float rand_range(float min, float max)
{
	const int rm = 0x7fffffffl;
	int r = rand() & rm;
	return interp2f(min, max, r, 0, rm);
}

static float calc_energy(const struct particles *this)
{
	float result = 0;
	FOREACH_CONST_PARTICLE(p) {
		result += p->mass * p->velocity * p->velocity;
	}
	return result / 2;
}

struct particles *particles_init(size_t num_leds, struct led *leds, int num_particles, float min_velocity, float max_velocity, float min_size, float max_size)
{
	struct particles *this = malloc(sizeof(*this));
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
	this->num_particles = num_particles + 2;
	this->particles = malloc(sizeof(*this->particles) * this->num_particles);
	if (!this->particles) {
		perror("malloc");
		goto fail;
	}
	/* Initialise particles (excluding walls at end) */
	for (int i = 1; i < this->num_particles - 1; ++i) {
		struct particle *p = &this->particles[i];
		p->size = rand_range(min_size, max_size);
		p->mass = p->size;
		p->position = interp2f(1, this->num_leds - 2, i, 0, this->num_particles - 1);
		p->velocity = rand_range(min_velocity, max_velocity) * (rand_range(0, 2) < 1 ? -1 : +1);
		struct hsv hsv = {
			.h = interp2f(0, 1, i, 1, this->num_particles - 1), // -1 instead of -2, we don't want to end on same colour as start
			.s = 1,
			.v = 1
		};
		hsv2rgb(&hsv, &p->colour);
		p->immobile = 0;
	}
	/* Left wall */
	{
		struct particle *p = &this->particles[0];
		p->size = 1;
		p->mass = 1e9;
		p->position = 0;
		p->velocity = 0;
		p->colour = white;
		p->immobile = 1;
	}
	/* Right wall */
	{
		struct particle *p = &this->particles[this->num_particles - 1];
		p->size = 1;
		p->mass = 1e9;
		p->position = this->num_leds - 1;
		p->velocity = 0;
		p->colour = white;
		p->immobile = 1;
	}
	this->total_energy = calc_energy(this);
	return this;
fail:
	particles_free(this);
	return NULL;
}

static float collision_post_velocity(
		const struct particle *p,
		const struct particle *q)
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
	if (p->immobile) {
		return u1;
	} else if (q->immobile) {
		return 2 * u2 - u1;
	} else {
		return ((m1 - m2) * u1 + 2 * m2 * u2) / (m1 + m2);
	}
}

static void find_first_collision(
		const struct particles *this,
		const struct particle *p,
		struct particle **collison_particle,
		float *collision_time,
		const struct particle *prev_a,
		const struct particle *prev_b)
{
	*collison_particle = NULL;
	/*
	 * Find earliest collision in time-step for particle *p
	 * that is not the same two particles as the previous collision.
	 * Return other particle involved and time of collision
	 */
	FOREACH_CONST_PARTICLE(q) {
		if (p == q) {
			continue;
		}
		if ((p == prev_a && q == prev_b) || (p == prev_b && q == prev_a)) {
			continue;
		}
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

static void get_next_collision(
		const struct particles *this,
		struct particle **a,
		struct particle **b,
		float *dt,
		const struct particle *prev_a,
		const struct particle *prev_b)
{
	FOREACH_CONST_PARTICLE(p) {
		if (p->immobile) {
			continue;
		}
		/* Find earliest collision during time-step */
		struct particle *cp = NULL;
		find_first_collision(this, p, &cp, dt, prev_a, prev_b);
		if (cp) {
			*a = p;
			*b = cp;
		}
	}
}

static void propagate(struct particle *p, float dt)
{
	/* Propagate motion */
	p->next_position = p->position + dt * p->velocity;
	p->next_velocity = p->velocity;
}

/* Applies collision to the first particle (*p) */
static void collide_one(struct particle *p, const struct particle *q)
{
	/* Set post-collision velocity */
	p->next_velocity = collision_post_velocity(p, q);
}

static void commit(struct particles *this)
{
	/* Apply new kinematics */
	FOREACH_PARTICLE(p) {
		if (p->immobile) {
			continue;
		}
		p->position = p->next_position;
		p->velocity = p->next_velocity;
	}
}

void particles_physics(struct particles *this, float dt)
{
	/* Used to prevent rounding error causing the same collision to occur ad infinitum */
	struct particle *prev_a = NULL;
	struct particle *prev_b = NULL;
	do {
		struct particle *a = NULL;
		struct particle *b = NULL;
		float dt_partial = dt;
		/* Get time and particles involved in next collision */
		get_next_collision(this, &a, &b, &dt_partial, prev_a, prev_b);
		/* Propagate all particles to time of next collision */
		FOREACH_PARTICLE(p) {
			propagate(p, dt_partial);
		}
		/* Apply collision physics to relevant particles */
		if (a) {
			collide_one(a, b);
			collide_one(b, a);
		}
		/* Commit new kinematics */
		commit(this);
		/* Remove time we already propagated from remaining time */
		dt -= dt_partial;
		/* Store collision particles */
		prev_a = a;
		prev_b = b;
		/* No collision?  We've propagated the full time-step, break */
	} while (prev_a);
}

static void draw_gaussian(struct particles *this, float mean, float sigma, float halfwidth, const struct rgb *colour, float alpha)
{
	/* Blend colour bar in using Gaussian profile for alpha over bar length */
	for (int x = clamp(0, this->num_leds - 1, mean - halfwidth), end = clamp(0, this->num_leds - 1, mean + halfwidth); x <= end; ++x) {
		float arg = (x - mean) / sigma;
		float value = expf(-1 * arg * arg);
		rgb_add(&this->leds[x].colour, colour, alpha * value);
	}
}

void particles_render(struct particles *this)
{
	/* Draw all particles */
	for (struct led *it = this->leds, *end = it + this->num_leds; it != end; ++it) {
		it->brightness = 1;
		it->colour = black;
	}
	FOREACH_CONST_PARTICLE(p) {
		draw_gaussian(this, p->position, p->size / 2, p->size, &p->colour, 1);
	}
}

void conserve_energy(struct particles *this)
{
	/*
	 * We enforce conservation of energy in order to avoid rounding errors
	 * in propagation from making things go crazy over time.
	 *
	 * This correction does not conserve momentum.
	 */
	/* Spread the deficit over all particles */
	float deficit = (this->total_energy - calc_energy(this)) / this->num_particles;
	FOREACH_PARTICLE(p) {
		if (p->immobile) {
			/*
			 * We won't balance energy fully due to this, in a single call,
			 * but since we call this for each iteration, we should tend towards conservation
			 */
			continue;
		}
		/*
		 * 1/2 m u^2 + e = 1/2 m v^2
		 * v^2 = u^2 + 2e/m
		 * v = sqrt(u^2 + 2e/m)
		 * v = u sqrt(1 + 2e / [m u^2])
		 */
		p->velocity *= sqrtf(1 + 2 * deficit / (p->mass * p->velocity * p->velocity));
	}
}

void particles_run(struct particles *this)
{
	/* Propagate and render */
	float dt = timing_step(&this->timing);
	particles_physics(this, dt);
	particles_render(this);
	conserve_energy(this);
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
