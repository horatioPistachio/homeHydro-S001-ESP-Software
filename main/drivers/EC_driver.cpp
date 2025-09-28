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
#include <numeric>
#include <cmath> // <-- Add this for std::sqrt

// Note that these value will need to be calibrated for your specific EC meter and water level sensor
#define EC_DISTANCE_FULL_TANK 1.35 //cm
#define EC_DISTANCE_EMPTY_TANK 3.64 //cm
#define EC_MIN_WATER_LEVEL 0.5 // cm (minimum water level for the EC meter to function properly)
#define EC_ELECTRODE_DIAMETER 0.15 // cm (diameter of the EC electrodes)
#define EC_ELECTRODE_MAX_LENGTH 4.3 // cm (length of the EC electrodes)
#define EC_ELECTRODE_AREA (0.15*4.3) // cm^2 (cross-sectional area of the EC electrodes, 0.15 cm diameter, 4.3 cm length)
#define EC_VOLTAGE 3.3 //v
#define EC_RESISTANCE 1000 // Ohms

#define SAMPLE_SIZE 1 // Number of samples to average for the EC value

static esp_err_t set_TDS_power(uint8_t power);
static esp_err_t set_water_level_power(uint8_t power);
static bool init_adc_calibration(adc_unit_t unit, adc_channel_t channel, adc_cali_handle_t *out_handle, adc_atten_t atten);
static double convert_raw_to_voltage(float average_raw, bool calibrated, adc_cali_handle_t cal_handle, const char *channel_name);

adc_oneshot_unit_handle_t  EC_adc_handle = NULL;
static adc_cali_handle_t tds_cali_handle = NULL;
static adc_cali_handle_t water_level_cali_handle = NULL;
static bool tds_calibrated = false;
static bool water_level_calibrated = false;

static double last_ec_value_seimens = 0.0;
static double last_water_level = 100.0; // Variable to store the last water level value

static uint8_t initialized = 0; // Flag to check if the EC driver is initialized


/**
 * @brief Initialize the EC driver.
 * 
 */
void init_ec_driver(void)
{
    if (initialized) {
        printf("EC driver already initialized.\n");
        return; // If already initialized, do nothing
    }
    initialized = 1; // Set the initialized flag to true
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
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT, // anything other than default will cause a crash. believe default to be 11 bits.
    };
    ESP_ERROR_CHECK (adc_oneshot_config_channel(EC_adc_handle, EC_WATER_LEVEL_ANALOG_IN, &EC_adc_channel_config)); // EC water level channel
    ESP_ERROR_CHECK (adc_oneshot_config_channel(EC_adc_handle, EC_TDS_ANALOG_IN, &EC_adc_channel_config)); // EC TDS channel

    water_level_calibrated = init_adc_calibration(ADC_UNIT_1, EC_WATER_LEVEL_ANALOG_IN, &water_level_cali_handle, EC_adc_channel_config.atten);
    tds_calibrated = init_adc_calibration(ADC_UNIT_1, EC_TDS_ANALOG_IN, &tds_cali_handle, EC_adc_channel_config.atten);

    printf("EC driver initialized.\n");
}

static bool init_adc_calibration(adc_unit_t unit, adc_channel_t channel, adc_cali_handle_t *out_handle, adc_atten_t atten)
{
    if (out_handle == NULL) {
        return false;
    }

    esp_err_t ret = ESP_FAIL;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, out_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, out_handle);
#else
    (void)unit;
    (void)channel;
    (void)atten;
    printf("ADC calibration scheme not supported on this target.\n");
    return false;
#endif

    if (ret == ESP_OK) {
        printf("ADC calibration enabled on channel %d.\n", channel);
        return true;
    }

    if (ret == ESP_ERR_NOT_SUPPORTED) {
        printf("Warning: eFuse not burnt, skipping ADC calibration on channel %d.\n", channel);
    } else {
        printf("Warning: failed to initialize ADC calibration on channel %d (err=0x%x).\n", channel, ret);
    }

    *out_handle = NULL;
    return false;
}

static double convert_raw_to_voltage(float average_raw, bool calibrated, adc_cali_handle_t cal_handle, const char *channel_name)
{
    double voltage = 0.0;
    bool voltage_calibrated = false;

    if (calibrated && cal_handle != NULL) {
        int voltage_mv = 0;
        esp_err_t cal_ret = adc_cali_raw_to_voltage(cal_handle, static_cast<int>(std::lround(average_raw)), &voltage_mv);
        if (cal_ret == ESP_OK) {
            voltage = static_cast<double>(voltage_mv) / 1000.0;
            voltage_calibrated = true;
        } else if (channel_name != nullptr) {
            printf("Warning: adc_cali_raw_to_voltage failed for %s channel (err=0x%x). Falling back to manual conversion.\n", channel_name, cal_ret);
        } else {
            printf("Warning: adc_cali_raw_to_voltage failed (err=0x%x). Falling back to manual conversion.\n", cal_ret);
        }
    }

    if (!voltage_calibrated) {
        voltage = (average_raw * 3.3) / 8191.0;
    }

    return voltage;
}

