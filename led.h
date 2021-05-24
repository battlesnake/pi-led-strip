#pragma once

#include "colour.h"

struct led
{
	/* 0..1 */
	float brightness;
	struct rgb colour;
};

const struct led LED_INIT;
