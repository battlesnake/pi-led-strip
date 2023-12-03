#pragma once

struct rgb
{
	float r;
	float g;
	float b;
};

struct hsv
{
	float h;
	float s;
	float v;
};

void hsv2rgb(const struct hsv *hsv, struct rgb *rgb);

void rgb_add(struct rgb *a, const struct rgb *b, float amount);

extern const struct rgb black;
extern const struct rgb white;
