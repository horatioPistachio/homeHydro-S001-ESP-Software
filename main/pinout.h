/**
 * @file pinout.h
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file contains definitions for the pinout of the ESP32.
 * @version 0.1
 * @date 2024-12-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#ifndef PINOUT_H
#define PINOUT_H

#include "driver/gpio.h"

#define CHIP_ID 0x45
#define VERSION 0x01

#define I2C_SCL GPIO_NUM_1
#define I2C_SDA GPIO_NUM_2

#define ST_USB_RST GPIO_NUM_3
#define ST_USB_ALERT GPIO_NUM_5

#define EC_WATER_LEVEL_ANALOG_IN ADC_CHANNEL_8 //GPIO_NUM_9
#define EC_TDS_ANALOG_IN ADC_CHANNEL_9 //GPIO_NUM_10
#define EC_TDS_POWER GPIO_NUM_11
#define EC_WATER_LEVEL_POWER GPIO_NUM_12

#define PUMP_PWM GPIO_NUM_40
#define FAN_1_PWM GPIO_NUM_41
#define FAN_2_PWM GPIO_NUM_42

#define RED_LED GPIO_NUM_6
#define GREEN_LED GPIO_NUM_7

#define INA219_ADDR 0x40
#define STUSB4500_ADDR 0x28

// Calibration task compile-time controls (safe defaults)
#ifndef ENABLE_EC_CALIBRATION_TASK
#define ENABLE_EC_CALIBRATION_TASK 0  //set to 1 to enable calibration mode
#endif

#ifndef EC_CAL_TASK_PERIOD_MS
#define EC_CAL_TASK_PERIOD_MS 10000
#endif

#ifndef EC_CAL_TASK_ALPHA
#define EC_CAL_TASK_ALPHA 0.2f
#endif

#ifndef PUMP_USE_PWM
#define PUMP_USE_PWM 0
#endif

#endif // PINOUT_H