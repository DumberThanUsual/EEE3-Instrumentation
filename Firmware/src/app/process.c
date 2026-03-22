#include "main.h"
#include "measure.h"
#include "process.h"

#include "math.h"
#include "arm_math.h"

#include "stm32f446xx.h"
#include "usart.h"
#include "goertzel.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "math.h"
#include "signal.h"
#include "adc.h"
#include "dac.h"
#include <stdio.h>

#define ADC_MID 2048
#define AUTORANGE_UP 1400
#define AUTORANGE_DOWN 400

#define VERBOSE

uint8_t rerange = 0;

// Wrap phase to [-pi, pi]
static inline float wrap_pi(float x)
{
    while (x > (float)PI)  x -= 2.0f * (float)PI;
    while (x < -(float)PI) x += 2.0f * (float)PI;
    return x;
}

static void unpack_channel(const uint32_t *w, uint16_t *dst, uint32_t N, int upper)
{
    for (uint32_t i=0;i<N;i++) {
        dst[i] = upper ? (uint16_t)(w[i] >> 16) : (uint16_t)(w[i] & 0xFFFF);
        //uint32_t u = ((uint32_t)dst[i] * 65535u + 2047u) / 4095u;
        //dst[i] = (q15_t)((int32_t)u - 32768);
    }
}
uint16_t tmp[4096];

uint8_t voltage_range_up()
{
    xSemaphoreTake(setup.mutex, portMAX_DELAY);

    if (!setup.auto_voltage_gain) {
        xSemaphoreGive(setup.mutex);
        return 0;
    }

    measurement_voltage_gain_t current = setup.voltage_gain;

    switch(current) {
    case MV_x0_25:
        xSemaphoreGive(setup.mutex);
        return 0;
        break;
    case MV_x1:
        setup.voltage_gain = MV_x0_25;
        break;
    case MV_x2_5:
        setup.voltage_gain = MV_x1;
        break;
    case MV_x10:
        setup.voltage_gain = MV_x2_5;
        break;
    case MV_x25:
        setup.voltage_gain = MV_x10;
        break;
    case MV_x100:
        setup.voltage_gain = MV_x25;
        break;
    }

    xSemaphoreGive(setup.mutex);

    return 1;
}

uint8_t voltage_range_down()
{
    xSemaphoreTake(setup.mutex, portMAX_DELAY);

    if (!setup.auto_voltage_gain) {
        xSemaphoreGive(setup.mutex);
        return 0;
    }

    measurement_voltage_gain_t current = setup.voltage_gain;

    switch(current) {
    case MV_x0_25:
        setup.voltage_gain = MV_x1;
        break;
    case MV_x1:
        setup.voltage_gain = MV_x2_5;
        break;
    case MV_x2_5:
        setup.voltage_gain = MV_x10;
        break;
    case MV_x10:
        setup.voltage_gain = MV_x25;
        break;
    case MV_x25:
        setup.voltage_gain = MV_x100;
        break;
    case MV_x100:
        xSemaphoreGive(setup.mutex);
        return 0;
        break;
    }

    xSemaphoreGive(setup.mutex);

    return 1;
}

uint8_t current_range_up()
{
    xSemaphoreTake(setup.mutex, portMAX_DELAY);

    if (!setup.auto_current_gain) {
        xSemaphoreGive(setup.mutex);
        return 0;
    }

    measurement_current_gain_t current = setup.current_gain;

    switch(current) {
    case MI_10Z:
        setup.current_gain = MI_100Z;   
        break;
    case MI_30Z:
        setup.current_gain = MI_100Z;   
        break;
    case MI_100Z:
        xSemaphoreGive(setup.mutex);
        return 0;
        break;
    case MI_300Z:
        setup.current_gain = MI_100Z;   
        break;
    case MI_1KZ:
        setup.current_gain = MI_300Z;
        break;
    case MI_3KZ:
        setup.current_gain = MI_1KZ;
        break;
    case MI_10KZ:
        setup.current_gain = MI_3KZ;
        break;
    case MI_30KZ:
        setup.current_gain = MI_10KZ;
        break;
    case MI_100KZ:
        setup.current_gain = MI_30KZ;
        break;
    case MI_300KZ:
        setup.current_gain = MI_100KZ;
        break;
    case MI_1MZ:
        setup.current_gain = MI_300KZ;
        break;
    case MI_3MZ:
        setup.current_gain = MI_1MZ;
        break;
    case MI_10MZ:
        setup.current_gain = MI_3MZ;
        break;
    case MI_30MZ:
        setup.current_gain = MI_10MZ;
        break;
    case MI_100MZ:
        setup.current_gain = MI_30MZ;
        break;
    }

    xSemaphoreGive(setup.mutex);

    return 1;
}

