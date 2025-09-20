# Copilot guide for homeHydro-S001-ESP-Software

This file teaches AI coding assistants how to work effectively in this repository. It summarizes the architecture, build/run workflow, coding patterns, and gives concrete prompt recipes for common changes.

- Target: ESP32-S2 (ESP-IDF 5.x style drivers detected)
- Language: C++ (with some C files), FreeRTOS
- Build system: ESP-IDF (CMake) via VS Code ESP-IDF extension
- Primary app entrypoint: `main/main.cpp` (function `app_main`)

If you’re an AI assistant, follow the change recipes and conventions below. If you’re a human, feel free to use the prompt examples directly in Copilot Chat.

## Repo at a glance

- Entry: `main/main.cpp`
- Tasks (FreeRTOS):
  - `startup_task` state machine initializes I2C, negotiates power (STUSB4500), then enables I2C slave mode.
  - `flood_task` controls the pump using EC and water-level readings.
  - `data_task` placeholder for periodic sensor refresh.
- Drivers:
  - `drivers/pump_driver.{h,cpp}`: LEDC-based PWM for pump (LEDC_TIMER_0, channel 0 @ 300 kHz)
  - `drivers/status_led_driver.{h,cpp}`: LED status (LEDC_TIMER_1, channels 1-2)
  - `drivers/EC_driver.{h,cpp}`: ADC one-shot for TDS/EC and water level; manages sensor power GPIOs; provides `get_TDS_value()` and `get_water_level()`
  - `drivers/I2C_Slave*.{h,cpp}`: Exposes a small I2C slave register map over I2C0 (SCL=GPIO1, SDA=GPIO2)
  - `drivers/INA219.{h,cpp}`: I2C current/voltage sensor
  - `drivers/STUSB4500*`: USB-PD controller (negotiation when used)
- Pinout: `main/pinout.h` defines all pins, addresses, and simple constants.
- Build glue: `CMakeLists.txt` at root and in `main/` component.

### Hardware assumptions (from `pinout.h`)
- I2C: SCL=GPIO1, SDA=GPIO2 (internal pull-ups enabled in code)
- LEDs: RED=GPIO6, GREEN=GPIO7 (active-low via LEDC with invert flag)
- Pump PWM: GPIO40 (LEDC low speed, timer 0)
- EC/TDS: ADC1 CH8/CH9; sensor power on GPIO11/12
- INA219 address: 0x40; STUSB4500 address: 0x28

## Build, flash, monitor

Use the included VS Code tasks (ESP-IDF extension). Prefer tasks over raw commands.

- Build: task “Build - Build project”
- Clean: task “Clean - Clean the project”
- Set Target: task “Set ESP-IDF Target” (target should be ESP32-S2)
- Flash: task “Flash - Flash the device”
- Monitor: task “Monitor: Start the monitor” (depends on flash)

Optional commands (PowerShell) if you need them; prefer the tasks:

```powershell
# Build
${env:IDF_PYTHON_ENV}; & $env:IDF_PYTHON_ENV $env:IDF_PATH\tools\idf.py build

# Flash and monitor (configure port/baud in VS Code ESP-IDF settings)
${env:IDF_PYTHON_ENV}; & $env:IDF_PYTHON_ENV $env:IDF_PATH\tools\idf.py -p COMx flash
${env:IDF_PYTHON_ENV}; & $env:IDF_PYTHON_ENV $env:IDF_PATH\tools\idf.py -p COMx monitor
```

## Key runtime flow

- `app_main` initializes: `init_startup_task()`, `pump_init()`, `init_ec_driver()`, `init_flood_task()`, `status_led_init()`.
- Main loop toggles red LED, prints TDS, and calls `run_startup_task()` while other tasks run in parallel.
- `startup_task` transitions: INITIAL → 9V_POWER → AWAIT_PI_START → BOOT_COMPLETE (current logic bypasses active PD negotiation; see code comments).
- `I2C_Slave` task handles receive/request callbacks; a simple register map allows a host to read/write state.

## Internal APIs you can rely on

- Pump
  - `void pump_init(void);`
  - `void pump_set_duty(uint8_t duty);  // 0..255`
  - `uint8_t pump_get_duty(void);`
- Status LEDs
  - `void status_led_init(void);`
  - `void status_led_red_set(uint8_t duty);`
  - `void status_led_green_set(uint8_t duty);`
  - `void status_led_red_toggle(void);`
  - `void status_led_green_toggle(void);`
- EC/TDS & Water level
  - `void init_ec_driver(void);`
  - `float get_TDS_value(void);`
  - `uint8_t get_water_level(void);  // percentage`
- Flood control
  - `void init_flood_task(void);`
  - `flood_state_t flood_status(void);`
  - `void begin_flooding(void);`
  - `void stop_flooding(void);`
- I2C Slave Register Map (`drivers/I2C_Slave_basic_registers.*`)
  - CHIP_ID (u8, read-only)
  - VERSION (u8, R/W)
  - PUMP_STATE (u8, R/W duty)
  - WATER_LEVEL (u8, read-only)
  - TDS (float, read-only)
  - FLOOD_STATE (u8, R/W: 1 start, 0 stop)

## Coding conventions and constraints

- Always `ESP_ERROR_CHECK(...)` ESP-IDF calls.
- Avoid double initialization. Several drivers keep a local `initialized` flag.
- Use LEDC timers/channels consistently:
  - Pump uses TIMER_0, CHANNEL_0; Status LEDs use TIMER_1, CHANNEL_1/2.
  - Don’t reuse timers/channels without auditing impacts.
