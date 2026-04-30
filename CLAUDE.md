# Project: ESP8266 LED Matrix NTP Clock

## Overview

NTP-synchronized clock displaying time, date, temperature, and humidity on a 32×16 LED matrix.
Features automatic brightness via LDR, PIR motion detection for power saving, web-based
configuration interface with live display mirror, and comprehensive timezone support (88 global
cities). Includes scheduled display on/off times and OTA firmware updates.

---

## Hardware

- MCU: ESP8266 D1 Mini Pro (WeMos)
- Peripherals:
  - 8× MAX7219 LED matrix modules (4×2 arrangement = 32×16 pixels)
  - BME280 environmental sensor (I²C, address 0x76)
  - PIR motion sensor (HC-SR501 or similar)
  - LDR photoresistor with 10 kΩ voltage divider
- Power: 5V USB (~2A max for all LEDs lit)
- Stability: 100–470 µF capacitor between VCC/GND recommended

---

## Build Environment

- Framework: Arduino
- Platform: espressif8266
- Board: d1\_mini\_pro
- Monitor Speed: 115200
- Key Libraries:
  - tzapu/WiFiManager@^2.0.17 (WiFi credential management)
  - adafruit/Adafruit BME280 Library@^2.3.0 (environmental sensor)
  - adafruit/Adafruit Unified Sensor@^1.1.14 (sensor abstraction)
  - Built-in: ESP8266WiFi, ESP8266WebServer, Wire, TZ.h, ArduinoOTA

---

## Project Structure

```
ESP_LEDMatrix_32x16_NTP_Clock/
├── platformio.ini          # Build config; sets DEBUG_LEVEL=3
├── README.md               # Hardware wiring and setup
├── CLAUDE.md               # This file
├── CHANGELOG.md            # Version history (Keep a Changelog format)
├── src/
│   └── main.cpp            # Main application (~1290 lines, monolithic)
└── include/
    ├── config.h            # All user-tuneable constants (pins, timing, OTA)
    ├── debug.h             # Leveled DBG_ERROR/WARN/INFO/VERBOSE macros
    ├── max7219.h           # MAX7219 SPI driver with rotation support
    ├── fonts.h             # PROGMEM font bitmaps (3×5, 5×8, 5×16, 7×16)
    └── timezones.h         # 88 POSIX timezone definitions
```

---

## Pin Mapping

| Function     | GPIO | Pin Label | Notes                                                  |
|:-------------|:-----|:----------|:-------------------------------------------------------|
| MAX7219 DIN  | 15   | D8        | SPI MOSI                                               |
| MAX7219 CS   | 13   | D7        | Chip Select                                            |
| MAX7219 CLK  | 12   | D6        | SPI Clock                                              |
| BME280 SDA   | 4    | D2        | I²C Data                                               |
| BME280 SCL   | 5    | D1        | I²C Clock                                              |
| PIR Sensor   | 0    | D3        | Digital input (motion detect)                          |
| LDR          | A0   | A0        | Analog 0–1023; voltage divider: 3.3V→10 kΩ→A0→LDR→GND |

**Important:** BME280 configured for I²C address **0x76** (not the common 0x77). Verify your module.

---

## Configuration

### Compile-time Settings (`include/config.h`)

All user-tuneable constants live in `include/config.h`. Edit there — do not add `#define`s to `main.cpp`.

