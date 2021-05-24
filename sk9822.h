#pragma once
#include <stddef.h>

#include "led.h"

struct sk9822
{
	int fd;
	size_t num_leds;
	struct led *leds;
};

struct sk9822 *sk9822_init(const char *spidev, int speed, size_t num_leds);
int sk9822_update(struct sk9822 *this);
void sk9822_free(struct sk9822 *this);
