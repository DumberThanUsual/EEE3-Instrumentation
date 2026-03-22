#ifndef GOERTZEL_H
#define GOERTZEL_H

#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx.h"

typedef struct {
    float magnitude; // peak amplitude in ADC counts (centered)
    float phase;       // radians in [-pi, pi]
} goertzel_out_t;

goertzel_out_t goertzel_u16(
    const uint16_t* buf,
    size_t N,
    float fs,
    float f0
);

#endif