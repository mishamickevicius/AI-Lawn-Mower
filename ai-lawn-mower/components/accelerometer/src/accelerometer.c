#include <stdint.h>
#include <math.h>

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"



#define SCL_PIN_NUM GPIO_NUM_17 // Selected pins 
#define SDA_PIN_NUM GPIO_NUM_18
#define FREQ_HZ 1000
#define I2C_MASTER_TIMEOUT_MS 1000 // Time out length
#define DELTA_TIME 0.0005 // Delta time value in seconds

#define MPU6050_SENSOR_ADDR 0x68 // Address of the sensor
#define MPU6050_ACCELE_CONFIG_ADDR 0x1C // Address of accelerometer config register

#define MPU6050_ACCELE_XOUT_MSB_BYTE_ADDR 0x3B // Address of the XOUT[15:8]
#define MPU6050_ACCELE_XOUT_LSB_BYTE_ADDR 0x3C // Addr of XOUT[7:0]

#define MPU6050_ACCELE_YOUT_MSB_BYTE_ADDR 0x3D // Address of the YOUT[15:8]
#define MPU6050_ACCELE_YOUT_LSB_BYTE_ADDR 0x3E // Addr of YOUT[7:0]

#define MPU6050_ACCELE_ZOUT_MSB_BYTE_ADDR 0x3F // Address of the ZOUT[15:8]
#define MPU6050_ACCELE_ZOUT_LSB_BYTE_ADDR 0x40 // Addr of ZOUT[7:0]



static const char* TAG = "Accelerometer";
static float* previous_velocity; // Pointer to velocity array
/* 
previous_velocity[0] -> X-axis
previous_velocity[1] -> Y-axis
previous_velocity[2] -> Z-axis
*/
static float last_calculated_vel_magnitude = 0;


/*
I2C Connection init function.
This function will be called by the system initilizer to setup the i2c connection
between the esp and accelerometer
*/
esp_err_t i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    // Config the i2c protocol for master(esp)
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SDA_PIN_NUM,
        .scl_io_num = SCL_PIN_NUM,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    // Config a device(sensor) and add it
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = MPU6050_SENSOR_ADDR,
        .scl_speed_hz = FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));

    // Init the velocity array
    previous_velocity = (float*)malloc(3 * sizeof(float));
    if (previous_velocity == NULL) {
        ESP_LOGE(TAG, "Could not allocate previous_velocity array");
        return ESP_ERR_NO_MEM;
    }

    // Init array
    previous_velocity[0] = 0.0;
    previous_velocity[1] = 0.0;
    previous_velocity[2] = 0.0;

    return ESP_OK;
}


esp_err_t read_mpu6050_register(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data)
{
    return i2c_master_transmit_receive(dev_handle, // Target device
                                       &reg_addr, // Target register address
                                       1, // Write size one byte
                                       data, // Result should be placed at data pointer
                                       1, // Read one byte
                                       I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS); // Time out 
}

esp_err_t write_mpu6050_register(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buffer[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle,
                               write_buffer, // Data to write
                               sizeof(write_buffer),
                               I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS); 
}

// This function will calculate and update the velocity 
// of only one component
esp_err_t calculate_velocity_component(i2c_master_dev_handle_t dev_handle, uint8_t msbyte_addr, uint8_t lsbyte_addr, int axis_num) 
{
    int16_t raw_value;
    float acceleration_value;
    uint8_t ms_byte, ls_byte;

    // Read registers
    ESP_ERROR_CHECK(read_mpu6050_register(dev_handle, (uint8_t)msbyte_addr, &ms_byte));
    ESP_ERROR_CHECK(read_mpu6050_register(dev_handle, (uint8_t)lsbyte_addr, &ls_byte));

    // Do calculation
    raw_value = (int16_t)(((uint16_t)ms_byte << 8) | ls_byte);
    acceleration_value = ((float)raw_value / 8192.0) * 9.81; 
    if (axis_num == 2) {acceleration_value -= 9.81;} // Account for gravity if in z-axis

    // Following the equation Vf = Vi + a*t
    previous_velocity[axis_num] = previous_velocity[axis_num] + acceleration_value * DELTA_TIME;

    return ESP_OK;
}

// This function will read accelerometer registers, calculate, and then update
// the previous_velocity array and magnitude 
// Returns the current velocity magnitude
float calculate_velocity(i2c_master_dev_handle_t dev_handle) 
{
    // Calculate components
    ESP_ERROR_CHECK(calculate_velocity_component(dev_handle, MPU6050_ACCELE_XOUT_MSB_BYTE_ADDR, MPU6050_ACCELE_XOUT_LSB_BYTE_ADDR, 0));
    ESP_ERROR_CHECK(calculate_velocity_component(dev_handle, MPU6050_ACCELE_YOUT_MSB_BYTE_ADDR, MPU6050_ACCELE_YOUT_LSB_BYTE_ADDR, 1));
    ESP_ERROR_CHECK(calculate_velocity_component(dev_handle, MPU6050_ACCELE_ZOUT_MSB_BYTE_ADDR, MPU6050_ACCELE_ZOUT_LSB_BYTE_ADDR, 2));

    last_calculated_vel_magnitude =  (float)sqrt(
        (previous_velocity[0] * previous_velocity[0]) + 
        (previous_velocity[1] * previous_velocity[1]) + 
        (previous_velocity[2] * previous_velocity[2])
    );
    
    return last_calculated_vel_magnitude;
}