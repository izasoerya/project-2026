# SIBOB Project Quick Guide

This document focuses on high-level operation settings for SIBOB:

- Exposed API through Web Server and WebSerial
- Automation-related configuration (bang-bang, heater behavior)
- How to switch device env (SIBOB_1 vs SIBOB_2)
- Where to change task intervals

## 1) Build Target and Device Variant

PlatformIO environment for this project is:

- `env:sibob` in `platformio.ini`

Device variant is selected in `src/projects/sibob/main.cpp`:

- `#define SIBOB_1`
- `#define SIBOB_2`

Use only one active define at a time.

What changes between variants:

- Hostname (`sibob-1` / `sibob-2`)
- WireGuard local IP/private key selection
- Sensor conversion formulas (air/soil/PH/weight)
- `device_id` payload value

## 2) Exposed Web Server API

Base server runs on port 80.

### GET /

Returns latest sensor snapshot as JSON.

Main fields:

- `device_id`
- `created_at`
- `temperature_soil`
- `humidity_soil`
- `ph`
- `weight_breed`
- `weight_yield`
- `weight`
- `temperature_air`
- `humidity_air`
- `status`

### GET /restart

Returns plain text response and then restarts device.

### OTA

ElegantOTA is enabled on this project. Keep using the existing OTA URL/path already used in your deployment workflow.

## 3) Exposed WebSerial API

WebSerial endpoint path:

- `/webserial`

### Commands

- `ADS_CALIBRATE`
  - Enter ADS calibration mode (PH + soil humidity raw ADS debug output).
- `HX711_CALIBRATE`
  - Enter HX711 calibration mode (raw load-cell debug output).
- `DISABLE_SUPABASE`
  - Stop cloud transmit flag.
- `ENABLE_SUPABASE`
  - Enable cloud transmit flag.
- `NORMAL`
  - Exit manual/calibration mode and resume normal automation.
- `FAN=<0..255>`
  - Manual PWM control for fan and sets manual mode.
- `MIST=<0..255>`
  - Manual PWM control for mist and sets manual mode.
- `HEATER=<0..255>`
  - Manual PWM control for heater and sets manual mode.

Notes:

- Unknown key format returns `Unknown command`.
- Non-matching input returns `Invalid command`.

## 4) Automation Config (High-Level)

### Bang-bang setpoints

Change these in `src/projects/sibob/main.cpp`:

- `TOP_TEMP_SET`
- `BOT_TEMP_SET`
- `TOP_HUM_SET`
- `BOT_HUM_SET`

Behavior:

- Fan: ON when temperature above top temp, OFF when below bottom temp.
- Mist: ON when humidity below bottom hum, OFF when above top hum.

### Heater control (intermittent style)

Heater logic runs in automation loop using hysteresis and a flip flag:

- Heater demand ON when temperature < bottom temp.
- Heater demand OFF when temperature >= top temp.
- `flipFlag` alternates each control cycle and forces OFF every other cycle.

This creates intermittent heater actuation in normal mode.

Automation is bypassed when:

- Manual mode is active (after `FAN=...`, `MIST=...`, `HEATER=...`)
- ADS/HX711 calibration mode is active

## 5) Where to Change Task Timing

All below are in `src/projects/sibob/main.cpp`.

- Sensor + local log loop period:
  - `if (millis() - lastLogLocal >= 3000)` -> currently 3 seconds
- Bang-bang/heater control period:
  - `if (millis() - lastTimerHeater >= 10000)` -> currently 10 seconds
- ADS calibration debug print period:
  - `if (millis() - lastLogADSDebug >= 200)` -> currently 200 ms
- HX711 calibration debug print period:
  - `if (millis() - lastLogHX711Debug >= 200)` -> currently 200 ms
- Supabase push task period:
  - `vTaskDelay(60000 / portTICK_PERIOD_MS)` -> currently 60 seconds
- HX711 sampling task period:
  - `vTaskDelay(1000 / portTICK_PERIOD_MS)` -> currently 1 second
- Other task scheduler tick:
  - `vTaskDelay(10 / portTICK_PERIOD_MS)` -> currently 10 ms

## 6) Scope of This Guide

This guide intentionally prioritizes high-level behavior and API exposure.
Low-level pinout and hardware wiring are intentionally excluded.