/**
 * @brief Get the EC meter value in mSeimens/cm
 * 
 * @return float The EC value in mS/cm
 */
float get_TDS_value(void)
{
    int raw_values[SAMPLE_SIZE] = {0};
    // Read the EC value from the EC meter.
    set_TDS_power(1); // Power on the EC meter
    vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for the EC meter to stabilize

    for (int i = 0; i < SAMPLE_SIZE; i++) {  
        vTaskDelay(10 / portTICK_PERIOD_MS); // Wait for the EC meter to stabilize
        ESP_ERROR_CHECK(adc_oneshot_read(EC_adc_handle, EC_TDS_ANALOG_IN, &(raw_values[i]) )); // Read the raw value from the ADC
    }
    set_TDS_power(0); // Power off the EC meter

    // Calculate the sum of all samples
    long long sum = std::accumulate(raw_values, raw_values + SAMPLE_SIZE, 0LL);

    // Calculate the average raw value
    float average_raw = static_cast<float>(sum) / SAMPLE_SIZE;
    
    // --- Start of Standard Deviation Calculation ---
    double sum_of_squared_diff = 0.0;
    for (int i = 0; i < SAMPLE_SIZE; ++i) {
        double diff = raw_values[i] - average_raw;
        sum_of_squared_diff += diff * diff;
    }
    // Calculate the variance (average of the squared differences)
    double variance = sum_of_squared_diff / SAMPLE_SIZE;
    // Calculate the standard deviation (square root of the variance)
    double std_dev = std::sqrt(variance);
    // printf("TDS Raw Stats: Avg=%.2f, StdDev=%.2f\n", average_raw, std_dev);
    // --- End of Standard Deviation Calculation ---

    // Convert the average raw value to voltage (0-3.3V)
    // Note: The ADC bitwidth is 13-bit for ESP32-S2, so the max value is 8191
    double ec_value_volts = convert_raw_to_voltage(average_raw, tds_calibrated, tds_cali_handle, "TDS");
    if (ec_value_volts > 3.2) {
        printf("Warning: resistance reading is very high (%.2f V). Check sensor connections.\n", ec_value_volts);
        return  -1;
    }
    else if (ec_value_volts < 0.1) {
        printf("Warning: resistance reading is very low (%.2f V). Check sensor connections.\n", ec_value_volts);
        return -2;
    }
    double resistance = ec_value_volts * EC_RESISTANCE / (EC_VOLTAGE - ec_value_volts);
     // Calculate the resistivity in Ohms
    // Calculate the EC value in mSeimens/cm
    // EC (mS/cm) = resistivity (Ohms) * cross-sectional area (cm^2) / length (cm)
    double resistivity = EC_ELECTRODE_DIAMETER* EC_ELECTRODE_MAX_LENGTH*resistance*last_water_level/(EC_DISTANCE_EMPTY_TANK+ (EC_DISTANCE_EMPTY_TANK-EC_DISTANCE_FULL_TANK)*(last_water_level + 1)/2.0); // Calculate the resistivity in Ohms
    double EC_seimens = 1/ resistivity * 1000; // Convert to mS/cm (1 S = 1000 mS)
    
    // printf("EC value: %.2f mS/cm\n", EC_seimens); // Debugging output
    
    last_ec_value_seimens = EC_seimens; // Store the last EC value for reference
    return EC_seimens; // Return the EC value in volts
}

/**
 * @brief Get the raw EC probe voltage in volts. Powers the probe, samples
 * the ADC and applies calibration if available.
 *
 * @return float The measured probe voltage in volts.
 */
float get_TDS_voltage_raw(void)
{
    int raw_values[SAMPLE_SIZE] = {0};

    set_TDS_power(1);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    for (int i = 0; i < SAMPLE_SIZE; ++i) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(adc_oneshot_read(EC_adc_handle, EC_TDS_ANALOG_IN, &(raw_values[i])));
    }

    set_TDS_power(0);

    long long sum = std::accumulate(raw_values, raw_values + SAMPLE_SIZE, 0LL);
    float average_raw = static_cast<float>(sum) / SAMPLE_SIZE;

    double voltage = convert_raw_to_voltage(average_raw, tds_calibrated, tds_cali_handle, "TDS");
    return static_cast<float>(voltage);
}

