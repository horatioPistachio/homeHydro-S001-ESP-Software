/**
 * @file I2C_Slave_basic_registers.h
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file contains the basic register action functions.
 * @version 0.1
 * @date 2025-01-10
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef I2C_SLAVE_BASIC_REGISTERS_H
#define I2C_SLAVE_BASIC_REGISTERS_H

#include <stdint.h>
#include <cstdlib>

// void (*get_slave_data)(uint8_t *data, size_t length);

void get_chip_id(void *data);

void get_version(void *data);

void set_version(void *data);

void get_pump_state(void *data);

void set_pump_state(void *data);

void get_water_level(void *data);

void get_TDS(void *data);

#endif // I2C_SLAVE_BASIC_REGISTERS_H
