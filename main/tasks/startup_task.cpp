/**
 * @file startup_task.cpp
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief  This file contains the startup task for the system. This should be the first and only task until system is fully initialized
 * @version 0.1
 * @date 2025-01-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "startup_task.h"

#include <stdio.h>

#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "../drivers/I2C_Slave.h"

#include "../pinout.h"

typedef enum { 
                STATE_INITIAL,
                STATE_NEGOTIATE_PD,
                STATE_AWAIT_PD_NEGOTIATION,
                STATE_5V_POWER,
                STATE_9V_POWER,
                STATE_AWAIT_PI_START,
                BOOT_COMPLETE,
                BOOT_TIMEOUT,
                NUM_STATES,
            } state_t;

typedef struct instance_data{
    int timeout;
} instance_data_t;

typedef state_t state_func_t( instance_data_t *data );

state_t do_state_initial( instance_data_t *data );
state_t do_state_negotiate_pd( instance_data_t *data );
state_t do_state_await_pd_negotiation( instance_data_t *data );
state_t do_state_5v_power( instance_data_t *data );
state_t do_state_9v_power( instance_data_t *data );
state_t do_state_await_pi_start( instance_data_t *data );
state_t do_boot_complete( instance_data_t *data );
state_t do_boot_timeout( instance_data_t *data );

state_func_t* const state_table[NUM_STATES] = {
    do_state_initial,
    do_state_negotiate_pd,
    do_state_await_pd_negotiation,
    do_state_5v_power,
    do_state_9v_power,
    do_state_await_pi_start,
    do_boot_complete,
    do_boot_timeout,
};


state_t current_state;

instance_data_t state_data_instance;
instance_data_t *state_data = &state_data_instance;



i2c_master_bus_handle_t i2c_primary_bus;
i2c_master_dev_handle_t stusb4500_i2c_handle;
i2c_master_dev_handle_t ina219_i2c_handle;


void init_startup_task()
{
    current_state = STATE_INITIAL;
    state_data->timeout = 0;
    printf("startup task initialized\n");
    
}

void run_startup_task()
{
    current_state = state_table[current_state]( state_data );
}



state_t do_state_initial( instance_data_t *data )
{
    printf("init state machine \n");

    i2c_master_bus_config_t i2c_primary_config = 
    {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0, // Set to zero to prevent async transactions
        .flags = {.enable_internal_pullup = true},
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_primary_config, &i2c_primary_bus));

    i2c_device_config_t i2c_primary_device_config = 
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x28,
        .scl_speed_hz = 100000,
        .scl_wait_us = 10000,
        .flags = {.disable_ack_check = false},
    };

    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_primary_bus, &i2c_primary_device_config, &stusb4500_i2c_handle));

    i2c_device_config_t i2c_ina219_device_config = 
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = INA219_ADDR,
        .scl_speed_hz = 100000,
        .scl_wait_us = 10000,
        .flags = {.disable_ack_check = false},
    };

    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_primary_bus, &i2c_ina219_device_config, &ina219_i2c_handle));

    return STATE_9V_POWER; // raspberry pi does negotiation
    // return STATE_NEGOTIATE_PD;
    
}


state_t do_state_negotiate_pd( instance_data_t *data )
{
    
    uint8_t buf[16];
    
    buf[0] = 0x2F;
    ESP_ERROR_CHECK(i2c_master_transmit_receive(stusb4500_i2c_handle, buf, 1, buf, 1, 1000));
    printf("Device ID: 0x%02x\n", buf[0]);

    uint8_t pdo_2_buf[] = {0x89, 0xC8, 0xD0, 0x42, 0x00};
    ESP_ERROR_CHECK(i2c_master_transmit(stusb4500_i2c_handle, pdo_2_buf, 5, 1000));

    // ets_delay_us(1000);

    buf[0] = 0x70;
    buf[1] = 0x02;
    ESP_ERROR_CHECK(i2c_master_transmit(stusb4500_i2c_handle, buf, 2, 1000));
    // ets_delay_us(1000);

    buf[0] = 0x51;
    buf[1] = 0x0D;
    ESP_ERROR_CHECK(i2c_master_transmit(stusb4500_i2c_handle, buf, 2, 1000));
    // ets_delay_us(1000);

    buf[0] = 0x1A;
    buf[1] = 0x26;
    ESP_ERROR_CHECK(i2c_master_transmit(stusb4500_i2c_handle, buf, 2, 1000));
    // ets_delay_us(1000);
    printf("negotiating PD\n");
    return STATE_AWAIT_PD_NEGOTIATION;
}

state_t do_state_await_pd_negotiation( instance_data_t *data )
{
    
    uint8_t write_buffer[1] = {0x02};
    uint8_t read_buffer[2] = {0,0};

    ESP_ERROR_CHECK(i2c_master_transmit_receive(ina219_i2c_handle, write_buffer, 1, read_buffer, 2, 1000));
    uint16_t bus_voltage =   ((uint16_t)(read_buffer[0] << 5) | ((uint16_t)read_buffer[1]) >> 3) * 4;
    printf("bus votlage: %d mV\n", bus_voltage);

    if (bus_voltage > 8000 && bus_voltage < 10000)
    {
        return STATE_9V_POWER;
    }
    else if (data->timeout >= 3 && bus_voltage < 6000)
    {
        data->timeout = 0;
        return STATE_5V_POWER;
    }
    else
    {
        data->timeout++;
        return STATE_INITIAL;
    }
    
    

    return current_state;
}

state_t do_state_5v_power( instance_data_t *data )
{
    printf("5v soft start is yet to be implemented\n");
    return current_state;
    
}


state_t do_state_9v_power( instance_data_t *data )
{
    //deinit old i2c driver
    ESP_ERROR_CHECK(i2c_del_master_bus(i2c_primary_bus));

    I2C_Slave_init();
    return STATE_AWAIT_PI_START;
}
state_t do_state_await_pi_start( instance_data_t *data )
{
    //no implementation yet, assume pi starts
    return BOOT_COMPLETE;
}

state_t do_boot_complete( instance_data_t *data )
{
    // do nothing forever
    // in future will use the rtos to turn off this task and start the other tasks
    // the i2c slave runs in the background so should affect anything.
    return current_state;
}
state_t do_boot_timeout( instance_data_t *data ){return current_state;}

