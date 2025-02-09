/**
 * @file INA219.cpp
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief  Driver for the INA219 current sensor
 * @version 0.1
 * @date 2024-12-01
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "INA219.h"
#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"


i2c_master_dev_handle_t i2c_dev_primary_handle;


void INA219_init()
{
    // Initialize the INA219    
    i2c_master_bus_config_t i2c_primary_config = 
    {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_2,
        .scl_io_num = GPIO_NUM_1,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0, // Set to zero to prevent async transactions
        .flags = {.enable_internal_pullup = true},
    };

    i2c_master_bus_handle_t i2c_primary_bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_primary_config, &i2c_primary_bus));
    
    i2c_device_config_t i2c_primary_device_config = 
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x40,
        .scl_speed_hz = 100000,
        .scl_wait_us = 1000,
        .flags = {.disable_ack_check = false},
    };    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_primary_bus, &i2c_primary_device_config, &i2c_dev_primary_handle));

    // ESP_ERROR_CHECK(i2c_master_register_event_callbacks(i2c_dev_primary_handle, NULL, NULL));

    // test to see if device is present
    // ESP_ERROR_CHECK(i2c_master_probe(i2c_primary_bus,0x40,1000));
    // for (int i =0;i<0xff; i++)
    // {
    //     esp_err_t e= i2c_master_probe(i2c_primary_bus,i,50);
    //     if (e==ESP_OK)
    //     {
    //         ESP_LOGI("INA219", "Device found at address 0x%02x", i);
    //         while(1);
    //     }
    // }
    // Set the configuration
    const uint8_t config_array[3] = {0x9a,0x99,0x99};
    uint8_t reg = 0x05;
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_dev_primary_handle, &reg,1, 1000));
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_dev_primary_handle, config_array, 3, 1000));
}

uint16_t INA219_getBusVoltage()
{
    // Get the bus voltage
    uint8_t write_buffer[1] = {0x02};
    uint8_t read_buffer[2] = {0,0};

    ESP_ERROR_CHECK(i2c_master_transmit_receive(i2c_dev_primary_handle, write_buffer, 1, read_buffer, 2, -1));
    return  ((uint16_t)(read_buffer[0] << 5) | ((uint16_t)read_buffer[1]) >> 3) * 4;
}


float INA219_getShuntVoltage();

float INA219_getCurrent();

float INA219_getPower();