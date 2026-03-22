#include <math.h>
#include <stdint.h>
#include <stddef.h>
#define __FPU_PRESENT 1U
#include "arm_math.h"

#include "goertzel.h"
#include "stm32f4xx.h"

#include "main.h"
#include "measure.h"

struct GoertzelResult {
    float real;       // Real part of complex bin
    float imag;       // Imag part of complex bin
    float magnitude;  // sqrt(real^2 + imag^2)
    float phase;      // atan2(imag, real) in radians
};

// Compute a single-frequency complex response using Goertzel on a uint16_t buffer.
// - buf: input samples (e.g., raw ADC codes)
// - N: number of samples
// - fs: sampling rate in Hz
// - f0: target frequency in Hz (0 < f0 < fs/2 recommended)
// - removeDC: subtract mean before processing (recommended for ADC codes)
// - useHann: apply Hann window to reduce leakage
// - amplitudeScale: if true, apply amplitude scaling (2/(N*CG)) where CG is window coherent gain
// Returns true on success; fills 'out' with real/imag/mag/phase.
goertzel_out_t goertzel_u16(
    const uint16_t* buf,
    size_t N,
    float fs,
    float f0
)
{
    goertzel_out_t out = {0};
    if (!buf || N < 3 || fs <= 0.0f || f0 <= 0.0f || f0 >= (fs * 0.5f)) return out;

    // Optional DC removal improves low-f results
    float mean = 0.0f;
    for (size_t i = 0; i < N; ++i) mean += buf[i];
    mean /= (float)N;

    float omega = 2.0f * PI * (f0 / fs);
    float c = arm_cos_f32(omega);
    float s = arm_sin_f32(omega);
    float coeff = 2.0f * c;

    float s0 = 0.0f, s1 = 0.0f, s2 = 0.0f;
    for (size_t n = 0; n < N; ++n) {
        float x = (float)buf[n] - mean;
        s0 = x + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }

    float real = s1 - s2 * c;
    float imag = s2 * s;

    // Single-sided amplitude scaling
    real *= 2.0f / (float)N;
    imag *= 2.0f / (float)N;

    out.magnitude = sqrtf(real * real + imag * imag);
    out.phase = atan2f(imag, real);
    return out;
}