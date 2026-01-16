# Project: ESP8266 LED Matrix NTP Clock

## Overview
NTP-synchronized clock displaying time, date, temperature, and humidity on a 32×16 LED matrix. Features automatic brightness via LDR, PIR motion detection for power saving, web-based configuration interface with live display mirror, and comprehensive timezone support (88 global cities). Includes scheduled display on/off times and environmental monitoring via BME280 sensor.

## Hardware
- MCU: ESP8266 D1 Mini Pro (WeMos)
- Peripherals:
  - 8× MAX7219 LED matrix modules (4×2 arrangement = 32×16 pixels)
  - BME280 environmental sensor (I²C, address 0x76)
  - PIR motion sensor (HC-SR501 or similar)
  - LDR photoresistor with 10kΩ voltage divider
- Power: 5V USB (LED matrix requires 5V, ensure adequate current capacity ~2A max)
- Power note: 100-470µF capacitor between VCC/GND recommended for LED stability

## Build Environment
- Framework: Arduino
- Platform: espressif8266
- Board: d1_mini_pro
- Monitor Speed: 115200
- Key Libraries:
  - tzapu/WiFiManager@^2.0.17 (WiFi credential management)
  - adafruit/Adafruit BME280 Library@^2.3.0 (environmental sensor)
  - adafruit/Adafruit Unified Sensor@^1.1.14 (sensor abstraction)
  - Built-in: ESP8266WiFi, ESP8266WebServer, Wire, TZ.h

## Project Structure
```
ESP_LEDMatrix_32x16_NTP_Clock/
├── platformio.ini          # PlatformIO build configuration
├── README.md               # Hardware wiring and setup instructions
├── CLAUDE.md              # This file - project context for Claude
├── src/
│   └── main.cpp           # Main application (1547 lines, monolithic)
└── include/
    ├── max7219.h          # MAX7219 SPI driver with rotation support
    ├── fonts.h            # PROGMEM font bitmaps (3×5, 5×8, 5×16, 7×16)
    └── timezones.h        # 88 POSIX timezone definitions
```

## Pin Mapping
| Function | GPIO | Pin Label | Notes |
|----------|------|-----------|-------|
| MAX7219 DIN | 15 | D8 | SPI MOSI |
| MAX7219 CS | 13 | D7 | Chip Select |
| MAX7219 CLK | 12 | D6 | SPI Clock |
| BME280 SDA | 4 | D2 | I²C Data |
| BME280 SCL | 5 | D1 | I²C Clock |
| PIR Sensor | 0 | D3 | Digital input (motion detect) |
| LDR | A0 | A0 | Analog input (0-1023), voltage divider: 3.3V→10kΩ→A0→LDR→GND |

**Important:** BME280 configured for I²C address **0x76** (not the common 0x77 default). Verify your module's address.

## Configuration

### Compile-time Settings (main.cpp lines 241-313)
Key defines that may need customization:

| Setting | Default | Location | Notes |
|---------|---------|----------|-------|
| `NUM_MAX` | 8 | Line 244 | Number of MAX7219 modules |
| `LINE_WIDTH` | 32 | Line 245 | Display width in pixels |
| `ROTATE` | 90 | Line 246 | Display rotation (0, 90, or 270) |
| `DISPLAY_TIMEOUT` | 60 | Line 262 | Seconds before display turns off (no motion) |
| `NTP_UPDATE_INTERVAL` | 600000 | Line 263 | NTP sync interval (10 minutes) |
| `MODE_CYCLE_TIME` | 20000 | Line 264 | Display mode cycle time (20 seconds) |
| `SENSOR_UPDATE_WITH_NTP` | true | Line 265 | Update sensor data during NTP sync |
| `MY_TZ` | `TZ_Australia_Sydney` | Line 281 | Default timezone (see timezones.h for 88 options) |
| `DEBUG_ENABLED` | true | Line 308 | Enable serial debug output |
| `STARTUP_GRACE_PERIOD` | 10000 | Line 386 | Milliseconds to keep display on at boot |
| `DISPLAY_MANUAL_OVERRIDE_DURATION` | 300000 | Line 360 | Manual toggle timeout (5 minutes) |

### Runtime Configuration (via Web Interface)
Accessible at `http://[device-ip]/`:
- Timezone selection (88 global cities)
- Temperature unit (Celsius/Fahrenheit)
- Time format (12-hour/24-hour)
- Display brightness (Auto/Manual with 1-15 slider)
- Display schedule (OFF window: default 22:00-06:00)
- Manual display on/off toggle (5-minute override)
- WiFi reset

