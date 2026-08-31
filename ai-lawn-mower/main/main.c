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
#include "rpi_connector.h"

// My Macros
#define IR_SENSOR_1_PIN GPIO_NUM_1
#define IR_SENSOR_2_PIN GPIO_NUM_2
#define IR_SENSOR_3_PIN GPIO_NUM_3
#define IR_SENSOR_4_PIN GPIO_NUM_4

#define RX_BUFFER_SIZE 256

static const char* TAG = "Main";

static void rpi_uart_task(void *pvParameters)
{
    uint8_t rx_buffer[RX_BUFFER_SIZE];

    while (1) {
        // 1. Read incoming data from the Raspberry Pi (timeout set to 1000 ms)
        int bytes_received = rpi_uart_receive(rx_buffer, sizeof(rx_buffer) - 1, 1000);

        if (bytes_received > 0) {
            rx_buffer[bytes_received] = '\0'; // Null-terminate for string printing
            ESP_LOGI(TAG, "Received from RPi (%d bytes): %s", bytes_received, rx_buffer);

            // 2. Respond back to the RPi with an acknowledgment
            const char *ack_msg = "ESP32 received your message!\n";
            rpi_uart_send(ack_msg, strlen(ack_msg));
        } else if (bytes_received == 0) {
            ESP_LOGD(TAG, "Waiting for data from RPi...");
        }

        // Short yield to let other tasks run
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

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

    // Initialize the UART peripheral, pins, and driver
    esp_err_t err = rpi_uart_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize UART to RPi: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "UART initialized successfully. Starting communication task...");

    // Create a dedicated FreeRTOS task for UART communication
    xTaskCreate(
        rpi_uart_task,      // Task function
        "rpi_uart_task",    // Task name
        4096,               // Stack depth in words
        NULL,               // Task parameters
        5,                  // Priority
        NULL                // Task handle
    );
}