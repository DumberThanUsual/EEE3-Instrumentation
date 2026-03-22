#include "stm32f446xx.h"
#include "signal.h"

#include "dac.h"
#include "tim.h"

#include "FreeRTOS.h"

#include "task.h"
#include "queue.h"

#include <math.h>
#include "arm_math.h"

typedef struct {
    uint32_t phase;          // 32-bit phase accumulator
    uint32_t phase_inc;      // 32-bit phase increment
    int16_t  amp_counts;     // amplitude in DAC counts (0..2047 typical)
    uint16_t buffer[2*HALF_BUF_LEN];
} dds_q15_t;

static dds_q15_t dds;

static int16_t sin_q15[LUT_QTR];

static void dds_build_quarter_lut(void)
{
    for (uint32_t i = 0; i < LUT_QTR; i++) {
        double th = (double)i * (PI/2.0) / (double)(LUT_QTR - 1u);
        int32_t s = (int32_t)lround(sin(th) * 32767.0); // Q15
        if (s > 32767) s = 32767; 
        if (s < 0) s = 0;
        sin_q15[i] = (int16_t)s;
    }
}

// Set frequency: phase_inc = f0 * 2^32 / fs
static inline void signal_set_freq_fixed(uint32_t f0_hz)
{
    double pincr = ((double)f0_hz * (double)(1ULL<<32)) / (double)DAC_FS;
    if (pincr < 0.0) pincr = 0.0; 
    if (pincr > 4294967295.0) pincr = 4294967295.0;
    dds.phase_inc = (uint32_t)(pincr + 0.5);
}

// Set amplitude in DAC counts (peak)
static inline void signal_set_amp_counts_fixed(uint16_t amp_counts)
{
    if (amp_counts > 2047) amp_counts = 2047;
    dds.amp_counts = (int16_t)amp_counts;
}

static inline int16_t dds_next_sine_q15(void)
{
    // Advance phase and decode quadrant + quarter index with FRAC_BITS of fraction
    uint32_t ph   = dds.phase;
    dds.phase    += dds.phase_inc;

    const uint32_t q_bits   = 2u;                          // top 2 bits = quadrant
    const uint32_t q_shift  = 32u - q_bits;
    const uint32_t q        = ph >> q_shift;               // 0..3

    // Bits used for quarter index (LUT_BITS-2) + FRAC_BITS of fraction
    const uint32_t uf_bits  = (LUT_BITS - 2u) + FRAC_BITS;
    const uint32_t uf_shift = 32u - q_bits - uf_bits;

    uint32_t u = (ph >> uf_shift) & ((1u << uf_bits) - 1u);               // 0.. (Q<<FRAC_BITS) - 1
    const uint32_t U_MAX = (LUT_QTR << FRAC_BITS) - 1u;                   // last fractional index in quarter

    // Mirror u in quadrants 1 and 3
    if (q & 1u) { // q==1 or q==3
        u = U_MAX - u;
    }

    // Integer/fractional split
    uint32_t pos  = u >> FRAC_BITS;                                       // 0.. Q-1
    uint32_t frac = u & ((1u << FRAC_BITS) - 1u);                         // 0.. (2^FRAC_BITS -1)

    // Clamp pos2 at end of quarter (no wrap)
    uint32_t pos2 = (pos + 1u < LUT_QTR) ? (pos + 1u) : pos;

    // Linear interpolation within quarter
    int32_t a = sin_q15[pos];
    int32_t b = sin_q15[pos2];
    int32_t y = a + (((b - a) * (int32_t)frac) >> FRAC_BITS);             // Q15

    // Apply sign for quadrants 2 and 3
    if (q >= 2u) 
        y = -y;

    // Clamp to Q15 range
    if (y > 32767) 
        y = 32767;

    if (y < -32768) 
        y = -32768;
    return (int16_t)y;
}

// Convert Q15 sine sample to DAC counts centered at DAC_MID using amp_counts
static inline uint16_t q15_to_dac(int16_t s_q15)
{
    // sample_counts = DAC_MID + (amp_counts * s_q15) >> 15
    int32_t v = (int32_t)DAC_MID + (((int32_t)dds.amp_counts * (int32_t)s_q15) >> 15);
    if (v < 0) 
        v = 0; 

    if (v > 4095) 
        v = 4095;
    return (uint16_t)v;
}

static inline void signal_fill_half(uint16_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        int16_t s = dds_next_sine_q15();
        buf[i] = q15_to_dac(s);
    }
}

void signal_stop()
{
    HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, DAC_MID);
}

void signal_task(void *pvParameters)
{
    for (;;) {
        uint32_t g_half = 0;
        xTaskNotifyWait(0x00, UINT32_MAX, &g_half, portMAX_DELAY);

        uint16_t *buf = g_half ? dds.buffer : (dds.buffer + HALF_BUF_LEN);
        signal_fill_half(buf, HALF_BUF_LEN);
    }
}


void signal_start(uint32_t frequency_hz, uint16_t amplitude_counts)
{
    signal_stop();

    dds.phase = 0;
    signal_set_freq_fixed(frequency_hz);
    signal_set_amp_counts_fixed(amplitude_counts);

    // Prefill BOTH halves
    signal_fill_half(dds.buffer, HALF_BUF_LEN);
    signal_fill_half(dds.buffer + HALF_BUF_LEN, HALF_BUF_LEN);

    // Start DMA (circular, half/full interrupts enabled in Cube)
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1,
                      (uint32_t*)dds.buffer, 2*HALF_BUF_LEN, DAC_ALIGN_12B_R);
}

BaseType_t signal_init()
{
    dds_build_quarter_lut();

    HAL_TIM_Base_Start(&htim2);

    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, DAC_MID);
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, DAC_MID);

    return xTaskCreate(
        signal_task,
        "signal_task",
        1024,
        NULL,
        1,
        &signal_task_handle
    );
}

BaseType_t signal_DMA_Handler(uint32_t half)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xHigherPriorityTaskWoken = xTaskNotifyFromISR(signal_task_handle, half, eSetValueWithOverwrite, NULL);
    return xHigherPriorityTaskWoken;
}