#include <math.h>
#include <stdint.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void goertzel_phase_i16(const int16_t *x, size_t N, float fs, float f0, float *mag, float *phase_rad);

// Optional: enable a Hann window to reduce leakage if f0 is not bin-centered
#ifndef GOERTZEL_USE_HANN
#define GOERTZEL_USE_HANN 1
#endif