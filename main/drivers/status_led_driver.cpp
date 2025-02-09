/**
 * @file status_led_driver.cpp
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief  This file contains the driver for the status LEDs
 * @version 0.1
 * @date 2025-02-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "status_led_driver.h"

#include "driver/ledc.h"
#include "esp_err.h"

#include "../pinout.h"

#define STATUS_LEDC_MODE LEDC_LOW_SPEED_MODE
#define RED_STATUS_LEDC_CHANNEL LEDC_CHANNEL_1
#define GREEN_STATUS_LEDC_CHANNEL LEDC_CHANNEL_2

uint8_t red_duty = 0;
uint8_t green_duty = 0;



void status_led_init( void )
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .timer_num        = LEDC_TIMER_1,
        .freq_hz          = 4000,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK,
        .deconfigure    = false,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel_red = {
        .gpio_num       = RED_LED,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_1,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0,
        .sleep_mode     = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags          = { .output_invert = 1 },
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_red));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel_green = {
        .gpio_num       = GREEN_LED,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_2,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_1,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0,
        .sleep_mode     = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags          = { .output_invert = 1 },
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_green));

    // ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 256));
    // ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
    // printf("Pump duty set to %d\n", 265);

    // ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 256));
    // ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));
}

void status_led_red_set( uint8_t duty )
{
    ESP_ERROR_CHECK(ledc_set_duty(STATUS_LEDC_MODE, RED_STATUS_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(STATUS_LEDC_MODE, RED_STATUS_LEDC_CHANNEL));
    red_duty = duty;
}

void status_led_red_toggle()
{
    if (red_duty == 0)
    {
        status_led_red_set(255);
    }
    else
    {
        status_led_red_set(0);
    }
}

void status_led_green_set( uint8_t duty )
{
    ESP_ERROR_CHECK(ledc_set_duty(STATUS_LEDC_MODE, GREEN_STATUS_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(STATUS_LEDC_MODE, GREEN_STATUS_LEDC_CHANNEL));
    green_duty = duty;
}

void status_led_green_toggle()
{
    if (green_duty == 0)
    {
        status_led_green_set(255);
    }
    else
    {
        status_led_green_set(0);
    }
}



