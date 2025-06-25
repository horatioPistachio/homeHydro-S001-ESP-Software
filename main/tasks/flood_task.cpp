/**
 * @file flood_task.cpp
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief  This file uses the pump driver and EC driver to flood the pot but not run the pump dry.
 * @version 0.1
 * @date 2025-04-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "flood_task.h"
#include "../drivers/pump_driver.h"
#include "../drivers/EC_driver.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

TickType_t xLastWakeTime;


typedef struct instance_data{
    int timeout;
} instance_data_t;

typedef flood_state_t state_func_t( instance_data_t *data );

flood_state_t do_state_init( instance_data_t *data );
flood_state_t do_state_await_flood_signal( instance_data_t *data );
flood_state_t do_state_flooding( instance_data_t *data );

state_func_t* const flood_state_table[NUM_STATES] = {
    do_state_init,
    do_state_await_flood_signal,
    do_state_flooding,
};

flood_state_t current_flood_state;
instance_data_t state_data_instance;
instance_data_t *state_data = &state_data_instance;

void init_flood_task()
{
    current_flood_state = INIT;
    state_data->timeout = 0;
    pump_init();
    init_ec_driver();
}

void flood_task()
{
    xLastWakeTime = xTaskGetTickCount();
    while(1)
    {
        current_flood_state = flood_state_table[current_flood_state]( state_data );
        xTaskDelayUntil(&xLastWakeTime, 500 / portTICK_PERIOD_MS);
    }
}


flood_state_t do_state_init( instance_data_t *data )
{
    printf("Flood task initialized\n");
    return AWAIT_FLOOD_SIGNAL;
}

flood_state_t do_state_await_flood_signal( instance_data_t *data )
{
    // wait for flood signal from i2c slave
    // if flood signal is received,
    return AWAIT_FLOOD_SIGNAL; // return to awaiting flood signal state
}

flood_state_t do_state_flooding( instance_data_t *data )
{
    // check for water level to ensure water is in the tank
    uint8_t water_level = get_water_level();
    if (water_level < 10) // if water level is below 10%, stop
    {
        printf("Water level too low, stopping pump\n");
        pump_set_duty(0); // stop the pump
        return AWAIT_FLOOD_SIGNAL; // return to awaiting flood signal state
    }
}

/**
 * @brief Returns the current flood state.
 * 
 * @return flood_state_t 
 */
flood_state_t flood_status()
{
    return current_flood_state;
}

/**
 * @brief Begins the flooding process by setting the pump to full duty.
 * 
 */
void begin_flooding()
{
    printf("Beginning flooding\n");
    current_flood_state = FLOODING;
    pump_set_duty(100); // set pump to full duty
    state_data->timeout = 0; // reset timeout
}

/**
 * @brief Stops the flooding process, mid cycle.
 * 
 */
void stop_flooding()
{
    printf("Prematurely stopping flooding\n");
    pump_set_duty(0); // stop the pump
    current_flood_state = AWAIT_FLOOD_SIGNAL; // return to awaiting flood signal state
}
