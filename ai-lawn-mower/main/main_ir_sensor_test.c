#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

// My components
#include "gpio_test.h"
#include "ir_sensor.h"

// My Macros
#define NUM_OF_IR_SENSORS 1
#define OUTPUT_GPIO_PIN_NUM 4

static const char* TAG = "Main";

void app_main(void)
{   
    ESP_LOGI(TAG, "Main program starting");
    esp_err_t ret; // Return var

    // Get the array pointer
    ir_sensor_data_t* irSensorArrP = create_ir_sensor_array(NUM_OF_IR_SENSORS);

    // Run a simple test
    // Use the ir sensor as a switch to allow a light bulb to turn on or off
    ret = set_gpio_to_output(OUTPUT_GPIO_PIN_NUM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error has occured in app_main\n Error: %s\n", esp_err_to_name(ret));
    }

    // Init ir sensors
    ret = ir_sensor_init(irSensorArrP, NUM_OF_IR_SENSORS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error has occured in app_main\n Error: %s\n", esp_err_to_name(ret));
    }

    // const TickType_t xDelay = 2000 / portTICK_PERIOD_MS; // delay amount

    while (1) {
        // vTaskDelay(xDelay); // Delay 
        // ESP_LOGI(TAG, "Stalling");

        int level = gpio_get_level(GPIO_NUM_35);
        ESP_LOGI(TAG, "IR raw level = %d", level);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