uint8_t current_range_down()
{
    xSemaphoreTake(setup.mutex, portMAX_DELAY);

    if (!setup.auto_current_gain) {
        xSemaphoreGive(setup.mutex);
        return 0;
    }

    measurement_current_gain_t current = setup.current_gain;

    switch(current) {
    case MI_10Z:
        setup.current_gain = MI_100Z;
        break;
    case MI_30Z:
        setup.current_gain = MI_100Z;
        break;
    case MI_100Z:
        setup.current_gain = MI_300Z;
        break;
    case MI_300Z:
        setup.current_gain = MI_1KZ;
        break;
    case MI_1KZ:
        setup.current_gain = MI_3KZ;
        break;
    case MI_3KZ:
        setup.current_gain = MI_10KZ;
        break;
    case MI_10KZ:
        setup.current_gain = MI_30KZ;
        break;
    case MI_30KZ:
        setup.current_gain = MI_100KZ;
        break;
    case MI_100KZ:
        setup.current_gain = MI_300KZ;
        break;
    case MI_300KZ:
        setup.current_gain = MI_1MZ;
        break;
    case MI_1MZ:
        setup.current_gain = MI_3MZ;
        break;
    case MI_3MZ:
        setup.current_gain = MI_10MZ;
        break;
    case MI_10MZ:
        setup.current_gain = MI_30MZ;
        break;
    case MI_30MZ:
        setup.current_gain = MI_100MZ;
        break;
    case MI_100MZ:
        xSemaphoreGive(setup.mutex);
        return 0;
        break;
    }

    xSemaphoreGive(setup.mutex);

    return 1;
}

void apply_voltage_gain(measurement_voltage_gain_t gain, float *val)
{
    switch(gain) {
    case MV_x0_25:
        *val = *val * 4;
        break;
    case MV_x1:
        break;
    case MV_x2_5:
        *val = *val / 2.5;
        break;
    case MV_x10:
        *val = *val / 10;
        break;
    case MV_x25:
        *val = *val/ 25;
        break;
    case MV_x100:
        *val = *val / 100;
        break;
    }
}

void apply_current_gain(measurement_current_gain_t gain, float *val)
{
    switch(gain) {
    case MI_10Z:
        *val = *val / 10;
        break;
    case MI_30Z:
        *val = *val / 30;
        break;
    case MI_100Z:
        *val = *val / 100;
        break;
    case MI_300Z:
        *val = *val / 300;
        break;
    case MI_1KZ:
        *val = *val / 1000;
        break;
    case MI_3KZ:
        *val = *val / 3000;
        break;
    case MI_10KZ:
        *val = *val / 10000;
        break;
    case MI_30KZ:
        *val = *val / 30000;
        break;
    case MI_100KZ:
        *val = *val / 100000;
        break;
    case MI_300KZ:
        *val = *val / 300000;
        break;
    case MI_1MZ:
        *val = *val / 1000000;
        break;
    case MI_3MZ:
        *val = *val / 3000000;
        break;
    case MI_10MZ:
        *val = *val / 10000000;
        break;
    case MI_30MZ:
        *val = *val / 30000000;
        break;
    case MI_100MZ:
        *val = *val / 100000000;
        break;
    }
}

