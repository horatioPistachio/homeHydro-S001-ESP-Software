/**
 * @file EC_driver.cpp
 * @author Conor Barry (conor@horatiopistachio.com)
 * @brief This file contains the EC meter and water level driver functions.
 * @version 0.1
 * @date 2025-04-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "EC_driver.h"
#include "../pinout.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static esp_err_t set_TDS_power(uint8_t power);
static esp_err_t set_water_level_power(uint8_t power);

adc_oneshot_unit_handle_t  EC_adc_handle = NULL;

float last_ec_value = 0.0;

void init_ec_driver(void)
{
    // Initialize the EC driver here.

    // configure powered GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << EC_TDS_POWER) | (1ULL << EC_WATER_LEVEL_POWER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // configure ADC for EC meter and water level sensor
    adc_oneshot_unit_init_cfg_t EC_adc_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK (adc_oneshot_new_unit(&EC_adc_config, &EC_adc_handle));

    adc_oneshot_chan_cfg_t EC_adc_channel_config = {
        .atten = ADC_ATTEN_DB_0,
        .bitwidth = ADC_BITWIDTH_DEFAULT, // anything other than default will cause a crash. believe default to be 11 bits.
    };
    ESP_ERROR_CHECK (adc_oneshot_config_channel(EC_adc_handle, EC_WATER_LEVEL_ANALOG_IN, &EC_adc_channel_config)); // EC water level channel
    ESP_ERROR_CHECK (adc_oneshot_config_channel(EC_adc_handle, EC_TDS_ANALOG_IN, &EC_adc_channel_config)); // EC TDS channel


    printf("EC driver initialized.\n");
}

float get_TDS_value(void)
{
    // Read the EC value from the EC meter.
    set_TDS_power(1); // Power on the EC meter
    vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for the EC meter to stabilize
    
    int raw_value = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(EC_adc_handle, EC_TDS_ANALOG_IN, &raw_value)); // Read the raw value from the ADC
    set_TDS_power(0); // Power off the EC meter

    float ec_value = (float)raw_value * 3.3 / 8191; // Convert the raw value to voltage (0-3.3V)
    last_ec_value = ec_value; // Store the last EC value for reference, might need to filter this in the future.
    return ec_value; // Return the EC value in volts

}

uint8_t get_water_level(void)
{
    // Read the water level from the water level sensor.
    if (last_ec_value == 0.0) {
        get_TDS_value(); // Ensure the EC value is read before calculating water level
    }
    set_water_level_power(1); // Power on the water level sensor
    vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for the water level sensor to stabilize

    int raw_value = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(EC_adc_handle, EC_WATER_LEVEL_ANALOG_IN, &raw_value)); // Read the raw value from the ADC
    set_water_level_power(0); // Power off the water level sensor

    float water_level_voltage = ((float)raw_value * 3.3 / 8191); // Convert the raw value to percentage (0-100%)
    uint8_t water_level = 100 - ((last_ec_value - water_level_voltage) / last_ec_value * 100); // Calculate the water level in percentage
    return water_level; // Return the water level in percentage
}

static esp_err_t set_TDS_power(uint8_t power)
{
    // Set the power for the EC meter
    return gpio_set_level(EC_TDS_POWER, power);
}

static esp_err_t set_water_level_power(uint8_t power)
{
    // Set the power for the water level sensor
    return gpio_set_level(EC_WATER_LEVEL_POWER, power);
}