- ADC oneshot is configured once in `init_ec_driver`; reuse the same handle.
- I2C: Startup creates a master bus to probe/configure, then switches to slave mode. Don’t assume both modes are active simultaneously without refactor.
- Keep units in comments and variable names (mV, mS/cm, percent, Hz).
- Logging via `printf` is common here; keep it concise in high-rate loops.

## Change recipes (step-by-step)

### 1) Add a new I2C slave register

Files to touch:
- `drivers/I2C_Slave.h`: extend `i2c_slave_register_t` enum
- `drivers/I2C_Slave_basic_registers.{h,cpp}`: add getter/setter signatures and implementations
- `drivers/I2C_Slave.cpp`: update `i2c_slave_action_functions` table entry with `{get,set,sizeof(type)}` in the correct enum order

Checklist:
- Choose an uninterrupted enum value (before `NUM_REGISTERS`).
- Getter signature: `void get_xxx(void* data)`; Setter: `void set_xxx(void* data)`; use `static_cast<type*>(data)`.
- For multi-byte types (e.g., float/u16 structs), ensure sizes match and endianness is acceptable to the host.
- In setters, validate range and apply using the public driver API, not raw registers.

Prompt example:
- “Add a read-only register for `pump_get_duty()` returning `uint8_t`. Update enum and callback table. Implement `get_pump_duty(void*)` wired to the pump driver.”

### 2) Create a periodic data task that smooths sensor readings

Files to touch:
- `main/tasks/data_task.{h,cpp}`

Steps:
- Implement a FreeRTOS task that periodically calls `get_TDS_value()` and `get_water_level()` and stores smoothed values in module-static variables.
- Provide `get_smoothed_TDS()` and `get_smoothed_water_level()` accessors.
- Rate-limit to avoid excessive ADC reads (e.g., 2–5 Hz), and power the sensors only when needed (use existing power helpers).

Prompt example:
- “Implement an exponential moving average (alpha=0.2) in `data_task` updating at 5 Hz; expose getters and use those getters in flood logic.”

### 3) Improve flood safety logic

Files to touch:
- `main/tasks/flood_task.cpp`

Steps:
- In `FLOODING`, if water level < threshold, stop pump and move to `AWAIT_FLOOD_SIGNAL`.
- Add timeout: after N seconds in FLOODING without adequate water-level increase, stop with an error path.
- Optionally modulate PWM ramp-up/down to reduce current surges.

Prompt example:
- “Add a 60s timeout in FLOODING; if water level hasn’t increased by 10%, stop flooding and return to AWAIT_FLOOD_SIGNAL; log reason.”

### 4) Add a new LED status pattern

Files to touch:
- `drivers/status_led_driver.{h,cpp}` and optionally a small scheduler in a new `status_task`.

Prompt example:
- “Add a `status_led_blink(pattern)` that toggles red/green in alternating 250 ms steps; prefer non-blocking with a FreeRTOS task.”

### 5) Extend PD negotiation (STUSB4500)

Files to touch:
- `tasks/startup_task.cpp`

Notes:
- The current code short-circuits to `STATE_9V_POWER`. To enable negotiation, revise the state flow to `STATE_NEGOTIATE_PD` → `STATE_AWAIT_PD_NEGOTIATION` and validate bus voltage via INA219.

Prompt example:
- “Enable PD negotiation path using `do_state_negotiate_pd` and `do_state_await_pd_negotiation`; factor out I2C register writes to helper functions and add retries.”

## Good prompt patterns

Be explicit about:
- The files to change
- The public API you need (inputs/outputs, units, ranges)
- Timing constraints (task rates, blocking vs non-blocking)
- Test/validation steps

Examples:

- “In `drivers/I2C_Slave_basic_registers.*`, add a R/W `uint8_t` register `PUMP_MAX_DUTY` that bounds `pump_set_duty`. Update table wiring and enforce in the pump driver. Include a unit test stub or a quick runtime assertion.”
- “Create a new `status_task` that flashes RED at 1 Hz when `startup_task` state != `BOOT_COMPLETE`, else GREEN steady. Don’t block other tasks; use `vTaskDelayUntil`.”
- “Refactor `EC_driver` to guard against division by zero and return saturated 0–100% for water level; add comments explaining each constant and TODOs for calibration.”

## Testing and verification

- Build must pass: use the “Build - Build project” task.
- Smoke test via monitor: you should see ‘Hello world!’, TDS prints, periodic LED toggles.
- For I2C slave changes: verify with a host I2C master reading/writing the new register.
- For timing-sensitive code: prefer `xTaskDelayUntil` to maintain fixed cadence.

## Documentation
- Update this README with any new tasks, drivers, or major changes.
- Add comments in code for non-obvious logic, especially around state machines and hardware interactions.
- Follow Doxygen-style comments for public APIs.


## Future enhancements (optional for AI)

- Add `Kconfig.projbuild` under `main/` to make calibration constants configurable at build time (EC electrode geometry, thresholds).
- Add Unity/CMock tests for logic that can be isolated (state transitions, math helpers).
- Replace `printf` with ESP-IDF logging (`ESP_LOGI/W/E`) and configurable log levels.

---

By following this guide, Copilot (and humans!) can make consistent, safe changes that build and run on the first try. When in doubt, prefer small PRs and keep changes local to the relevant driver/task.


