/**
 * @file status_led_driver.h
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file contains the driver for the status LEDs
 * @version 0.1
 * @date 2025-02-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef STATUS_LED_DRIVER_H
#define STATUS_LED_DRIVER_H

#include "stdint.h"

void status_led_init( void );

void status_led_red_set( uint8_t duty );

void status_led_green_set( uint8_t duty );

void status_led_red_toggle( void );

void status_led_green_toggle( void );


#endif // STATUS_LED_DRIVER_H