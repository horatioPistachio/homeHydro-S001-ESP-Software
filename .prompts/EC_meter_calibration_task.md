# EC meter calibration task (implementation prompt)

Implement a dedicated FreeRTOS task to help calibrate the EC/TDS and water-level sensors by periodically logging raw readings and a smoothed value. The task should be easy to enable/disable at compile time and run indefinitely while we perform calibration experiments at different salinity and water levels.

## Objective

- Create a new task `ec_calibration_task` that:
  - Runs every 5 seconds (configurable)
  - Reads EC/TDS and water level via existing driver APIs
  - Prints both RAW and SMOOTHED values to the serial console in a calibration-friendly format
  - Can run indefinitely and is allowed to block other tasks while enabled (simple, single-purpose mode)

## Definitions and constraints

- RAW value: The direct return values from the existing driver calls:
  - `float get_TDS_value()` returns an EC/TDS value in mS/cm (already computed by the driver)
  - `uint8_t get_water_level()` returns a percentage (0–100)
- SMOOTHED value: An exponential moving average (EMA) maintained inside the calibration task only.
  - Default alpha: 0.2 (configurable)
  - First sample initializes the EMA to the RAW value
- Units:
  - TDS in mS/cm (float)
  - Water level in % (uint8_t or float for EMA)
- Timing:
  - Periodic with `vTaskDelayUntil`, default period 5000 ms (configurable)
- Scope:
  - Do NOT change the EC driver behavior; the task only calls its public APIs
  - Keep changes minimal and local (new task files + wire-up)

## Files to add

Create these new files under `main/tasks/`:

- `ec_calibration_task.h`
- `ec_calibration_task.cpp`

API in header:

```cpp
// main/tasks/ec_calibration_task.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void ec_calibration_task_init(void);

#ifdef __cplusplus
}
#endif
```

Source implementation details:

- Include: `freertos/FreeRTOS.h`, `freertos/task.h`, `../drivers/EC_driver.h`, `<stdio.h>`
- Create a FreeRTOS task (e.g., name: "ec_calibration_task", stack: 3072 bytes, priority: 5)
- Use `xTaskDelayUntil` with a `TickType_t` anchor for a 5000 ms period (configurable via `#define`)
- Maintain static module-scope EMA variables for TDS and water level (floats)
- On first loop, initialize EMA = RAW
- Print a single CSV header once on task start for easy copy/paste to a spreadsheet:
  - `timestamp_s,tds_raw_ms_cm,tds_ema_ms_cm,water_raw_pct,water_ema_pct`
- Each iteration print a CSV line plus an optional human-readable line (see Output format below)
- Use `xTaskGetTickCount()` to compute a seconds timestamp (`ticks / configTICK_RATE_HZ`)

## Wiring the task (build and enable flag)

1) Add a compile-time flag in `main/pinout.h` to toggle calibration mode:

```cpp
// pinout.h
// ...existing code...
#ifndef ENABLE_EC_CALIBRATION_TASK
#define ENABLE_EC_CALIBRATION_TASK 0  // set to 1 to enable calibration mode
#endif

#ifndef EC_CAL_TASK_PERIOD_MS
#define EC_CAL_TASK_PERIOD_MS 5000
#endif

#ifndef EC_CAL_TASK_ALPHA
#define EC_CAL_TASK_ALPHA 0.2f
#endif
// ...existing code...
```

2) Update `main/CMakeLists.txt` to include the new sources:
- Add `"tasks/ec_calibration_task.cpp"` to the `idf_component_register(SRCS ...)` list.

3) Update `main/main.cpp`:
- After `init_ec_driver()` (so ADC is configured), and near other task inits, conditionally start the calibration task:

```cpp
// main.cpp
// ...existing code...
#include "tasks/ec_calibration_task.h"
// ...existing code...
    init_ec_driver();
#if ENABLE_EC_CALIBRATION_TASK
    ec_calibration_task_init();
#endif
// ...existing code...
```

Note: When calibration task is enabled, it may block or outpace other tasks; this is acceptable for calibration sessions per requirements.

## Output format (both CSV and readable)

Print header once on task start:
```text
Time (s), TDS Raw (mS/cm), TDS EMA (mS/cm), Water Level Raw (%), Water Level EMA (%)
```

Each iteration logs:
```text
15, 1.83, 1.74, 62, 60.2
[EC CAL] 15s – TDS: 1.83 mS/cm (raw), 1.74 mS/cm (EMA) | Water Level: 62 % (raw), 60.2 % (EMA)
```

- Keep printf efficient: one CSV line and one human-readable line per sample
- EMA update: `ema = alpha * raw + (1 - alpha) * ema`

## Acceptance criteria

- Build passes with `ENABLE_EC_CALIBRATION_TASK = 0` (default) and no behavior change in normal mode.
- With `ENABLE_EC_CALIBRATION_TASK = 1`:
  - The task starts, prints the header once, then logs data every `EC_CAL_TASK_PERIOD_MS` (~5s default)
  - RAW values come directly from `get_TDS_value()` and `get_water_level()`
  - EMA uses `EC_CAL_TASK_ALPHA` and initializes on first sample
  - Task runs indefinitely without crashing; minor blocking of other tasks is acceptable as specified
- No changes to existing driver logic or public APIs
- `main/CMakeLists.txt` includes the new source file so the project links

## Nice-to-haves (optional)

- Add a short banner message on start: `"EC calibration mode ENABLED"`
- Use `ESP_LOGI` instead of `printf` if log tags are preferred (keep output terse)

## Notes

- The EC driver already powers sensors and handles ADC setup. Call its APIs only—do not reconfigure ADC or GPIO here.
- If future work requires true raw ADC codes, we can extend the EC driver with debug getters; out of scope for this task.
- please follow the copilot instructions in `.prompts/copilot-instructions.md` for coding conventions, architecture, and change recipes.