#pragma once

int clamp(int min, int max, int arg);
float clampf(float min, float max, float arg);

float interpf(float min, float max, float arg);
float interp2f(float min, float max, float arg, float argmin, float argmax);
