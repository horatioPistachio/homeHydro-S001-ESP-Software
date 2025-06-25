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

#include "../tasks/flood_task.h"

static uint8_t version_id = 0x11;


/**
 * @brief Get the chip id object
 * 
 * @param data 
 */
void get_chip_id(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    *value = CHIP_ID;
}

/**
 * @brief Gets the current firmware version
 * 
 * @param data Pointer to store the version value (uint8_t)
 */
void get_version(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    *value = version_id;

    printf("Getting version %d\n", version_id);
}

/**
 * @brief Sets a new firmware version
 * 
 * @param data Pointer to the new version value (uint8_t)
 */
void set_version(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    version_id = *value;

    printf("Setting version to %d\n", *value);
}

/**
 * @brief Gets the current pump duty cycle
 * 
 * @param data Pointer to store the pump duty cycle value (uint8_t)
 */
void get_pump_state(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    *value = pump_get_duty();
}

/**
 * @brief Sets the pump duty cycle
 * 
 * @param data Pointer to the new duty cycle value (uint8_t)
 */
void set_pump_state(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    pump_set_duty(value[0]);
}

/**
 * @brief Gets the current water level
 * 
 * @param data Pointer to store the water level value (uint8_t)
 */
void get_water_level(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    uint8_t water_level = get_water_level();
    printf("Water level: %d\n", water_level);
    *value = water_level;
}

/**
 * @brief Gets the current TDS (Total Dissolved Solids) value
 * 
 * @param data Pointer to store the TDS value (float)
 */
void get_TDS(void *data)
{
    float* value = static_cast<float*>(data);
    float tds = get_TDS_value();
    printf("TDS: %f\n", tds);
    *value = tds;
}

/**
 * @brief Controls the flood state of the system
 * 
 * @param data Pointer to the flood state value (uint8_t): 1 to start flooding, 0 to stop
 */
void set_flood_state(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    if (*value == 1)
    {
        printf("Flood state set to ON\n");
        // Here you would typically activate a flood state, e.g., start a pump or open a valve.
        begin_flooding(); // Assuming this starts the flood task which handles flooding logic.
    }
    else
    {
        printf("Flood state set to OFF\n");
        // Here you would typically deactivate the flood state, e.g., stop a pump or close a valve.
        stop_flooding(); // Assuming this stops the flood task or pump.
    }
}

/**
 * @brief Gets the current flood state of the system
 * 
 * @param data Pointer to store the flood state value (uint8_t)
 */
void get_flood_state(void *data)
{
    uint8_t* value = static_cast<uint8_t*>(data);
    *value = flood_status();
}
