/**
 * @file INA219.h
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief Driver for the INA219 current sensor
 * @version 0.1
 * @date 2024-12-01
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef INA219_H
#define INA219_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void INA219_init();


/**
 * @brief Returns eh bus voltage in mV
 * 
 * @return uint16_t 
 */
uint16_t INA219_getBusVoltage();

float INA219_getShuntVoltage();

float INA219_getCurrent();

float INA219_getPower();








#ifdef __cplusplus
}
#endif // __cplusplus


#endif // INA219_H