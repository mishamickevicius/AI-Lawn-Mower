#include "ir_sensor.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "gpio_test.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_intr_alloc.h"

static const char* TAG = "IrSensor";

#define MAX_NUM_OF_SENSORS 5
#define ESP_INTR_FLAG_DEFAULT 0
#define IR_SENSOR_DEBOUNCE_MS 50

/*
Basic idea: Have a array of scructs where each scruct is an ir sensor
and have the helper functions (in this file) have the array pointer 
as a param and handle the array inside. 

Ideas:
1. Have a functions that initilizes the array of sensors
2. Have a function that goes through the array and initilizes the GPIO pins
using gpio_driver
3. Have the task and ISR functionallity here
*/

// Remember to check for NULL after calling this function
ir_sensor_data_t* create_ir_sensor_array(int numOfSensors) // Max 5 Sensors
{
    // Check for limit
    if (numOfSensors > MAX_NUM_OF_SENSORS || numOfSensors <= 0) {
        return NULL;
    }
    // Allocate memory on heap for array
    ir_sensor_data_t* arr = (ir_sensor_data_t*)malloc(numOfSensors * sizeof(ir_sensor_data_t));

    // Check if memory was allocated
    if (arr == NULL) {
        ESP_LOGE(TAG, "create_ir_sensor_array could not allocate memory");
        return NULL;
    }

    // Valid GPIO pins
    int valid_pins[5] = {35, 36, 37, 38, 39};

    // Fill array with pin nums
    for (int i = 0; i < numOfSensors; i++) {
        arr[i].pinNum = valid_pins[i];
        arr[i].currentState = IR_STATE_INIT;
        arr[i].lastLevelChangeTime = 0; 
        arr[i].previousLevel = 0;


        // Init gpio pins to input mode

        esp_err_t ret = set_gpio_to_input(valid_pins[i], false, true, GPIO_INTR_ANYEDGE);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error has occured with gpio pin init \n Error: %s\n", esp_err_to_name(ret));
            return NULL;
        }
    }

    return arr; 
}


/* 
--------------------------------------------
------ISR and event related functions ------ 
--------------------------------------------
*/

// Static (file-scope) vars for the components internal state
// This is a queue from FreeRTOS that acts as the communcation channel
static QueueHandle_t sIrSensorEvtQueue = NULL;

// This stores the handle(indentifier) of the FreeRTOS task that processes events
static TaskHandle_t sIrSensorTaskHandle = NULL;

// Pointer to the array of pinNums
static ir_sensor_data_t sIrSensorsData[MAX_NUM_OF_SENSORS];

// Stores the number of pins
static size_t sNumIrSensorsInternal = 0;

// --- Internal ISR Handler
// This function runs in interrupt context. Keep it as short as possible.
// Its main job is to send the GPIO number to the queue.
// IRAM_ATTR tells the mcu to place this function into IRAM not Flash memory
static void IRAM_ATTR ir_sensor_isr_handler(void* arg)
{
    uint32_t pinNum = (uint32_t) arg; // This casts the arg to an int and stores it
    xQueueSendFromISR(sIrSensorEvtQueue, &pinNum, NULL);
}