### WiFi Setup
First boot creates AP: **LED_Clock_Setup**
- Connect and configure via captive portal
- Credentials stored in ESP8266 flash
- Reset via web interface at `/reset`

## Current State
**Version 2.8 (16th January 2026)** - Production Ready

**Working Features:**
- ✅ NTP time synchronization with automatic DST handling
- ✅ BME280 temperature, humidity, pressure sensing
- ✅ Three rotating display modes (time+temp, large time, time+date)
- ✅ PIR motion detection with 60-second auto-off
- ✅ LDR-based automatic brightness (inverted: darker = dimmer)
- ✅ Web interface with AJAX updates (no page refresh)
- ✅ Live LED matrix mirror on webpage (500ms updates)
- ✅ Scheduled display OFF window (configurable start/end times)
- ✅ Manual brightness override with slider
- ✅ 12-hour and 24-hour time format support
- ✅ 88 timezone support with web dropdown selection
- ✅ Celsius/Fahrenheit temperature switching

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

## Architecture Notes

### Event Loop Pattern (Non-blocking)
Main loop uses `millis()` for all timing - no blocking `delay()` except 100ms throttle. This allows:
- Periodic NTP sync (10 min intervals)
- Display mode cycling (20s intervals)
- Motion detection and brightness updates every loop iteration
- Web server request handling
- Smooth LED animations (dot blinking at 2Hz)

### Display Power Management State Machine
Priority hierarchy (lines 848-935):
1. **Startup grace period** (10 seconds): Always on
2. **Schedule OFF window**: Forced off (overrides everything except startup)
3. **Manual override**: User toggle via web (5-minute timeout)
4. **Motion detection**: Auto-on with 60s countdown
5. **Idle timeout**: Gradual fade to off

### Memory Optimization
- Font bitmaps in PROGMEM (flash storage): keeps ~500 bytes in SRAM
- 88 timezone strings in RAM (could be optimized to PROGMEM): ~2KB
- Display buffer only 72 bytes (`scr[NUM_MAX*8 + 8]`)
- Total SRAM usage kept minimal for ESP8266's 80KB limit

### Web Interface Architecture
- AJAX-based SPA (no full-page refreshes to prevent flicker)
- `/api/all` - consolidated JSON (time, date, status, sensor data) - 2s polling
- `/api/display` - raw pixel buffer (512 chars: "0"/"1") - 500ms polling
- GET endpoints for instant configuration updates
- Canvas rendering with 20× pixel scaling for LED mirror

### Rotation Handling
MAX7219 modules physically arranged in 4×2 grid, rotated 90° in software:
- `refreshAllRot90()` performs bit-shifting transformation
- `/api/display` endpoint mirrors this rotation logic for web canvas
- Allows vertical module stacking to appear as horizontal display

### Timezone Implementation
- Migrated from deprecated `simpleDSTadjust` to native ESP8266 `TZ.h` (v2.4)
- POSIX TZ strings handle automatic DST transitions
- 88 predefined timezones covering all major global cities
- Custom TZ string support for edge cases

### Brightness Control Logic
LDR mapping is **inverted** to match real-world behavior:
- Low LDR reading (dark room) = high resistance = low brightness
- High LDR reading (bright room) = low resistance = high brightness
- Formula: `brightness = 15 - map(constrain(ldrValue, 0, 1023), 0, 1023, 1, 15)` (line 810)
- The `constrain()` ensures out-of-range ADC values don't cause issues

## Known Issues

### Hardware Constraints
- **I²C Address:** BME280 modules vary (0x76 vs 0x77). Current code uses 0x76. Check your module with I²C scanner if sensor not detected.
- **Power supply:** Ensure 5V supply can handle ~2A peak when all LEDs lit. USB ports may brownout.
- **24-hour mode limitation:** Due to 32px width constraint, 24-hour format shows HH:MM (no seconds). 12-hour mode fits H:MM:SS because hour isn't zero-padded.

### Software Limitations
- Time format toggle doesn't persist across reboots (stored in volatile RAM)
- Timezone selection doesn't persist across reboots (defaults to `MY_TZ` define)
- No SPIFFS/LittleFS config file - all settings volatile except WiFi credentials
- Manual brightness override has fixed 5-minute timeout (not configurable via UI)

### Display Mode Limitations
- Mode 1 (large time) only supports 12-hour format (insufficient space for 24-hour layout)
- Font rendering is optimized for English text (no Unicode support)

## TODO

