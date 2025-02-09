/**
 * @file pump_driver.h
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file contains the driver for the pump.
 * @version 0.1
 * @date 2025-01-10
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef PUMP_DRIVER_H
#define PUMP_DRIVER_H

#include "stdint.h"

void pump_init( void );

void pump_set_duty( uint8_t duty );

uint8_t pump_get_duty( void );

#endif // PUMP_DRIVER_H
