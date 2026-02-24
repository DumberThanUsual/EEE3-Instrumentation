#include "stm32f446xx.h"
#include "bias.h"
#include "FreeRTOS.h"

#include "task.h"
#include "queue.h"

/*
typedef enum {
    BIAS_VOLTAGE,
    BIAS_CURRENT_RAMP_UP,
    BIAS_CURRENT_RAMP_DOWN,
    BIAS_CURRENT_STEADY,
} bias_state_t;

*/

void start_adc() 
{
    return;
}


static void bias_task(void *pvParameters) 
{
    start_adc();
    bias_t *req;
    bias_t state;
    while (1) {
        req = 0;
        req = ulTaskNotifyTake(pdTRUE, 0 );
        if (req) {
            setup_bias(req);
        }

        switch (state.type) {
            case BIAS_VOLTAGE:
                break;
            case BIAS_CURRENT:

                break;
        }

    }
}

void set_bias(bias_t *bias) 
{
    return;
}