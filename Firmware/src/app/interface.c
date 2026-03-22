#include "scpi/scpi.h"
#include "measure.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

#define SCPI_IDN1 "EEE3"
#define SCPI_IDN2 "INSTR2026"
#define SCPI_IDN3 "Harry & Yuki, EEE3 Instrumentation"
#define SCPI_IDN4 "Version 1.0"

uint8_t msg[64];
uint8_t len;

TaskHandle_t interface_task_handle;

scpi_choice_def_t angle_unit[] = {
    {"DEGree", DEGREE},
    {"RADian", RADIAN},
    SCPI_CHOICE_LIST_END /* termination of option list */
};

scpi_result_t SCPI_ConfAngleUnit(scpi_t * context) {
    int32_t param;
    if (!SCPI_ParamChoice(context, angle_unit, &param, TRUE)) {
        return SCPI_RES_ERR;
    }
    switch (param) {
        case DEGREE: // DEGREE
            buffered_new_setup.angle_unit = DEGREE;
            break;
        case RADIAN: // RADIAN (use current measurement unit, no conversion)
            buffered_new_setup.angle_unit = RADIAN; // Use the currently selected unit
            break;
        default:
            return SCPI_RES_ERR; // Invalid choice
    }
    return SCPI_RES_OK;
}

scpi_result_t SCPI_ConfAngleUnitQ(scpi_t * context) {
    const char *name;
    if (!SCPI_ChoiceToName(angle_unit, buffered_new_setup.angle_unit, &name)) {
        return SCPI_RES_ERR;
    }
    SCPI_ResultMnemonic(context, name);
    return SCPI_RES_OK;
}

// MEAS:IMP? handler
scpi_result_t SCPI_MeasImpedanceQ(scpi_t * context) 
{
    xTaskNotifyGive(measure_task_handle);
    xSemaphoreTake(complete, portMAX_DELAY);
    SCPI_ResultArrayFloat(context, &result, 2, SCPI_FORMAT_ASCII);
    return SCPI_RES_OK;
}

// CONF:FREQ <value>
scpi_result_t SCPI_ConfFreq(scpi_t * context) {
    float freq;
    if (!SCPI_ParamFloat(context, &freq, TRUE)) return SCPI_RES_ERR;
    buffered_new_setup.source_frequency = freq;
    return SCPI_RES_OK;
}

// CONF:FREQ?
scpi_result_t SCPI_ConfFreqQ(scpi_t * context) {
    float freq = buffered_new_setup.source_frequency;
    SCPI_ResultFloat(context, freq);
    return SCPI_RES_OK;
}

// CONF:VOLTage <value>
scpi_result_t SCPI_ConfVoltage(scpi_t * context) {
    float voltage;
    if (!SCPI_ParamFloat(context, &voltage, TRUE)) return SCPI_RES_ERR;
    buffered_new_setup.source_amplitude = voltage;
    return SCPI_RES_OK;
}

// CONF:VOLTage?
scpi_result_t SCPI_ConfVoltageQ(scpi_t * context) {
    float voltage = buffered_new_setup.source_amplitude;
    SCPI_ResultFloat(context, voltage);
    return SCPI_RES_OK;
}

scpi_result_t SCPI_CorrectionStateQ(scpi_t * context) {
    SCPI_ResultBool(context, buffered_new_setup.correction_enabled);
    return SCPI_RES_OK;
}

scpi_result_t SCPI_CorrectionState(scpi_t * context) {
    if (!SCPI_ParamBool(context, &buffered_new_setup.correction_enabled, TRUE)) return SCPI_RES_ERR;
    return SCPI_RES_OK;
}

scpi_result_t SCPI_CalOpen(scpi_t * context) {
    xTaskNotifyGive(measure_task_handle);
    xSemaphoreTake(complete, portMAX_DELAY);
    buffered_new_setup.correction_open.magnitude = result.impedance_magnitude;
    buffered_new_setup.correction_open.phase = result.impedance_phase;
    return SCPI_RES_OK;
}

scpi_result_t SCPI_CalShort(scpi_t * context) {
    xTaskNotifyGive(measure_task_handle);
    xSemaphoreTake(complete, portMAX_DELAY);
    buffered_new_setup.correction_short.magnitude = result.impedance_magnitude;
    buffered_new_setup.correction_short.phase = result.impedance_phase;
    return SCPI_RES_OK;
}

scpi_result_t SCPI_CalLoad(scpi_t * context) {
    // Implement your calibration routine here
    // For example, you might set a flag to perform a load calibration in the next measurement cycle
    // buffered_new_setup.calibrate_load = 1; // Example flag, implement as needed
    return SCPI_RES_OK;
}

const scpi_command_t scpi_commands[] = {
    {"*IDN?", SCPI_CoreIdnQ, 0},
    {"MEAS:IMP?", SCPI_MeasImpedanceQ, 0},
    {"CONF:FREQ", SCPI_ConfFreq, 0},
    {"CONF:FREQ?", SCPI_ConfFreqQ, 0},
    {"CONF:ANGleunit", SCPI_ConfAngleUnit, 0},
    {"CONF:ANGleunit?", SCPI_ConfAngleUnitQ, 0},
    {"SOURCE:VOLTage", SCPI_ConfVoltage, 0},
    {"CONF:VOLTage?", SCPI_ConfVoltageQ, 0},
    {"CORRection:STATE", SCPI_CorrectionStateQ, 0},
    {"CORRection:STATE?", SCPI_CorrectionStateQ, 0},
    {"CORRection:OPEN:ACQuire", SCPI_CalOpen, 0},
    {"CORRection:SHORT:ACQuire", SCPI_CalShort, 0},
    {"CORRection:LOAD:ACQuire", SCPI_CalLoad, 0},
    SCPI_CMD_LIST_END
};

size_t SCPI_write(scpi_t *context, const char *data, size_t len) {
    // Replace &huart1 with your UART handle if different
    HAL_UART_Transmit(&huart1, (uint8_t *)data, len, HAL_MAX_DELAY);
    return len;
}

scpi_interface_t scpi_interface = {
    .error = NULL,
    .write = SCPI_write, // Implement this to send data over UART
    .control = NULL,
    .flush = NULL,
    .reset = NULL,
};

#define SCPI_INPUT_BUFFER_LENGTH 256
static char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
static scpi_error_t scpi_error_queue_data[7];
scpi_t scpi_context;
uint8_t uart_rx_byte;

void SCPI_task()
{
    SCPI_Init(&scpi_context,
            scpi_commands,
            &scpi_interface,
            scpi_units_def,
            SCPI_IDN1, SCPI_IDN2, SCPI_IDN3, SCPI_IDN4,
            scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
            scpi_error_queue_data, 7);

    while (1) {
        HAL_UART_Receive(&huart1, &uart_rx_byte, 1, HAL_MAX_DELAY); // Start receiving data
        HAL_UART_Transmit(&huart1, &uart_rx_byte, 1, HAL_MAX_DELAY); // Echo back received byte (optional)
        SCPI_Input(&scpi_context, (char *)&uart_rx_byte, 1);
    }
}


void interface_init()
{
    xTaskCreate(SCPI_task, "SCPI_task", 1024, NULL, 1, &interface_task_handle);
}