### High Priority (from code TODOs)
- [ ] LDR sensitivity control via web interface (on/off toggle, threshold adjustment)
- [ ] OpenWeatherMap API integration for weather forecast display
- [ ] OTA firmware update capability (ESP8266HTTPUpdate library)

### Feature Enhancements
- [ ] Persist runtime config (timezone, time format, temp unit) to SPIFFS/LittleFS
- [ ] Add configuration backup/restore via JSON export/import
- [ ] Multiple schedule windows (e.g., off during work hours + nighttime)
- [ ] Configurable display mode preferences (disable unwanted modes)
- [ ] Scrolling text message support for custom announcements
- [ ] MQTT support for Home Assistant integration
- [ ] Add indoor air quality sensor (CCS811 or BME680)

### Web Interface Improvements
- [ ] Consolidate multi-section web interface into single-page tabbed layout
- [ ] Add time format and timezone persistence controls
- [ ] Display uptime and WiFi signal strength
- [ ] Add manual NTP sync trigger button
- [ ] Show next scheduled display on/off event countdown
- [ ] Add graph of temperature/humidity history (requires data logging)

### Code Quality
- [ ] Move timezone array to PROGMEM to save ~2KB RAM (requires refactoring struct access with pgm_read_ptr)
- [ ] Refactor monolithic main.cpp into modular files (display, web, sensors, time)
- [ ] Add unit tests for critical functions (schedule logic, rotation math)
- [ ] Implement proper logging levels (INFO, WARN, ERROR) instead of DEBUG macro
- [ ] Add watchdog timer for crash recovery

### Hardware Expansion Ideas
- [ ] Support for larger matrix displays (64×32, 64×64)
- [ ] RGB LED matrix support (HUB75 interface)
- [ ] Add buzzer for hourly chimes or alarm functionality
- [ ] External RTC (DS3231) for timekeeping during WiFi outages

## Development Notes

### Building and Uploading
```bash
# Build project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor -b 115200
```

### Debugging Tips
- Serial debug output enabled by default (`DEBUG_ENABLED true`)
- Status printed every 2 seconds: time, sensor data, light level, brightness, motion, display state, schedule
- NTP sync events logged with timezone name
- WiFi connection details printed during setup
- Sensor initialization shows success/failure with readings

### Web API Testing
```bash
# Get all status data
curl http://[device-ip]/api/all

# Get display pixel buffer
curl http://[device-ip]/api/display

# Toggle brightness mode
curl http://[device-ip]/brightness?mode=toggle

# Set manual brightness (1-15)
curl http://[device-ip]/brightness?value=10

# Toggle time format
curl http://[device-ip]/timeformat?mode=toggle

# Toggle temperature unit
curl http://[device-ip]/temperature?mode=toggle

# Change timezone (0-87 index)
curl http://[device-ip]/timezone?tz=13

# Toggle display on/off
curl http://[device-ip]/display?mode=toggle

# Update schedule (24-hour format)
curl "http://[device-ip]/schedule?enabled=1&start_hour=22&start_min=0&end_hour=6&end_min=0"
```

### Adding New Timezones
Edit `include/timezones.h`:
1. Find POSIX TZ string for your location (search "POSIX timezone [city]")
2. Add to `timezones[]` array: `{"City, Country", "TZ_STRING"}`
3. `numTimezones` auto-updates via `sizeof` calculation
4. Rebuild and upload

### Customizing Display Modes
Edit display functions (lines 593-691):
- `displayTimeAndTemp()` - Mode 0 (time + sensor data)
- `displayTimeLarge()` - Mode 1 (large time only)
- `displayTimeAndDate()` - Mode 2 (time + date)

Modify `MODE_CYCLE_TIME` (line 264) to adjust rotation speed.

### Font Customization
Fonts stored in `include/fonts.h` as PROGMEM arrays:
- `digits7x16[]` - Large 7×16 digits
- `digits5x16rn[]` - Medium 5×16 digits (narrow)
- `digits5x8rn[]` - Small 5×8 digits
- `digits3x5[]` - Tiny 3×5 digits
- `font3x7[]` - 3×7 alphanumeric font

Use online LED font generators or bitmap editors to create custom fonts. Format must match existing structure (width, height, char range, bitmap data).

## Acknowledgements
- Original code by @cbm80amiga: https://www.youtube.com/watch?v=2wJOdi0xzas&t=32s
- MAX7219 driver functions by Pawel A. Hernik
- Modern rewrite and enhancements by Anthony Clarke (2025)