// --- Internal Processing Task ---
// This task runs in task context and processes events from the queue.
// This is where your FSM logic or other complex processing for IR sensors would go.
/*
Things that need to be added/kept in mind:
- Infinite Loop (If it exits loop, it will "die")
- Blocking on queue (The task will be blocked most of the time and only
get unblocked when an queue event happens)
- Receving data (Reading the queue)
- Confirm pin state(Crucial for edge interrupts)
- Debouncing 
- FSM Logic 
- Trigger Actions
- Etc

This task will push state changes to the central queue
*/
static void ir_sensor_processing_task(void* arg)
{
    uint32_t pinNum; // Holder var for pin num
    for (;;) {
        if (xQueueReceive(sIrSensorEvtQueue, &pinNum, portMAX_DELAY) == pdTRUE) 
        {
            ESP_LOGI(TAG, "Event recieved by task --- pin num: %d", (int)pinNum);
            int index = 35 - (int) pinNum; // Pin 35 is the first avalible pin therefore 35-35 = 0 index
            TickType_t currentTicks = xTaskGetTickCount();
            TickType_t debounceThreshold = pdMS_TO_TICKS(IR_SENSOR_DEBOUNCE_MS);
            int currentRawLevel;
            // FSM Logic
            switch (sIrSensorsData[index].currentState)
            {
                case IR_STATE_INIT:
                    if (gpio_get_level(pinNum) == 1) { 
                        sIrSensorsData[index].currentState = IR_STATE_STABLE_CLEAN;
                        ESP_LOGI(TAG, "Sensor %d (GPIO %d): Initial state -> STABLE_CLEAN", index, (int)pinNum);
                    } else {
                        sIrSensorsData[index].currentState = IR_STATE_STABLE_DETECTED;
                        ESP_LOGI(TAG, "Sensor %d (GPIO %d): Initial state -> STABLE_DETECTED", index, (int)pinNum);
                    }
                    ESP_LOGI(TAG, "Updating lastLevelChangeTime");
                    sIrSensorsData[index].lastLevelChangeTime = currentTicks;
                    sIrSensorsData[index].previousLevel = gpio_get_level(pinNum);
                    break;
                
                case IR_STATE_DEBOUNCING_CLEAN:
                    currentRawLevel = gpio_get_level(pinNum);

                    if (currentRawLevel == 1) { // If the level is still high
                        if ((currentTicks - sIrSensorsData[index].lastLevelChangeTime) >= debounceThreshold) {
                            sIrSensorsData[index].currentState = IR_STATE_STABLE_CLEAN;
                            sIrSensorsData[index].previousLevel = 1;
                            ESP_LOGI(TAG, "Sensor %d (GPIO %d): DEBOUNCING_CLEAN -> STABLE_CLEAN", index, (int)pinNum);
                        } else { // Haven't met the debounce threshold
                            // Do nothing
                        }
                    } else { // Level went to low -- This is a bounce
                        sIrSensorsData[index].currentState = IR_STATE_DEBOUNCING_DETECTED; // Transition to debouncing the 'detected' state
                        ESP_LOGI(TAG, "Updating lastLevelChangeTime");
                        sIrSensorsData[index].lastLevelChangeTime = currentTicks;
                        // ONLY set the previousLevel once its determinded to be a stable level
                        ESP_LOGI(TAG, "Sensor %d (GPIO %d): DEBOUNCING_CLEAN -> DEBOUNCING_DETECTED", index, (int)pinNum);
                    }
                    break;
                
                case IR_STATE_DEBOUNCING_DETECTED:
                    currentRawLevel = gpio_get_level(pinNum);

                    if (currentRawLevel == 0) {
                        if ((currentTicks - sIrSensorsData[index].lastLevelChangeTime) >= debounceThreshold) {
                            sIrSensorsData[index].currentState = IR_STATE_STABLE_DETECTED;
                            sIrSensorsData[index].previousLevel = 0;
                            ESP_LOGI(TAG, "Sensor %d (GPIO %d): DEBOUNCING_DETECTED -> STABLE_DETECTED", index, (int)pinNum);
                        } else {
                            // Do Nothing
                        }
                    } else { // Level went High, this is a bounce
                        sIrSensorsData[index].currentState = IR_STATE_DEBOUNCING_CLEAN;
                        ESP_LOGI(TAG, "Updating lastLevelChangeTime");
                        sIrSensorsData[index].lastLevelChangeTime = currentTicks;
                        ESP_LOGI(TAG, "Sensor %d (GPIO %d): DEBOUNCING_DETECTED -> DEBOUNCING_CLEAN", index, (int)pinNum);
                    }
                    break;
                
                case IR_STATE_STABLE_CLEAN:
                    currentRawLevel = gpio_get_level(pinNum);

                    if (currentRawLevel == 0) {
                        sIrSensorsData[index].currentState = IR_STATE_DEBOUNCING_DETECTED;
                        ESP_LOGI(TAG, "Updating lastLevelChangeTime");
                        sIrSensorsData[index].lastLevelChangeTime = currentTicks;
                        ESP_LOGI(TAG, "Sensor %d (GPIO %d): STABLE_CLEAN -> DEBOUNCING_DETECTED", index, (int)pinNum);
                    }
                    // Don't need to do anything if level == 1
                    break;
                
                case IR_STATE_STABLE_DETECTED:
                    currentRawLevel = gpio_get_level(pinNum);

                    if (currentRawLevel == 1) {
                        sIrSensorsData[index].currentState = IR_STATE_DEBOUNCING_CLEAN;
                        ESP_LOGI(TAG, "Updating lastLevelChangeTime");
                        sIrSensorsData[index].lastLevelChangeTime = currentTicks;
                        ESP_LOGI(TAG, "Sensor %d (GPIO %d): STABLE_DETECTED -> DEBOUNCING_CLEAN", index, (int)pinNum);
                    }
                    // Don't need to do anything if level == 0
                    break;
            }
        }
    }
}

