/**
 * @file EC_driver.h
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file contains the EC meter and water level driver functions.
 * @version 0.1
 * @date 2025-04-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef EC_DRIVER_H
#define EC_DRIVER_H



#include <stdio.h>
#include <stdint.h>

void init_ec_driver(void);

float get_TDS_value(void);
float get_TDS_voltage_raw(void);
float get_water_level_voltage_raw(void);

/**
 * @brief Get the water level object
 * 
 * @return uint8_t the water level in percentage (0-100%)
 */
uint8_t get_water_level(void);



#endif // EC_DRIVER_H