/**
 * @file pump_driver.cpp
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file contains the driver for the pump.
 * @version 0.1
 * @date 2025-01-10
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>
#include "pump_driver.h"
#include "driver/ledc.h"
#include "esp_err.h"

#include "../pinout.h"

#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0

static uint8_t initialized = 0; // Flag to check if the pump driver is initialized

/**
 * @brief Initialize the pump driver.
 * 
 */
void pump_init( void )
{
    if (initialized) {
        printf("Pump driver already initialized.\n");
        return; // If already initialized, do nothing
    }
    initialized = 1; // Set the initialized flag to true
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 300000,  // Set output frequency at 300 kHz, this frequency must be kept high. else the pump wont start
        .clk_cfg          = LEDC_AUTO_CLK,
        .deconfigure    = false,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = PUMP_PWM,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0,
        .sleep_mode     = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags          = { .output_invert = 0 },
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}


/**
 * @brief Set the pump duty cycle
 * 
 * @param duty Duty cycle value (0-255)
 */
void pump_set_duty( uint8_t duty )
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    printf("Pump duty set to %d\n", duty);
}

/**
 * @brief Get the current pump duty cycle.
 * 
 * @return uint8_t The current duty cycle value (0-255)
 */
uint8_t pump_get_duty( void )
{
    uint32_t duty = ledc_get_duty(LEDC_MODE, LEDC_CHANNEL);
    return (uint8_t)duty;
}