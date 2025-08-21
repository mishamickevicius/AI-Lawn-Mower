#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include "driver/i2c_master.h"

esp_err_t i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);
esp_err_t read_mpu6050_register(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data);
esp_err_t write_mpu6050_register(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data);
esp_err_t calculate_velocity_component(i2c_master_dev_handle_t dev_handle, uint8_t msbyte_addr, uint8_t lsbyte_addr, int axis_num);
float calculate_velocity(i2c_master_dev_handle_t dev_handle);


#endif