static inline const char* current_gain_to_str(measurement_current_gain_t g)
{
    switch (g) {
    case MI_10Z:     return "10R";
    case MI_30Z:     return "30R";
    case MI_100Z:    return "100R";
    case MI_300Z:    return "300R";
    case MI_1KZ:     return "1k";
    case MI_3KZ:     return "3k";
    case MI_10KZ:    return "10k";
    case MI_30KZ:    return "30k";
    case MI_100KZ:   return "100k";
    case MI_300KZ:   return "300k";
    case MI_1MZ:     return "1M";
    case MI_3MZ:     return "3M";
    case MI_10MZ:    return "10M";
    case MI_30MZ:    return "30M";
    case MI_100MZ:   return "100M";
    default:         return "?";
    }
}

static inline const char* voltage_gain_to_str(measurement_voltage_gain_t g)
{
    switch (g) {
    case MV_x0_25: return "x0.25";
    case MV_x1:    return "x1";
    case MV_x2_5:  return "x2.5";
    case MV_x10:   return "x10";
    case MV_x25:   return "x25";
    case MV_x100:  return "x100";
    default:       return "?";
    }
}

const char* eng_notation(float value, char* buf, size_t bufsize, const char* unit) {
    const char* prefixes[] = {"p", "n", "u", "m", "", "k", "M", "G"};
    int exp = 0;
    float v = value;
    while (fabsf(v) > 0 && fabsf(v) < 1.0f && exp > -12) { v *= 1000.0f; exp -= 3; }
    while (fabsf(v) >= 1000.0f && exp < 9) { v /= 1000.0f; exp += 3; }
    int idx = exp/3 + 4; // 4 is index for ""
    if (idx < 0) idx = 0;
    if (idx > 7) idx = 7;
    snprintf(buf, bufsize, "%.3g %s%s", v, prefixes[idx], unit);
    return buf;
}

char eng_buf[32];

const float counts_to_volts = (3.3/4096);

float fft_input[SAMPLE_BUFFER_SIZE];
float fft_output[SAMPLE_BUFFER_SIZE];
arm_rfft_fast_instance_f32 fft_instance;

