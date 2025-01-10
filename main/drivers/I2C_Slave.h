/**
 * @file I2C_Slave.h
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file contains the driver to use the esp32 as an I2C slave device
 * @version 0.1
 * @date 2024-12-31
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

typedef enum {
    FIRST_REGISTER = 0,
    CHIP_ID_REGISTER,
    VERSION_REGISTER,

    NUM_REGISTERS,

} i2c_slave_register_t;


void I2C_Slave_init( void );



#endif // I2C_SLAVE_H