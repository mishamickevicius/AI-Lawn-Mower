#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

// My components
#include "gpio_test.h"
#include "ir_sensor.h"
#include "accelerometer.h"
#include "motor_control.h"

// My Macros
#define NUM_OF_IR_SENSORS 1
#define OUTPUT_GPIO_PIN_NUM 4

static const char* TAG = "Main";

void app_main(void)
{   
    ESP_LOGI(TAG, "Main program starting");
    esp_err_t ret; // Return var

    ESP_ERROR_CHECK(init_pwm_module());

    while (1) {
        ESP_LOGI(TAG, "PWM Should be active");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
