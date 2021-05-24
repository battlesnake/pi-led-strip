#include <math.h>

#include "util.h"
#include "colour.h"

void hsv2rgb(const struct hsv *hsv, struct rgb *rgb)
{
	float h = (hsv->h - floorf(hsv->h)) * 6;
	float s = clampf(0, 1, hsv->s);
	float v = clampf(0, 1, hsv->v);
	int sector = h;
	float angle = h - sector;
	float p = v * (1.0f - s);
	float q = v * (1.0f - (s * angle));
	float t = v * (1.0f - (s * (1.0f - angle)));
	switch(sector) {
		case 0:
			rgb->r = v;
			rgb->g = t;
			rgb->b = p;
			break;
		case 1:
			rgb->r = q;
			rgb->g = v;
			rgb->b = p;
			break;
		case 2:
			rgb->r = p;
			rgb->g = v;
			rgb->b = t;
			break;

		case 3:
			rgb->r = p;
			rgb->g = q;
			rgb->b = v;
			break;
		case 4:
			rgb->r = t;
			rgb->g = p;
			rgb->b = v;
			break;
		case 5:
		default:
			rgb->r = v;
			rgb->g = p;
			rgb->b = q;
			break;
	}
}

void rgb_add(struct rgb *a, const struct rgb *b, float amount)
{
	a->r += b->r * amount;
	a->g += b->g * amount;
	a->b += b->b * amount;
}

const struct rgb black = { .r = 0, .g = 0, .b = 0 };

const struct rgb white = { .r = 1, .g = 1, .b = 1 };
