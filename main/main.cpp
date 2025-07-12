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
#include "drivers/pump_driver.h"
#include "drivers/status_led_driver.h"
#include "drivers/EC_driver.h"

#include "tasks/flood_task.h"
#include "tasks/startup_task.h"


// #include "stusb4500/include/stusb4500.h"

TickType_t xLastWakeTime;


extern "C" void app_main()
{
    printf("Hello world!\n");
    // INA219_init();
    // STUSB4500_init();
    
    // I2C_Slave_init();
    init_startup_task();
    pump_init();
    init_ec_driver();
    init_flood_task();

    status_led_init();

    float tds = get_TDS_value();
    int water_level = get_water_level();

    printf("TDS: %f\n", tds);
    printf("Water level: %d\n", water_level);

    xLastWakeTime = xTaskGetTickCount();
    // begin_flooding();


    while (1)
    {
        // printf("Bus voltage: %d mV\n", INA219_getBusVoltage());
        run_startup_task();

        xTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_PERIOD_MS);
        status_led_red_toggle();
        printf("TDS: %f\n", get_TDS_value());
        

    }
    
}
