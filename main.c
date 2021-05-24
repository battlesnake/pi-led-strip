#if 0
set -xe
trap 'rm -f ~/led-animation' EXIT
gcc *.c -O2 -march=native -mtune=native -Wall -Wextra -Werror -ffunction-sections -fdata-sections -Wl,--gc-sections -flto -lm -s -o ~/led-animation
exec ~/led-animation "$@"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <getopt.h>

#include "sk9822.h"
#include "timing.h"
#include "rainbow_pulse.h"
#include "launch.h"
#include "particles.h"

#define case_fallthrough __attribute__ ((fallthrough))

static volatile int quitting = 0;

static void sigint_handler(int signo)
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
	int device_speed = 10000000;
	int num_leds = 300;

	/* Parse arguments */
	int opt;
	while ((opt = getopt(argc, argv, "hd:s:l:a:p:")) != -1) {
		switch (opt) {
		case 'd':
			device = optarg;
			break;
		case 's':
			device_speed = atoi(optarg);
			break;
		case 'l':
			num_leds = atoi(optarg);
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
		case '?':
			case_fallthrough;
		default:
invalid_arg:
			fprintf(stderr, "Invalid argument\n");
			case_fallthrough;
		case 'h':
			fprintf(stderr, "Syntax: %s [-d device] [-s device_speed] [-l num_leds] [-a { rainbow_pulse | launch | particles } ] [-p { apa102 | sk9822 } ]\n", argv[0]);
			goto fail_args;
		}
	}

	if (signal(SIGINT, sigint_handler) == SIG_ERR) {
		perror("signal(SIGINT)");
	}

	/* Create LED driver */
	void *led_state;
	int (*led_update)(void *);
	void (*led_free)(void *);
	struct led *leds;
	if (protocol == APA102 || protocol == SK9822) {
		led_update = (void *) sk9822_update;
		led_free = (void *) sk9822_free;
		led_state = sk9822_init(device, device_speed, num_leds);
		if (!led_state) {
			perror("sk9822_init");
			goto fail_led;
		}
		leds = ((struct sk9822 *) led_state)->leds;
	} else {
		perror("Unknown protocol");
		goto fail_led;
	}

	/* Create timer */
	struct timing timing;
	if (timing_init(&timing) != 0) {
		perror("timing_init");
		goto fail_timing;
	}

	/* Create animation engine */
	void *animation_state;
	int (*animation_update)(void *);
	void (*animation_free)(void *);
	if (animation_to_run == RAINBOW_PULSE) {
		animation_update = (void *) rainbow_pulse_run;
		animation_free = (void *) rainbow_pulse_free;
		animation_state = rainbow_pulse_init(num_leds, leds, &timing);
		if (!animation_state) {
			perror("rainbow_pulse_init");
			goto fail_animation;
		}
	} else if (animation_to_run == LAUNCH) {
		animation_update = (void *) launch_run;
		animation_free = (void *) launch_free;
		animation_state = launch_init(num_leds, leds, &timing);
		if (!animation_state) {
			perror("launch_init");
			goto fail_animation;
		}
	} else if (animation_to_run == PARTICLES) {
		animation_update = (void *) particles_run;
		animation_free = (void *) particles_free;
		animation_state = particles_init(num_leds, leds, &timing,
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
		if (led_update(led_state) != 0) {
			perror("led_update");
			goto fail_run;
		}
		usleep(1000);
	}

	/* Clear LEDs */
	for (int i = 0; i < num_leds; ++i) {
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
	timing_free(&timing);
fail_timing:
	led_free(&led_state);
fail_led:
fail_args:
	return ret;
}