// --- Public Initialization Function ---
// This is the public api function to be called by main.c
// SHOULD ONLY BE CALLED ONCE PER RUNTIME
esp_err_t ir_sensor_init(ir_sensor_data_t* irSensorArr, int numOfSensors)
{
    esp_err_t ret;
    sIrSensorEvtQueue = xQueueCreate(10, sizeof(uint32_t)); // Create the queue
    // Check for err
    if (sIrSensorEvtQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create IR Sensor event queue");
        return ESP_FAIL;
    }

    // Create the task
    BaseType_t task_created = xTaskCreate(ir_sensor_processing_task,
                              "ir_sensor_processing task",
                               2048,     // Stack size in words
                               NULL,     // Parameter passed to task
                               10,       // Task priority
                               &sIrSensorTaskHandle); // Store task handle
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create IR Sensor processing task");
        vQueueDelete(sIrSensorEvtQueue); // Clean up
        return ESP_FAIL;
    } 

    // Add to static vars
    sNumIrSensorsInternal = (size_t)numOfSensors;
    // Copy data from the caller's array to our internal static array
    for (int i = 0; i < numOfSensors; i++) {
        // This line performs the element-by-element copy
        sIrSensorsData[i] = irSensorArr[i]; // Copies the entire struct content
    }
    
    // Install isr service
    ret = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { //ESP_ERR_INVALID_STATE means already installed
        ESP_LOGE(TAG, "Failed to install GPIO ISR Service Error: %s", esp_err_to_name(ret));
        vQueueDelete(sIrSensorEvtQueue);
        vTaskDelete(sIrSensorTaskHandle);
        return ret;
    }    
    // If it was ESP_ERR_INVALID_STATE, it means another component already installed it, which is fine.
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "GPIO ISR service installed.");
    } else if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "GPIO ISR service already installed.");
    }

    // Add the isr_handler to each gpio pinCan 
    for (int i = 0; i < numOfSensors; i++) {
        ret = gpio_isr_handler_add(irSensorArr[i].pinNum, ir_sensor_isr_handler, (void*)irSensorArr[i].pinNum);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ir_sensor_init error when adding handler to pin num: %d\n error: %s",
                     irSensorArr[i].pinNum,
                    esp_err_to_name(ret));
            vQueueDelete(sIrSensorEvtQueue);
            vTaskDelete(sIrSensorTaskHandle);

            return ret;
        }
        gpio_intr_enable(irSensorArr[i].pinNum);
        // gpio_dump_io_configuration(stdout, (1ULL << irSensorArr[i].pinNum));
        ESP_LOGI(TAG, "Configured IR Sensor on GPIO %d with ISR.", irSensorArr[i].pinNum);
    }
    ESP_LOGI(TAG, "IR Sensor component initialized successfully with %d sensors.", numOfSensors);
    return ESP_OK;
}