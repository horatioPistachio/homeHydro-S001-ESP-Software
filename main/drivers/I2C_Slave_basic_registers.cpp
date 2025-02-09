/**
 * @file I2C_Slave_basic_registers.cpp
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file contains the basic register action functions.
 * @version 0.1
 * @date 2025-01-10
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "I2C_Slave_basic_registers.h"

#include <stdio.h>

#include "../pinout.h"
#include "pump_driver.h"

static uint8_t chip_id = 0x11;

void get_chip_id(uint8_t *data)
{
    *data = CHIP_ID;
}

void get_version(uint8_t *data)
{
    printf("Getting version %d\n", chip_id);
    *data = chip_id;
}

void set_version(uint8_t *data)
{
    printf("Setting version to %d\n", *data);
    chip_id = *data;
}


void get_pump_state(uint8_t *data)
{
    *data = pump_get_duty();
}

void set_pump_state(uint8_t *data)
{
    pump_set_duty(data[0]);
}