| Setting                           | Default             | Notes                                               |
|:----------------------------------|:--------------------|:----------------------------------------------------|
| `NUM_MAX`                         | 8                   | MAX7219 module count                                |
| `LINE_WIDTH`                      | 32                  | Display width in pixels                             |
| `ROTATE`                          | 90                  | Rotation: 0, 90, or 270                             |
| `DISPLAY_TIMEOUT`                 | 60                  | Seconds before display off (no motion)              |
| `NTP_UPDATE_INTERVAL`             | 600000              | NTP sync interval ms (10 min)                       |
| `MODE_CYCLE_TIME`                 | 20000               | Display mode change interval ms (20 s)              |
| `SENSOR_UPDATE_WITH_NTP`          | true                | Read sensor on each NTP sync                        |
| `LDR_FILTER_WEIGHT`               | 8                   | EMA weight; higher = slower response                |
| `LDR_BRIGHTNESS_HYSTERESIS`       | 35                  | ADC delta before accepting new brightness           |
| `BRIGHTNESS_UPDATE_INTERVAL`      | 500                 | Min ms between auto brightness changes              |
| `DISPLAY_MANUAL_OVERRIDE_DURATION`| 300000              | Manual override timeout ms (5 min)                  |
| `STARTUP_GRACE_PERIOD`            | 10000               | ms to keep display on after boot                    |
| `MY_TZ`                           | `TZ_Australia_Sydney` | Reference only; runtime default is `timezones[0]` |
| `WIFI_AP_NAME`                    | `LED_Clock_Setup`   | Captive portal AP name                              |
| `OTA_HOSTNAME`                    | `led-clock`         | mDNS hostname for OTA                               |
| `OTA_PASSWORD`                    | `ledclock`          | Change before untrusted network deployment          |

### Debug Level (`platformio.ini`)

```ini
build_flags = -DDEBUG_LEVEL=3
```

| Level | Output                          |
|:------|:--------------------------------|
| 0     | Silent                          |
| 1     | `[E]` errors only               |
| 2     | `[E]` + `[W]` warnings          |
| 3     | + `[I]` info (default)          |
| 4     | + `[V]` verbose (per-loop noise)|

### Runtime Configuration (Web Interface)

Accessible at `http://[device-ip]/`:

- Timezone selection (88 global cities)
- Temperature unit (Celsius/Fahrenheit)
- Time format (12-hour/24-hour)
- Display brightness (Auto/Manual with 1–15 slider)
- Display schedule (OFF window: default 22:00–06:00)
- Manual display on/off toggle (5-minute override)
- WiFi reset

### OTA Updates

With the device on the network, upload via PlatformIO:

```bash
pio run --target upload --upload-port led-clock.local
```

Or use Arduino IDE's Sketch → Upload via OTA. Hostname: `led-clock` (set in `config.h`).
The display blanks during the update and restores on reboot.

### WiFi Setup

First boot creates AP: **LED\_Clock\_Setup**. Connect and configure via captive portal.
Reset via web interface at `/reset`.

---

## Current State

**Version 2.9.0 (30th April 2026)** — Production Ready

**Version Information:**

