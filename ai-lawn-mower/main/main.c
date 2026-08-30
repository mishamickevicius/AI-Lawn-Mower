#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

// My components
#include "gpio_driver.h"
#include "ir_sensor.h"
#include "motor_control.h"

// My Macros
#define IR_SENSOR_1_PIN GPIO_NUM_1
#define IR_SENSOR_2_PIN GPIO_NUM_2
#define IR_SENSOR_3_PIN GPIO_NUM_3
#define IR_SENSOR_4_PIN GPIO_NUM_4

static const char* TAG = "Main";

void app_main(void)
{   
    ESP_LOGI(TAG, "Main program starting");
    esp_err_t ret; // Return var

    // Init IR Sensor 1
    ir_sensor_data_t irSensor1Data;
    ret = ir_sensor_init(IR_SENSOR_1_PIN, &irSensor1Data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error has occured with IR Sensor 1 init\n Error: %s\n", esp_err_to_name(ret));
    }

    while(true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "Current IR Sensor state: %d", irSensor1Data.currentState);
    }
}
