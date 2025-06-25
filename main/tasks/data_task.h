/**
 * @file data_task.h
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file contains the task to constantly refresh the data on the sensors. It allows for quick access during an i2c call.
 * @version 0.1
 * @date 2025-02-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef DATA_TASK_H
#define DATA_TASK_H

#include "stdint.h"

void data_task_init( void );

void data_task_run( void );


#endif // DATA_TASK_H