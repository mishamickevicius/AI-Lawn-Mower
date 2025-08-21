#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

// My components
#include "gpio_test.h"

// My Macros
#define TEST_GPIO_PIN GPIO_NUM_4

static const char* TAG = "Main";

void app_main(void)
{
    ESP_LOGI(TAG, "Main program is starting\n"); 

    // Configure GPIO pin 
    esp_err_t ret = set_gpio_to_output(TEST_GPIO_PIN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error has occured in app_main\n Error: %s\n", esp_err_to_name(ret));
    }

    // Check init value
    if (get_gpio_value(TEST_GPIO_PIN) != false) {
        ESP_LOGE(TAG, "GPIO pin not init at correct value\n");
    }

    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS; // delay amount

    // Test the gpio pin by switching it on and off 
    bool curr_value = 0; // Starting value
    while(1) {
        ESP_LOGI(TAG, "Starting level switch Curr Level: %d", curr_value);
        
        curr_value = !curr_value; // Switch the value
        ret = set_gpio(TEST_GPIO_PIN, curr_value); // Set value

        vTaskDelay(xDelay); // Delay 


    }
    
}
