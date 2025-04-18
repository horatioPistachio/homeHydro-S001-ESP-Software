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

#include <cstring>
#include <array>

#include "I2C_Slave_basic_registers.h"

static i2c_slave_dev_handle_t i2c_slave_handle;

#define I2C_RECEIVE_BUFFER_LENGTH 256

static uint8_t i2c_receive_buffer[I2C_RECEIVE_BUFFER_LENGTH];

static uint8_t i2c_process_buffer[I2C_RECEIVE_BUFFER_LENGTH];

typedef enum
{
    I2C_RECEIVE,
    I2C_REQUEST,
} i2c_event_t;

typedef struct
{
    uint8_t i2c_data[I2C_RECEIVE_BUFFER_LENGTH];
    size_t length;
    i2c_event_t event;
} i2c_event_data_t;

QueueHandle_t i2c_slave_queue;

typedef struct
{
    void (*get_slave_data)(void *data);
    void (*set_slave_data)(void *data);
    size_t register_size;
} i2c_slave_action_functions_t;



/* Callback table for the i2c commands */
/* {getter, setter, length} */
std::array<i2c_slave_action_functions_t, NUM_REGISTERS> i2c_slave_action_functions = {
    {
        {nullptr, nullptr, 0},                  // FIRST_REGISTER
        {get_chip_id, nullptr, sizeof(uint8_t)},              // CHIP_ID_REGISTER
        {get_version, set_version, sizeof(uint8_t)},          // VERSION_REGISTER
        {get_pump_state, set_pump_state, sizeof(uint8_t)},    // PUMP_STATE_REGISTER
        {get_water_level, nullptr, sizeof(uint8_t)},          // WATER_LEVEL_REGISTER
        {get_TDS, nullptr, sizeof(float)},      // TDS_REGISTER
    }};

i2c_slave_action_functions_t *active_register;


static IRAM_ATTR bool i2c_slave_receive_callback(i2c_slave_dev_handle_t channel, const i2c_slave_rx_done_event_data_t *edata, void *user_data);
static IRAM_ATTR bool i2c_slave_request_callback(i2c_slave_dev_handle_t channel, const i2c_slave_request_event_data_t *edata, void *user_data);
static void i2c_slave_task(void *arg);

void I2C_Slave_init(void)
{

    i2c_slave_queue = xQueueCreate(3, sizeof(i2c_event_data_t));
    if (i2c_slave_queue == NULL)
    {
        printf("Failed to create queue\n");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    i2c_slave_config_t i2c_slv_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .send_buf_depth = 256,
        .receive_buf_depth = 256,
        .slave_addr = 0x4B,
        .addr_bit_len = I2C_ADDR_BIT_LEN_7,
        .intr_priority = 1,

    };
    ESP_ERROR_CHECK(i2c_new_slave_device(&i2c_slv_config, &i2c_slave_handle));

    i2c_slave_event_callbacks_t cbs = {
        .on_request = i2c_slave_request_callback,
        .on_receive = i2c_slave_receive_callback,
    };

    ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(i2c_slave_handle, &cbs, NULL));
    // ESP_ERROR_CHECK(i2c_slave_receive(i2c_slave_handle, i2c_receive_buffer, I2C_RECEIVE_BUFFER_LENGTH ));

    xTaskCreate(i2c_slave_task, "i2c_slave_task", 2048, NULL, 10, NULL);
}

static IRAM_ATTR bool i2c_slave_receive_callback(i2c_slave_dev_handle_t channel, const i2c_slave_rx_done_event_data_t *edata, void *user_data)
{
    i2c_event_data_t event_data;
    event_data.event = I2C_RECEIVE;
    event_data.length = edata->length;
    memcpy(event_data.i2c_data, edata->buffer, edata->length);
    xQueueSendFromISR(i2c_slave_queue, &event_data, NULL);
    return 1;
}

static IRAM_ATTR bool i2c_slave_request_callback(i2c_slave_dev_handle_t channel, const i2c_slave_request_event_data_t *edata, void *user_data)
{
    // printf("Received a request\n");
    i2c_event_data_t event_data;
    event_data.event = I2C_REQUEST;
    event_data.length = 0;
    memcpy(event_data.i2c_data, edata, I2C_RECEIVE_BUFFER_LENGTH);
    xQueueSendFromISR(i2c_slave_queue, &event_data, NULL);
    return 1;
}

static void i2c_slave_task(void *arg)
{

    while (true)
    {
        i2c_event_data_t i2c_event_data;
        if (xQueueReceive(i2c_slave_queue, &i2c_event_data, 10) == pdTRUE)
        {
            printf("Received data: %d of %x %x %x\n", i2c_event_data.event, i2c_event_data.i2c_data[0], i2c_event_data.i2c_data[1], i2c_event_data.i2c_data[2]);

            if (i2c_event_data.length != 0)
            {
                // Recieved a new register request.
                // printf("Received a new register request: %d\n", i2c_event_data.i2c_data[0]);
                i2c_slave_register_t i2c_register = (i2c_slave_register_t)i2c_event_data.i2c_data[0];
                if (i2c_register < NUM_REGISTERS)
                {
                    active_register = &(i2c_slave_action_functions.at(i2c_register));
                }
                else
                {
                    // Register is out of range. just ignore it
                    continue;
                }
            }

            if ((i2c_event_data.event == I2C_RECEIVE) && (active_register->set_slave_data) && (i2c_event_data.length > 1))
            {
                active_register->set_slave_data(i2c_event_data.i2c_data + 1);
            }
            else if (i2c_event_data.event == I2C_REQUEST && active_register->get_slave_data)
            {
                active_register->get_slave_data(i2c_process_buffer);
                uint32_t write_length = 0;
                ESP_ERROR_CHECK(i2c_slave_write(i2c_slave_handle, i2c_process_buffer, active_register->register_size, &write_length, 1000));
            }
        }
    }
}