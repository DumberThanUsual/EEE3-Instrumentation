#ifndef MEASURE_H
#define MEASURE_H

#include "stm32f446xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "math.h"
#include "signal.h"
#include "adc.h"
#include "dac.h"
#include "main.h"

typedef enum {
    M_OFFSET_VOLTAGE,
    M_OFFSET_CURRENT,
} measurement_offset_type_t;

typedef enum {
    MV_x100,
    MV_x25,
    MV_x10,
    MV_x2_5,
    MV_x1,
    MV_x0_25
} measurement_voltage_gain_t;

typedef enum {
    MI_10Z,
    MI_30Z,
    MI_100Z,
    MI_300Z,
    MI_1KZ,
    MI_3KZ,
    MI_10KZ,
    MI_30KZ,
    MI_100KZ,
    MI_300KZ,
    MI_1MZ,
    MI_3MZ,
    MI_10MZ,
    MI_30MZ,
    MI_100MZ
} measurement_current_gain_t;

typedef enum {
    DEGREE,
    RADIAN
} measurement_angle_unit_t;

typedef struct {
    float magnitude;
    float phase;
} impedance_t;

typedef struct {
    uint16_t source_amplitude;
    uint32_t source_frequency;
    measurement_voltage_gain_t voltage_gain;
    measurement_current_gain_t current_gain;

    uint8_t auto_voltage_gain;
    uint8_t auto_current_gain;

    uint32_t sample_rate;
    measurement_angle_unit_t angle_unit;

    impedance_t correction_short;
    impedance_t correction_open;
    uint8_t correction_enabled;

    SemaphoreHandle_t mutex;
} setup_t;

typedef struct {
    float impedance_magnitude;
    float impedance_phase;
} result_t;

extern setup_t setup;
extern setup_t buffered_new_setup;
extern setup_t buffered_autorange_setup;

extern result_t result;

extern TaskHandle_t measure_task_handle;

#define SAMPLE_BUFFER_SIZE 4096
extern uint32_t sample_buffer[SAMPLE_BUFFER_SIZE];
extern SemaphoreHandle_t buffer_in_use;
extern SemaphoreHandle_t complete;

BaseType_t measure_init();

#endif