uint8_t process()
{
    uint8_t msg[128];
    int len;        
    rerange = 0;

    unpack_channel(sample_buffer, tmp, SAMPLE_BUFFER_SIZE, 1);

    int saturated = 0;
    for (uint32_t i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
        if (tmp[i] < 100 || tmp[i] > 3996) { // 10-count margin for noise
            saturated = 1;
            break;
        }
    }

    goertzel_out_t voltage = goertzel_u16(
        tmp,
        SAMPLE_BUFFER_SIZE,
        2000000,
        setup.source_frequency
    );

    #ifdef FFT

    for (uint32_t i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
        fft_input[i] = (float)tmp[i] - 2048.0f; // Center if needed
    }

    arm_rfft_fast_init_f32(&fft_instance, SAMPLE_BUFFER_SIZE);

    arm_rfft_fast_f32(&fft_instance, fft_input, fft_output, 0);


    //get peak bin
    uint32_t peak_bin = 0;
    float peak_mag = 0;
    for (uint32_t i = 0; i < SAMPLE_BUFFER_SIZE / 2; i++) {
        float re = fft_output[2 * i];
        float im = fft_output[2 * i + 1];
        float mag = sqrtf(re * re + im * im);
        if (mag > peak_mag) {
            peak_mag = mag;
            peak_bin = i;
        }
    }

    float re = fft_output[2 * peak_bin];
    float im = fft_output[2 * peak_bin + 1];
    float magnitude = sqrtf(re * re + im * im) * 2.0f / SAMPLE_BUFFER_SIZE; // scale for amplitude
    float phase = atan2f(im, re);
    magnitude *= counts_to_volts; // Convert to volts


    voltage.magnitude = magnitude;
    voltage.phase = phase;

    len = 0;
    len = snprintf((char*)msg, sizeof(msg), "FFT mag: %f\r\n", magnitude);
    HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);

    #endif

    measurement_voltage_gain_t v_gain = setup.voltage_gain;
    measurement_current_gain_t i_gain = setup.current_gain;

    #ifdef VERBOSE
    len = 0;
    len = snprintf((char*)msg, sizeof(msg), "voltage mag: %f\r\n", voltage.magnitude);
    HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);

    if (saturated) {
        len = 0;
        len = snprintf((char*)msg, sizeof(msg), "Voltage saturation detected\r\n");
        HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);
    }
    #endif

    if (voltage.magnitude > AUTORANGE_UP || saturated) {
        rerange = voltage_range_up();
        #ifdef VERBOSE
        len = 0;
        len = snprintf((char*)msg, sizeof(msg), "voltage range up\r\n");
        HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);
        #endif
    } else if (voltage.magnitude < AUTORANGE_DOWN/*  && !saturated */) {
        rerange = voltage_range_down();
        #ifdef VERBOSE
        len = 0;
        len = snprintf((char*)msg, sizeof(msg), "voltage range down\r\n");
        HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);
        #endif
    }

    unpack_channel(sample_buffer, tmp, SAMPLE_BUFFER_SIZE, 0);

    saturated = 0;
    for (uint32_t i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
        if (tmp[i] < 400 || tmp[i] > 3696) {
            saturated = 1;
            break;
        }
    }

    goertzel_out_t current = goertzel_u16(
        tmp,
        SAMPLE_BUFFER_SIZE,
        2000000,
        setup.source_frequency
    );

    #ifdef FFT

    for (uint32_t i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
        fft_input[i] = (float)tmp[i] - 2048.0f; // Center if needed
    }

    arm_rfft_fast_init_f32(&fft_instance, SAMPLE_BUFFER_SIZE);

    arm_rfft_fast_f32(&fft_instance, fft_input, fft_output, 0);


    //get peak bin
     peak_bin = 0;
     peak_mag = 0;
    for (uint32_t i = 0; i < SAMPLE_BUFFER_SIZE / 2; i++) {
        float re = fft_output[2 * i];
        float im = fft_output[2 * i + 1];
        float mag = sqrtf(re * re + im * im);
        if (mag > peak_mag) {
            peak_mag = mag;
            peak_bin = i;
        }
    }

    re = fft_output[2 * peak_bin];
    im = fft_output[2 * peak_bin + 1];
    magnitude = sqrtf(re * re + im * im) * 2.0f / SAMPLE_BUFFER_SIZE; // scale for amplitude
    magnitude *= counts_to_volts; // Convert to volts

    phase = atan2f(im, re);


    current.magnitude = magnitude;
    current.phase = phase;

    #endif

    #ifdef VERBOSE
    len = 0;
    len = snprintf((char*)msg, sizeof(msg), "current mag: %f\r\n", current.magnitude);
    HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);

    if (saturated) {
        len = 0;
        len = snprintf((char*)msg, sizeof(msg), "Current saturation detected\r\n");
        HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);
    }
    #endif

    if (current.magnitude > AUTORANGE_UP/*  || saturated */) {
        rerange = current_range_up();
        #ifdef VERBOSE
        len = 0;
        len = snprintf((char*)msg, sizeof(msg), "current range up\r\n");
        HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);
        #endif
    }

    if (current.magnitude < AUTORANGE_DOWN/*  && !saturated */) {
        rerange = current_range_down();
        #ifdef VERBOSE
        len = 0;
        len = snprintf((char*)msg, sizeof(msg), "current range down\r\n");
        HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);
        #endif
    }

    voltage.magnitude *= counts_to_volts;

    apply_voltage_gain(v_gain, &voltage.magnitude);

    current.magnitude *= counts_to_volts;
    //current.phase += PI;
    current.phase = wrap_pi(current.phase);
    voltage.phase = wrap_pi(voltage.phase);

    apply_current_gain(i_gain, &current.magnitude);

    const char* v_gain_str = voltage_gain_to_str(v_gain);
    const char* i_gain_str = current_gain_to_str(i_gain);

    //current.magnitude -= 0.0000021;

    #ifdef VERBOSE
    len = 0;
    eng_notation(voltage.magnitude, eng_buf, sizeof(eng_buf), "V");
    len = snprintf((char*)msg, sizeof(msg), "Voltage: %s, %s, %f\r\n", v_gain_str, eng_buf, (voltage.phase * (180.0f / PI)));
    //len = snprintf((char*)msg, sizeof(msg), "Frequency %lu, Delta Phase: %f, Magnitude Ratio %f, %f\r\n", f, (phase2 - phase1) * (180.0f / (float)PI), magnitude1, magnitude2);
    HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);

    len = 0;
    eng_notation(current.magnitude, eng_buf, sizeof(eng_buf), "A");
    len = snprintf((char*)msg, sizeof(msg), "Current: %s, %s, %f\r\n", i_gain_str, eng_buf, (current.phase * (180.0f / PI)));
    //len = snprintf((char*)msg, sizeof(msg), "Frequency %lu, Delta Phase: %f, Magnitude Ratio %f, %f\r\n", f, (phase2 - phase1) * (180.0f / (float)PI), magnitude1, magnitude2);
    HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);
    #endif

    float impedance_magnitude;
    float impedance_phase;

    impedance_phase = voltage.phase - current.phase;
    impedance_phase = wrap_pi(impedance_phase);

    //impedance_phase -= 33 * (PI/180.0f);

    impedance_magnitude = voltage.magnitude / current.magnitude;

    #ifdef VERBOSE
    len = 0;
    len = snprintf((char*)msg, sizeof(msg), "Impedance: %G, %f\r\n", impedance_magnitude, (impedance_phase * (180.0f / PI)));
    //len = snprintf((char*)msg, sizeof(msg), "Frequency %lu, Delta Phase: %f, Magnitude Ratio %f, %f\r\n", f, (phase2 - phase1) * (180.0f / (float)PI), magnitude1, magnitude2);
    HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);


    float cap = 0.0f, ind = 0.0f;
    float phase_deg = impedance_phase * (180.0f / PI);
    float freq = (float)setup.source_frequency;

    if (phase_deg < -45.0f) {
        // Capacitive
        cap = 1.0f / (2.0f * PI * freq * impedance_magnitude);
        eng_notation(cap, eng_buf, sizeof(eng_buf), "F");
        len = snprintf((char*)msg, sizeof(msg), "Equivalent Capacitance: %s\r\n\r\n", eng_buf);
        HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);
    } else if (phase_deg > 45.0f) {
        // Inductive
        ind = impedance_magnitude / (2.0f * PI * freq);
        eng_notation(ind, eng_buf, sizeof(eng_buf), "H");
        len = snprintf((char*)msg, sizeof(msg), "Equivalent Inductance: %s\r\n\r\n", eng_buf);
        HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);
    } else {
        eng_notation(impedance_magnitude, eng_buf, sizeof(eng_buf), "Ohm");
        len = snprintf((char*)msg, sizeof(msg), "Equivalent Resistance: %s\r\n\r\n", eng_buf);
        HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);
    }
    #endif

    result.impedance_magnitude = impedance_magnitude;
    if (setup.angle_unit == DEGREE) {
        result.impedance_phase = impedance_phase * (180.0f / PI);
    } else {
        result.impedance_phase = impedance_phase;
    }

    if (rerange) {
        #ifdef VERBOSE
        len = 0;
        len = snprintf((char*)msg, sizeof(msg), "Applied new range settings. Please wait for next measurement...\r\n\r\n");
        HAL_UART_Transmit(&huart1, msg, len, HAL_MAX_DELAY);
        #endif
        return 1;
    }

    return 0;
}