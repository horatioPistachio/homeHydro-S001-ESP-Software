/**
 * EC Calibration Task
 * Periodically logs EC/TDS and water level (raw and EMA smoothed) for calibration.
 */

#include "ec_calibration_task.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../drivers/EC_driver.h"
#include "../pinout.h"

static void ec_cal_task(void *arg);

void ec_calibration_task_init(void)
{
    // Banner so it's obvious when enabled
    printf("EC calibration mode ENABLED\n");
    xTaskCreate(ec_cal_task, "ec_calibration_task", 3072, NULL, 5, NULL);
}

static void ec_cal_task(void *arg)
{
    // Print CSV header once
    printf("Time (s), TDS Raw (mS/cm), TDS EMA (mS/cm), Water Level Raw (%%), Water Level EMA (%%)\n");

    TickType_t last_wake = xTaskGetTickCount();

    bool first = true;
    float ema_tds = 0.0f;
    float ema_water = 0.0f;
    const float alpha = EC_CAL_TASK_ALPHA;

    while (true)
    {
        // Read raw values
        float tds_raw = get_TDS_value();
        uint8_t water_raw_u8 = get_water_level();
        float water_raw = static_cast<float>(water_raw_u8);

        if (first)
        {
            ema_tds = tds_raw;
            ema_water = water_raw;
            first = false;
        }
        else
        {
            ema_tds = alpha * tds_raw + (1.0f - alpha) * ema_tds;
            ema_water = alpha * water_raw + (1.0f - alpha) * ema_water;
        }

        // Timestamp in seconds
        uint32_t t_s = xTaskGetTickCount() / configTICK_RATE_HZ;

        // CSV line
        printf("%lu, %.3f, %.3f, %u, %.1f\n", (unsigned long)t_s, tds_raw, ema_tds, (unsigned)water_raw_u8, ema_water);
        // Human-readable line
        printf("[EC CAL] %lus – TDS: %.3f mS/cm (raw), %.3f mS/cm (EMA) | Water Level: %u %% (raw), %.1f %% (EMA)\n",
               (unsigned long)t_s, tds_raw, ema_tds, (unsigned)water_raw_u8, ema_water);

        vTaskDelayUntil(&last_wake, EC_CAL_TASK_PERIOD_MS / portTICK_PERIOD_MS);
    }
}
