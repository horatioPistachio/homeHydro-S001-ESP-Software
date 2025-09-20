| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 | Linux |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- | ----- |

# homeHydro-S001-ESP-Software

Firmware for the homeHydro S001 controller based on ESP32-S2 using ESP-IDF. It manages pump control, EC/TDS measurement, water level sensing, status LEDs, optional USB-PD negotiation, and exposes a simple I2C slave interface for a host (e.g., Raspberry Pi). It interfaces with a raspberry pi zero w which run the main homeHydro softwre.

- Target MCU: ESP32-S2
- Framework: ESP-IDF (CMake), FreeRTOS
- Language: C++ (with some C)
- Entrypoint: `main/main.cpp` (`app_main`)

## Features

- Startup state machine initializes I2C and power negotiation (STUSB4500) and then enables I2C slave mode.
- Flood controller task drives the pump with safeguards based on water level.
- EC/TDS and water level sensing via ADC one-shot with sensor power gating.
- Status LEDs (red/green) via LEDC PWM.
- I2C slave register map to read/write key system parameters from a host.

## Getting started

Prerequisites:
- VS Code with the Espressif IDF extension configured for ESP32-S2
- Serial port configured in the extension settings

Use the built-in VS Code tasks (recommended):
- Build: “Build - Build project”
- Clean: “Clean - Clean the project”
- Set target: “Set ESP-IDF Target” (ESP32-S2)
- Flash: “Flash - Flash the device”
- Monitor: “Monitor: Start the monitor” (depends on Flash)

Optional PowerShell commands (prefer the tasks above):
```powershell
# Build
${env:IDF_PYTHON_ENV}; & $env:IDF_PYTHON_ENV $env:IDF_PATH\tools\idf.py build

# Flash and monitor (configure COMx and baud in the extension settings)
${env:IDF_PYTHON_ENV}; & $env:IDF_PYTHON_ENV $env:IDF_PATH\tools\idf.py -p COMx flash
${env:IDF_PYTHON_ENV}; & $env:IDF_PYTHON_ENV $env:IDF_PATH\tools\idf.py -p COMx monitor
```

## Runtime overview

On boot `app_main` initializes:
- `init_startup_task()` to run the startup state machine
- `pump_init()` for pump PWM
- `init_ec_driver()` to configure ADC and sensor power GPIOs
- `init_flood_task()` to start the pump control task
- `status_led_init()` for LEDs

The main loop toggles the red LED, prints TDS and water level periodically, and advances the startup state machine while other tasks run concurrently.

## Architecture

- Tasks
  - `startup_task` (`main/tasks/startup_task.*`): state machine that initializes I2C, can perform STUSB4500 PD negotiation, then switches to I2C slave mode and waits for host.
  - `flood_task` (`main/tasks/flood_task.*`): controls the pump; ensures safe operation based on water level. Provides `begin_flooding`, `stop_flooding`, and `flood_status`.
  - `data_task` (`main/tasks/data_task.*`): placeholder for future periodic sensor smoothing/processing.
- Drivers
  - Pump (`main/drivers/pump_driver.*`): LEDC PWM @ 300 kHz (Timer0, Channel0) on `PUMP_PWM`.
  - Status LEDs (`main/drivers/status_led_driver.*`): LEDC Timer1, channels 1 and 2 with invert flags; red and green.
  - EC/TDS & water level (`main/drivers/EC_driver.*`): ADC one-shot; powers sensors via GPIO, provides `get_TDS_value()` and `get_water_level()`.
  - I2C Slave (`main/drivers/I2C_Slave*.{h,cpp}`): exposes a compact register map on I2C0 for host access.
  - INA219 (`main/drivers/INA219.*`): current/voltage sensor on I2C.
  - STUSB4500 (`main/drivers/STUSB4500*.{c,cpp}`): USB-PD controller used by startup state machine when enabled.
- Pinout (`main/pinout.h`)
  - I2C: SCL=GPIO1, SDA=GPIO2 (internal pull-ups enabled)
  - LEDs: RED=GPIO6, GREEN=GPIO7 (active-low via LEDC invert flags)
  - Pump PWM: GPIO40
  - EC/TDS ADC: ADC1_CH8 (GPIO9), ADC1_CH9 (GPIO10)
  - Sensor power: EC_TDS_POWER=GPIO11, EC_WATER_LEVEL_POWER=GPIO12
  - INA219 I2C address: 0x40; STUSB4500 address: 0x28

## I2C slave interface

The I2C slave enumerates at address 0x4B and supports a small register map. Access pattern:
- Host writes a single byte with the register ID to select a register
- For writes: host then writes payload bytes (if applicable)
- For reads: host performs a read of the register-sized payload

Registers (see `main/drivers/I2C_Slave.h` and `I2C_Slave_basic_registers.*`):
- CHIP_ID (u8, read-only)
- VERSION (u8, read/write)
- PUMP_STATE (u8, read/write: PWM duty 0–255)
- WATER_LEVEL (u8, read-only: percent 0–100)
- TDS (float, read-only)
- FLOOD_STATE (u8, read/write: 1=start flooding, 0=stop)

Note: For multi-byte values (e.g., float), the host should match the MCU endianness when interpreting bytes.

## Calibration and configuration

- EC/TDS and water-level calibration constants are defined in `main/drivers/EC_driver.cpp` as macros. Adjust for your sensors.
- Consider migrating constants to Kconfig for build-time configuration in future revisions.

## Troubleshooting

- Flashing/monitoring issues:
  - Verify the correct COM port and baud in the ESP-IDF extension settings.
  - Try lowering flash baud if downloads fail.
  - Use the Monitor task to view boot logs and confirm startup.
- I2C communication issues:
  - Ensure pull-ups are present/working for SCL/SDA.
  - Confirm the device address (0x4B) and transaction sequence (select register before read).
- Pump/LED not responding:
  - Check LEDC timer/channel conflicts; the project assigns Timer0/Ch0 for pump and Timer1/Ch1–2 for LEDs.

## Development notes

- Prefer `ESP_ERROR_CHECK(...)` for ESP-IDF API calls.
- Avoid double initialization; drivers track an internal `initialized` flag.
- Use `xTaskDelayUntil` for periodic work to maintain stable rates.
- See `COPILOT.md` for detailed change recipes and prompt examples tailored to this codebase.

## License

This project is provided as-is. Refer to individual source file headers for copyright.
