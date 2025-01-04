/**
 * @file STUSB4500.cpp
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief  Driver for the STUSB4500 USB PD controller
 * @version 0.1
 * @date 2024-12-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "STUSB4500_driver.h"
#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "stusb4500.h"




static i2c_master_dev_handle_t i2c_dev_primary_handle;

bool my_stusb4500_write(uint16_t addr, uint8_t reg, void const* buf, size_t len, void* context)
{   
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_dev_primary_handle, (uint8_t*)buf, len, 1000));
    
    return true;
}

bool my_stusb4500_read(uint16_t addr, uint8_t reg, void* buf, size_t len, void* context)
{
    ESP_ERROR_CHECK(i2c_master_transmit_receive(i2c_dev_primary_handle, (uint8_t*)buf, len, (uint8_t*)buf, len, 1000));
    return true;
}

void STUSB4500_init()
{
    // Initialize the STUSB4500
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
        .device_address = 0x28,
        .scl_speed_hz = 10000,
        .scl_wait_us = 10000,
        .flags = {.disable_ack_check = false},
    };

    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_primary_bus, &i2c_primary_device_config, &i2c_dev_primary_handle));

    gpio_config_t io_conf =
    {
        .pin_bit_mask = (1ULL << GPIO_NUM_5 ),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));


    stusb4500_t my_stusb4500 = {
        .addr = 0x28,
        .write = my_stusb4500_write,
        .read = my_stusb4500_read,
        .context = NULL
    };

    uint8_t buf[64];
    stusb4500_nvm_read(&my_stusb4500, buf);
    for (int i = 0; i < 40; i++)
    {
        printf("0x%02x ", buf[i]);
        if ((i+1)%8 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
    printf("nvm completed\n");

    // stusb4500_nvm_write(&my_stusb4500, buf);
}


//     uint8_t reg = 0x2F;
//     uint8_t data = 0x00;



//     ESP_ERROR_CHECK(i2c_master_transmit_receive(i2c_dev_primary_handle, &reg, 1, &data, 1, 1000));

//     ESP_LOGI("STUSB4500", "Device ID: 0x%02x", data);
// }