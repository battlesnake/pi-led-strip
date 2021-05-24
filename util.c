#include "util.h"

int clamp(int min, int max, int arg)
{
	return arg < min ? min : arg > max ? max : arg;
}

float clampf(float min, float max, float arg)
{
	return arg < min ? min : arg > max ? max : arg;
}

float interpf(float min, float max, float arg)
{
	return (max - min) * arg + min;
}

float interp2f(float min, float max, float arg, float argmin, float argmax)
{
	return interpf(min, max, (arg - argmin) / (argmax - argmin));
}
