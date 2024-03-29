#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <getopt.h>

#include "sk9822.h"
#include "rainbow_pulse.h"
#include "launch.h"
#include "particles.h"
#include "mirror.h"

static volatile int quitting = 0;

static void exit_signal_handler(int signo)
{
	(void) signo;
	quitting = 1;
}

enum animation
{
	RAINBOW_PULSE = 0,
	LAUNCH = 1,
	PARTICLES = 2,
};

enum protocol
{
	APA102 = 0,
	SK9822 = 1
};

int main(int argc, char *argv[])
{
	int ret = 1;

	enum animation animation_to_run = RAINBOW_PULSE;
	enum protocol protocol = APA102;
	const char *device = "/dev/spidev0.0";
	int device_speed = 1000000;
	int real_num_leds = 288;
	int time_step_us = 10000;
	bool mirror = false;
	float brightness = 1;

	/* Parse arguments */
	int opt;
	while ((opt = getopt(argc, argv, "hd:s:l:a:p:t:mb:")) != -1) {
		switch (opt) {
		case 'd':
			device = optarg;
			break;
		case 's':
			device_speed = atoi(optarg);
			break;
		case 'l':
			real_num_leds = atoi(optarg);
			break;
		case 'a':
			if (strcasecmp(optarg, "rainbow_pulse") == 0) {
				animation_to_run = RAINBOW_PULSE;
			} else if (strcasecmp(optarg, "launch") == 0) {
				animation_to_run = LAUNCH;
			} else if (strcasecmp(optarg, "particles") == 0) {
				animation_to_run = PARTICLES;
			} else {
				goto invalid_arg;
			}
			break;
		case 'p':
			if (strcasecmp(optarg, "apa102") == 0) {
				protocol = APA102;
			} else if (strcasecmp(optarg, "sk9822") == 0) {
				protocol = SK9822;
			} else {
				goto invalid_arg;
			}
			break;
		case 't':
			time_step_us = atof(optarg) * 1000;
			break;
		case 'm':
			mirror = true;
			break;
		case 'b':
			brightness = atof(optarg);
			break;
		case '?':
		default:
invalid_arg:
			fprintf(stderr, "Invalid argument\n");
			goto help;
		case 'h':
help:
			fprintf(stderr, "Syntax: %s"
					"\n\t [ -d device ]"
					"\n\t [ -s device_speed ]"
					"\n\t [ -l effective_num_leds ]"
					"\n\t [ -a { rainbow_pulse | launch | particles } ]"
					"\n\t [ -p { apa102 | sk9822 } ]"
					"\n\t [ -t time_step_ms ]"
					"\n\t [ -m ]  <--mirror"
					"\n\t [ -b brightness ]"
					"\n", argv[0]);
			goto fail_args;
		}
	}

	if (signal(SIGINT, exit_signal_handler) == SIG_ERR) {
		perror("signal");
	}

	if (signal(SIGTERM, exit_signal_handler) == SIG_ERR) {
		perror("signal");
	}

	int effective_num_leds = real_num_leds;
	if (mirror) {
		effective_num_leds /= 2;
	}

	/* Create LED driver */
	void *led_state;
	int (*led_update)(void *);
	void (*led_free)(void *);
	struct led *leds;
	if (protocol == APA102 || protocol == SK9822) {
		led_update = (void *) sk9822_update;
		led_free = (void *) sk9822_free;
		led_state = sk9822_init(device, device_speed, real_num_leds);
		if (!led_state) {
			perror("sk9822_init");
			goto fail_led;
		}
		leds = ((struct sk9822 *) led_state)->leds;
	} else {
		perror("Unknown protocol");
		goto fail_led;
	}

	/* Create animation engine */
	void *animation_state;
	int (*animation_update)(void *);
	void (*animation_free)(void *);
	if (animation_to_run == RAINBOW_PULSE) {
		animation_update = (void *) rainbow_pulse_run;
		animation_free = (void *) rainbow_pulse_free;
		animation_state = rainbow_pulse_init(effective_num_leds, leds);
		if (!animation_state) {
			perror("rainbow_pulse_init");
			goto fail_animation;
		}
	} else if (animation_to_run == LAUNCH) {
		animation_update = (void *) launch_run;
		animation_free = (void *) launch_free;
		animation_state = launch_init(effective_num_leds, leds);
		if (!animation_state) {
			perror("launch_init");
			goto fail_animation;
		}
	} else if (animation_to_run == PARTICLES) {
		animation_update = (void *) particles_run;
		animation_free = (void *) particles_free;
		animation_state = particles_init(effective_num_leds, leds,
				8, /* #particles */
				30, 50, /* velocity */
				1, 5); /* size */
		if (!animation_state) {
			perror("particles_init");
			goto fail_animation;
		}
	} else {
		perror("Unknown animation");
		goto fail_animation;
	}

	/* Main loop */
	while (!quitting) {
		animation_update(animation_state);
		if (mirror) {
			mirror_leds(real_num_leds, leds);
		}
		for (struct led *led = leds, *out = leds + real_num_leds; led != out; ++led) {
			led->brightness *= brightness;
		}
		if (led_update(led_state) != 0) {
			perror("led_update");
			goto fail_run;
		}
		usleep(time_step_us);
	}

	/* Clear LEDs */
	fprintf(stderr, "Clearing LEDs\n");
	for (int it = 0; it < 25; ++it) {
		for (int i = 0; i < real_num_leds; ++i) {
			leds[i].brightness *= 0.7;
		}
		if (led_update(led_state) != 0) {
			perror("led_update");
			goto fail_run;
		}
		usleep(time_step_us);
	}
	for (int i = 0; i < real_num_leds; ++i) {
		leds[i] = LED_INIT;
	}
	if (led_update(led_state) != 0) {
		perror("led_update");
		goto fail_run;
	}

	ret = 0;

	/* Clean up */
fail_run:
	animation_free(animation_state);
fail_animation:
	led_free(led_state);
fail_led:
fail_args:
	return ret;
}
