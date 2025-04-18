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
#include "EC_driver.h"

static uint8_t version_id = 0x11;

void get_chip_id(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    *value = CHIP_ID;
}

void get_version(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    *value = version_id;

    printf("Getting version %d\n", version_id);
}

void set_version(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    version_id = *value;

    printf("Setting version to %d\n", *value);
}


void get_pump_state(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    *value = pump_get_duty();
}

void set_pump_state(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    pump_set_duty(value[0]);
}

void get_water_level(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    uint8_t water_level = get_water_level();
    printf("Water level: %d\n", water_level);
    *value = water_level;
}

void get_TDS(void *data)
{
    float* value = static_cast<float*>(data);
    float tds = get_TDS_value();
    printf("TDS: %f\n", tds);
    *value = tds;
}
