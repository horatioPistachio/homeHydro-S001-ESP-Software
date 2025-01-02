/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

#include "drivers/INA219.h"
#include "drivers/STUSB4500.h"
#include "drivers/I2C_Slave.h"

extern "C" void app_main()
{
    printf("Hello world!\n");
    // INA219_init();
    // STUSB4500_init();
    I2C_Slave_init();
    while (1)
    {
        // printf("Bus voltage: %d mV\n", INA219_getBusVoltage());
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}
