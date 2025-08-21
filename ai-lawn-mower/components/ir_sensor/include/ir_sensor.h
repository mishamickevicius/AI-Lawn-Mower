#ifndef IR_SENSOR_H
#define IR_SENSOR_H

#include "esp_err.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"

// State names enumerated 
typedef enum {
    IR_STATE_INIT,
    IR_STATE_STABLE_DETECTED,
    IR_STATE_DEBOUNCING_CLEAN,
    IR_STATE_STABLE_CLEAN,
    IR_STATE_DEBOUNCING_DETECTED
} ir_sensor_state_t;

// Struct that holds FSM state and relavent data for ONE sensor
typedef struct {
    int pinNum;
    ir_sensor_state_t currentState;
    bool previousLevel;
    TickType_t lastLevelChangeTime;

} ir_sensor_data_t;


ir_sensor_data_t* create_ir_sensor_array(int numOfSensors);

// ISR Function
esp_err_t ir_sensor_init(ir_sensor_data_t* irSensorArr, int numOfSensors);

#endif