#include "mirror.h"

void mirror_leds(int num_leds, struct led *leds)
{
	struct led *out = leds + num_leds - 1;
	struct led *in = leds;
	while (in < out) {
		*out = *in;
		++in;
		--out;
	}
}
