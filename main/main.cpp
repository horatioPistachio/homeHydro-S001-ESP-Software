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
#include "drivers/STUSB4500_driver.h"
#include "drivers/I2C_Slave.h"

#include "tasks/startup_task.h"


// #include "stusb4500/include/stusb4500.h"



extern "C" void app_main()
{
    printf("Hello world!\n");
    // INA219_init();
    // STUSB4500_init();
    
    // I2C_Slave_init();
    init_startup_task();


    while (1)
    {
        // printf("Bus voltage: %d mV\n", INA219_getBusVoltage());
        run_startup_task();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}
