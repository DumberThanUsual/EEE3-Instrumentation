#include <math.h>
#include <stdint.h>
#include <stddef.h>

#include "goertzel.h"

static inline float hann_w(size_t n, size_t N)
{
#if GOERTZEL_USE_HANN
    if (N <= 1) return 1.0f;
    return 0.5f * (1.0f - cosf(2.0f * (float)M_PI * (float)n / (float)(N - 1)));
#else
    (void)n; (void)N;
    return 1.0f;
#endif
}

// Compute magnitude and phase for a single tone using Goertzel.
// x: int16 samples, N: length, fs: sampling rate [Hz], f0: target tone [Hz]
// Returns mag (linear) and phase (radians) via out params.
// Notes:
//  - Removes DC (mean) first for stability.
//  - Use float for speed, double for more precision if needed.
void goertzel_phase_i16(const int16_t *x, size_t N, float fs, float f0, float *mag, float *phase_rad)
{
    if (!x || N < 2 || fs <= 0.0f || f0 <= 0.0f) {
        if (mag) *mag = 0.0f;
        if (phase_rad) *phase_rad = 0.0f;
        return;
    }

    volatile const float omega = 2.0f * (float)M_PI * (f0 / fs);
    const float cos_omega = cosf(omega);
    const float sin_omega = sinf(omega);
    const float coeff = 2.0f * cos_omega;

    // 1) DC removal
    double mean = 0.0;
    for (size_t i = 0; i < N; ++i) {
        mean += x[i];
    }
    mean /= (double)N;

    // 2) Goertzel recursion
    float s0 = 0.0f, s1 = 0.0f, s2 = 0.0f;
    for (size_t n = 0; n < N; ++n) {
        float xn = (float)((double)x[n] - mean);
        xn *= hann_w(n, N);
        s0 = xn + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }

    // 3) Final real/imag and outputs
    float real = s1 - s2 * cos_omega;
    float imag = s2 * sin_omega;
    float m = sqrtf(real * real + imag * imag);
    float ph = atan2f(imag, real);  // [-pi, pi]

    if (mag) *mag = m;
    if (phase_rad) *phase_rad = ph;
}