- Version number defined in [src/main.cpp:2-4](src/main.cpp#L2-L4)
- Full changelog maintained in [CHANGELOG.md](CHANGELOG.md)
- Version displayed on serial bootup with build date/time

**Working Features:**

- ✅ NTP time synchronization with automatic DST handling
- ✅ BME280 temperature, humidity, pressure sensing
- ✅ Three rotating display modes (time+temp, large time, time+date)
- ✅ PIR motion detection with 60-second auto-off
- ✅ LDR-based automatic brightness with smoothing and hysteresis
- ✅ Web interface with AJAX updates (no page refresh)
- ✅ Live LED matrix mirror on webpage (500 ms updates)
- ✅ Scheduled display OFF window (configurable start/end times)
- ✅ Manual brightness override with slider
- ✅ 12-hour and 24-hour time format support
- ✅ 88 timezone support with web dropdown selection
- ✅ Celsius/Fahrenheit temperature switching
- ✅ OTA firmware updates (ArduinoOTA, hostname `led-clock`)

**Display Modes (20-second cycle):**

1. Mode 0: Time (H:MM:SS or HH:MM) + Temperature/Humidity
2. Mode 1: Large time display (5×16 font)
3. Mode 2: Time + Date (DD/MMM/YY)

**Web Interface Features:**

- Real-time clock display with Orbitron digital font
- Environmental data (temp, humidity, pressure)
- Visual light level indicator with gradient bar
- Live LED matrix pixel mirror (32×16 canvas)
- Consolidated status and configuration controls
- JSON API endpoints for automation

---

## Architecture Notes

### Intentional Deviations from Global Scaffolding

This is an ESP8266 project; some CYD/ESP32 global conventions apply differently:

- **Custom MAX7219 driver** — `include/max7219.h` is the original Pawel Hernik driver,
  not `MD_MAX72XX`/`MD_Parola`. Both stacks coexist; this project predates that convention.
- **`#define` not `constexpr`** — ESP8266 Arduino core compatibility; acceptable here.
- **Monolithic `main.cpp`** — known issue; refactoring into modules is a documented TODO.

### Event Loop Pattern (Non-blocking)

Main loop uses `millis()` for all timing — no blocking `delay()` except the 100 ms throttle. This allows:

- Periodic NTP sync (10 min intervals)
- Display mode cycling (20 s intervals)
- Motion detection and brightness updates every loop iteration
- Web server + OTA request handling
- Smooth LED animations (dot blinking at 2 Hz)

### Display Power Management State Machine

Priority hierarchy (`handleBrightnessAndMotion()`):

1. **Startup grace period** (10 s): Always on
2. **Manual override timeout**: Expires after 5 min, reverts to automatic
3. **Manual override active**: Respects user on/off toggle, no motion logic
4. **Schedule OFF window**: Forced off (`isWithinScheduleOffWindow()`)
5. **Motion detection**: Auto-on with 60 s countdown
6. **Idle timeout**: Gradual brightness fade to off

### Memory Optimization

- Font bitmaps in PROGMEM (flash storage)
- 88 timezone strings in RAM (~2 KB — PROGMEM migration is a documented TODO)
- Display buffer only 64 bytes (`scr[NUM_MAX*8]`)
- Total SRAM usage kept minimal for ESP8266's 80 KB limit

### Web Interface Architecture

- AJAX-based SPA (no full-page refreshes to prevent flicker)
- `/api/all` — consolidated JSON (time, date, status, sensor data, `light_changed` flag) — 2 s polling
- `/api/display` — JSON `{"pixels":"<512 chars>","width":32,"height":16}` — 500 ms polling; pixels are "0"/"1" chars
- GET endpoints for instant configuration updates
- Canvas rendering with 20× pixel scaling for LED mirror
- Display-off overlay shown over canvas when `displayOn` is false
- `light_changed` flag: when set, client calls `updateAll()` again immediately

### Key Functions (`src/main.cpp`)

| Function                                       | Line | Purpose                                                            |
|:-----------------------------------------------|:-----|:-------------------------------------------------------------------|
| `displayTimeAndTemp()`                         | 321  | Mode 0 — time + sensor data                                        |
| `displayTimeLarge()`                           | 364  | Mode 1 — large 5×16 time (12-hour only)                            |
| `displayTimeAndDate()`                         | 381  | Mode 2 — time + date DD/MMM/YY                                     |
| `updateAmbientLightReading()`                  | 519  | Reads LDR ADC; maintains smoothed value for auto brightness        |
| `computeAmbientBrightnessFromLdr(ldrValue)`    | 532  | Maps LDR ADC (0–1023) to brightness (15–0; darker = dimmer)       |
| `computeStableAmbientBrightnessFromLdr(...)`   | 537  | Applies hysteresis and rate limiting before changing brightness    |
| `applyDisplayHardwareState(on, intensity)`     | 562  | Single point for `CMD_SHUTDOWN` + `CMD_INTENSITY`; skips no-ops   |
| `isWithinScheduleOffWindow()`                  | 585  | Returns true when inside scheduled OFF window; handles midnight wrap|

### Rotation Handling

MAX7219 modules physically arranged in 4×2 grid, rotated 90° in software:

- `refreshAllRot90()` performs bit-shifting transformation
- `/api/display` endpoint mirrors this rotation logic for web canvas
- Allows vertical module stacking to appear as horizontal display

### Brightness Control Logic

LDR mapping: darker ambient light → lower LED brightness.

- `lightLevel` — latest raw ADC value (for web UI display)
- `filteredLightLevel` — EMA used for brightness decisions
- Formula: `map(constrain(ldrValue, 0, 1023), 0, 1023, 15, 0)`
- `computeStableAmbientBrightnessFromLdr()` adds ADC hysteresis and minimum update interval
- Hardware state applied via `applyDisplayHardwareState()`, which skips redundant SPI commands
- 5% raw ADC threshold triggers `lightLevelChanged` → web client refreshes immediately

---

## Known Issues

### Hardware Constraints

- **I²C Address:** BME280 modules vary (0x76 vs 0x77). Current code uses 0x76.
- **Power supply:** USB ports may brownout under ~2A peak LED load.
- **24-hour mode:** Shows HH:MM (no seconds) due to 32 px width. 12-hour fits H:MM:SS.

### Software Limitations

- Settings (time format, timezone, temperature unit) lost on reboot — no persistent config file
- Manual brightness override has fixed 5-minute timeout (not configurable via UI)
- OTA password is plaintext in `config.h` — change `OTA_PASSWORD` before public deployment

### Display Mode Limitations

- Mode 1 (large time) supports 12-hour format only; 24-hour does not fit at 5×16 font size
- Font rendering optimized for English text (no Unicode support)

---

## Development Notes

### Building and Uploading

```bash
pio run                        # Build
pio run --target upload        # Upload via USB
pio run --target upload --upload-port led-clock.local  # Upload via OTA
pio device monitor -b 115200   # Serial monitor
```

### Debugging Tips

- `DEBUG_LEVEL=3` (default) prints `[I]` info events: WiFi, NTP sync, mode changes, schedule
- `DEBUG_LEVEL=4` adds `[V]` verbose: per-loop LDR values, display mode changes
- Status printed every 2 s: time, sensor data, light, brightness, motion, display state, schedule
- Sensor init shows `[I] BME280 OK: 22C, 55% RH, 1013 hPa` or `[E]` error with wiring hint

### Web API Testing

```bash
curl http://[device-ip]/api/all
curl http://[device-ip]/api/display
curl http://[device-ip]/brightness?mode=toggle
curl http://[device-ip]/brightness?value=10
curl http://[device-ip]/timeformat?mode=toggle
curl http://[device-ip]/temperature?mode=toggle
curl "http://[device-ip]/timezone?tz=13"
curl http://[device-ip]/display?mode=toggle
curl "http://[device-ip]/schedule?enabled=1&start_hour=22&start_min=0&end_hour=6&end_min=0"
```

### Adding New Timezones

Edit `include/timezones.h`:

1. Find POSIX TZ string for your location
2. Add `{"City, Country", "TZ_STRING"}` to the `timezones[]` array
3. `numTimezones` auto-updates via `sizeof` calculation
4. Rebuild and upload

### Customizing Display Modes

Edit display functions in `src/main.cpp`:

- `displayTimeAndTemp()` (line 321) — Mode 0
- `displayTimeLarge()` (line 364) — Mode 1 (12-hour only)
- `displayTimeAndDate()` (line 381) — Mode 2

Modify `MODE_CYCLE_TIME` in `include/config.h` to adjust rotation speed.

### Font Customization

Fonts in `include/fonts.h` as PROGMEM arrays:

- `digits7x16[]` — Large 7×16 digits
- `digits5x16rn[]` — Medium 5×16 digits (narrow)
- `digits5x8rn[]` — Small 5×8 digits
- `digits3x5[]` — Tiny 3×5 digits
- `font3x7[]` — 3×7 alphanumeric font

---

## Acknowledgements

- Original code by @cbm80amiga: https://www.youtube.com/watch?v=2wJOdi0xzas&t=32s
- MAX7219 driver functions by Pawel A. Hernik
- Modern rewrite and enhancements by Anthony Clarke (2025–2026)