/**
 * @brief Get the raw water level probe voltage in volts. Powers the sensor,
 * samples the ADC and applies calibration if available.
 *
 * @return float The measured water level probe voltage in volts.
 */
float get_water_level_voltage_raw(void)
{
    int raw_values[SAMPLE_SIZE] = {0};

    set_water_level_power(1);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    for (int i = 0; i < SAMPLE_SIZE; ++i) {
        vTaskDelay(20 / portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(adc_oneshot_read(EC_adc_handle, EC_WATER_LEVEL_ANALOG_IN, &(raw_values[i])));
    }

    set_water_level_power(0);

    long long sum = std::accumulate(raw_values, raw_values + SAMPLE_SIZE, 0LL);
    float average_raw = static_cast<float>(sum) / SAMPLE_SIZE;

    double voltage = convert_raw_to_voltage(average_raw, water_level_calibrated, water_level_cali_handle, "water level");
    return static_cast<float>(voltage);
}

/**
 * @brief Calculate the water level in the tank based on the EC value and water level sensor readings.
 * 
 * @return uint8_t The water level as a percentage (from 0 to 100)
 */
uint8_t get_water_level(void)
{
    // Read the water level from the water level sensor.
    if (fabs(last_ec_value_seimens) < 1e-6) {
        get_TDS_value(); // Ensure the EC value is read before calculating water level
    }
    set_water_level_power(1); // Power on the water level sensor
    vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for the water level sensor to stabilize

    int raw_value[SAMPLE_SIZE];
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        vTaskDelay(20 / portTICK_PERIOD_MS); // Wait for the water level sensor to stabilize
        ESP_ERROR_CHECK(adc_oneshot_read(EC_adc_handle, EC_WATER_LEVEL_ANALOG_IN, &(raw_value[i]) )); // Read the raw value from the ADC
    }
    set_water_level_power(0); // Power off the water level sensor

    // Calculate the sum of all samples
    long long sum = std::accumulate(raw_value, raw_value + SAMPLE_SIZE, 0LL);
    // Calculate the average raw value
    float average_raw = static_cast<float>(sum) / SAMPLE_SIZE;

    // --- Standard Deviation Calculation for Water Level ---
    double sum_of_squared_diff = 0.0;
    for (int i = 0; i < SAMPLE_SIZE; ++i) {
        double diff = raw_value[i] - average_raw;
        sum_of_squared_diff += diff * diff;
    }
    double variance = sum_of_squared_diff / SAMPLE_SIZE;
    double std_dev = std::sqrt(variance);
    // printf("Water Level Raw Stats: Avg=%.2f, StdDev=%.2f\n", average_raw, std_dev);
    // --- End of Calculation ---

    // Convert the average raw value to voltage (0-3.3V)
    double water_level_voltage = convert_raw_to_voltage(average_raw, water_level_calibrated, water_level_cali_handle, "water level");

    double resistance = water_level_voltage * EC_RESISTANCE / (EC_VOLTAGE - water_level_voltage);
    double resistivity_numerator = EC_ELECTRODE_DIAMETER* EC_ELECTRODE_MAX_LENGTH*resistance*last_water_level;
    double resistivity_denominator = EC_DISTANCE_EMPTY_TANK+ (EC_DISTANCE_EMPTY_TANK-EC_DISTANCE_FULL_TANK)*(last_water_level + 1)/2.0; // Calculate the resistivity in Ohms
    double water_level = 0.0;
    if (fabs(resistivity_denominator) < 1e-6) {
        printf("Warning: Division by zero avoided in water level calculation.\n");
        water_level = 0.0; // or set to a default/fallback value
    } else {
        water_level = 100 * resistivity_numerator / resistivity_denominator; // Calculate the water level in percentage
    }
    last_water_level = water_level; // Store the last water level value for reference
    return (uint8_t)water_level; // Return the water level in percentage
    
}


/**
 * @brief Set the power state of the EC meter. This must be powered on before reading values.
 * 
 * @param power 1 to power on, 0 to power off
 * @return esp_err_t ESP_OK on success
 */
static esp_err_t set_TDS_power(uint8_t power)
{
    // Set the power for the EC meter
    return gpio_set_level(EC_TDS_POWER, power);
}

/**
 * @brief Set the power state of the water level sensor. This must be powered on before reading values.
 * 
 * @param power 1 to power on, 0 to power off
 * @return esp_err_t ESP_OK on success
 */
static esp_err_t set_water_level_power(uint8_t power)
{
    // Set the power for the water level sensor
    return gpio_set_level(EC_WATER_LEVEL_POWER, power);
}