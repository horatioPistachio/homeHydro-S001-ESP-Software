/**
 * @file I2C_Slave.cpp
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file contains the driver to use the esp32 as an I2C slave device
 * @version 0.1
 * @date 2024-12-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "I2C_Slave.h"

#include "driver/i2c_slave.h"
#include "../pinout.h"

#include "esp_attr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static i2c_slave_dev_handle_t i2c_slave_handle;

#define I2C_RECEIVE_BUFFER_LENGTH 256

static uint8_t i2c_receive_buffer[I2C_RECEIVE_BUFFER_LENGTH];


static IRAM_ATTR bool i2c_slave_rx_done_callback(i2c_slave_dev_handle_t channel, const i2c_slave_rx_done_event_data_t *edata, void *user_data);


void I2C_Slave_init( void )
{
    i2c_slave_config_t i2c_slv_config = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = I2C_SDA,
    .scl_io_num = I2C_SCL,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .send_buf_depth = 256,
    .slave_addr = 0x4B,
    .addr_bit_len = I2C_ADDR_BIT_LEN_7,
    .intr_priority = 1,
    };
    ESP_ERROR_CHECK(i2c_new_slave_device(&i2c_slv_config, &i2c_slave_handle));

    i2c_slave_event_callbacks_t cbs = {
        .on_recv_done = i2c_slave_rx_done_callback,
    };

    ESP_ERROR_CHECK( i2c_slave_register_event_callbacks(i2c_slave_handle, &cbs, NULL));
    ESP_ERROR_CHECK(i2c_slave_receive(i2c_slave_handle, i2c_receive_buffer, I2C_RECEIVE_BUFFER_LENGTH ));

}


static IRAM_ATTR bool i2c_slave_rx_done_callback(i2c_slave_dev_handle_t channel, const i2c_slave_rx_done_event_data_t *edata, void *user_data)
{
   printf("Received a mention\n");
   return 1;
}