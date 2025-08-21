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

// My Macros
#define NUM_OF_IR_SENSORS 1
#define OUTPUT_GPIO_PIN_NUM 4

static const char* TAG = "Main";

void app_main(void)
{   
    ESP_LOGI(TAG, "Main program starting");
    esp_err_t ret; // Return var

    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    ret = i2c_master_init(&bus_handle, &dev_handle);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error with i2c_master_init: %s", esp_err_to_name(ret));
    }

    // Wake up mpu6050
    ESP_ERROR_CHECK(write_mpu6050_register(dev_handle, (uint8_t)0x6B, 0x00));

    // Set Config reg
    uint8_t config_msg = 0b00000001;
    ESP_ERROR_CHECK(write_mpu6050_register(dev_handle, (uint8_t)0x1A, config_msg));

    // Set Accelerometer config reg
    config_msg = 0b00001000;
    ESP_ERROR_CHECK(write_mpu6050_register(dev_handle, (uint8_t)0x1C, config_msg));

    // Read Accelerometer config reg
    uint8_t acc_config_read;
    ESP_ERROR_CHECK(read_mpu6050_register(dev_handle, (uint8_t)0x1C, &acc_config_read));
    ESP_LOGI(TAG, "Accelerometer Config Register Current Value: %d", acc_config_read);


    // int16_t raw_value;
    // float z_acceleration_value;
    // uint8_t msb_byte, lsb_byte;
    // TickType_t start_time, num_of_ticks;
    float vel_magnitude;
    while (1) {
        // Keep reading the acceleration of Z-axis
        // start_time = xTaskGetTickCount();
        // ESP_ERROR_CHECK(read_mpu6050_register(dev_handle, (uint8_t)0x3F, &msb_byte));
        // ESP_ERROR_CHECK(read_mpu6050_register(dev_handle, (uint8_t)0x40, &lsb_byte));


        // raw_value = (int16_t)(((uint16_t)msb_byte << 8) | lsb_byte);
        // z_acceleration_value = ((float)raw_value / 8192.0) * 9.81;
        // // num_of_ticks = (xTaskGetTickCount() - start_time);
        // // ESP_LOGI(TAG, "The register read took %d ms long", (int)(num_of_ticks / portTICK_PERIOD_MS));
        // ESP_LOGI(TAG, "Z Acceleration Value: %f", z_acceleration_value);
        
        // Test calculate_velocity()
        vel_magnitude = calculate_velocity(dev_handle);
        ESP_LOGI(TAG, "Current Velocity Magnitude ---> %f m/s", vel_magnitude);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
