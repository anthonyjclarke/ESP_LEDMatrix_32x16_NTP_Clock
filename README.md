# ESP8266 LED Matrix Clock

**Version:** 2.8.0
**Build System:** PlatformIO
**Platform:** ESP8266 (D1 Mini Pro)

NTP-synchronized clock with 32×16 LED matrix display, featuring automatic brightness, motion detection, environmental monitoring, and comprehensive web interface.

## Features

- **NTP Time Synchronization** - Automatic DST handling with 88 global timezone support
- **Environmental Monitoring** - BME280 sensor for temperature, humidity, and pressure
- **Smart Display Control** - PIR motion detection with auto-off and scheduled operation
- **Automatic Brightness** - LDR-based ambient light adjustment
- **Web Interface** - AJAX-based configuration with live LED matrix mirror
- **Multiple Display Modes** - Three rotating layouts (time+temp, large time, time+date)
- **Flexible Configuration** - 12/24-hour time, °C/°F units, manual overrides

## Hardware Requirements

### Components
- ESP8266 D1 Mini Pro (WeMos)
- 8× MAX7219 LED matrix modules (4×2 arrangement = 32×16 pixels)
- BME280 environmental sensor (I²C)
- PIR motion sensor (HC-SR501 or similar)
- LDR photoresistor with 10kΩ resistor
- 100-470µF capacitor (for LED power stability)

### Pin Connections

| Component | ESP8266 Pin | GPIO | Notes |
|-----------|-------------|------|-------|
| MAX7219 DIN | D8 | GPIO15 | SPI MOSI |
| MAX7219 CS | D7 | GPIO13 | Chip Select |
| MAX7219 CLK | D6 | GPIO12 | SPI Clock |
| BME280 SDA | D2 | GPIO4 | I²C Data |
| BME280 SCL | D1 | GPIO5 | I²C Clock |
| PIR Sensor | D3 | GPIO0 | Motion detect |
| LDR | A0 | A0 | Analog input |

**Power:**
- MAX7219: 5V (ensure 2A supply capacity)
- BME280: 3.3V
- PIR: 5V
- LDR circuit: 3.3V → 10kΩ → A0 → LDR → GND

**Important:** BME280 I²C address is configured for **0x76** (not the default 0x77). Verify your module's address if sensor not detected.

## Installation

### PlatformIO

```bash
# Clone the repository
git clone https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock.git
cd ESP_LEDMatrix_32x16_NTP_Clock

# Build and upload
pio run --target upload

# Monitor serial output
pio device monitor -b 115200
```

### First-Time Setup

1. Upload firmware to ESP8266
2. ESP creates WiFi AP: **LED_Clock_Setup**
3. Connect to AP with phone/computer
4. Captive portal opens (or navigate to 192.168.4.1)
5. Select your WiFi network and enter credentials
6. Click Save - device connects and remembers settings

To reset WiFi: Visit `http://[device-ip]/reset`

## Configuration

### Web Interface

Access the web interface at `http://[device-ip]/`

**Available Settings:**
- Timezone selection (88 global cities)
- Temperature unit (Celsius/Fahrenheit)
- Time format (12-hour/24-hour)
- Display brightness (Auto/Manual with 1-15 slider)
- Display schedule (OFF window with start/end times)
- Manual display on/off toggle
- WiFi reset

### Compile-Time Settings

Edit `src/main.cpp` to customize default behavior:

```cpp
#define MY_TZ TZ_Australia_Sydney     // Default timezone
#define DISPLAY_TIMEOUT 60             // Seconds before auto-off
#define MODE_CYCLE_TIME 20000          // Display mode rotation (ms)
#define DEBUG_ENABLED true             // Serial debug output
```

See [CLAUDE.md](CLAUDE.md) for complete configuration reference.

## Web API

RESTful API endpoints for automation:

```bash
# Get all status data
curl http://[device-ip]/api/all

# Get display pixel buffer
curl http://[device-ip]/api/display

# Toggle brightness mode (Auto/Manual)
curl http://[device-ip]/brightness?mode=toggle

# Set manual brightness (1-15)
curl http://[device-ip]/brightness?value=10

# Toggle time format (12/24 hour)
curl http://[device-ip]/timeformat?mode=toggle

# Toggle temperature unit (C/F)
curl http://[device-ip]/temperature?mode=toggle

# Change timezone (0-87 index)
curl http://[device-ip]/timezone?tz=13

# Toggle display on/off
curl http://[device-ip]/display?mode=toggle

# Update schedule
curl "http://[device-ip]/schedule?enabled=1&start_hour=22&start_min=0&end_hour=6&end_min=0"
```

## Display Modes

Three rotating display modes (20-second cycle):

1. **Mode 0:** Time + Temperature/Humidity
   - 12h: H:MM:SS with temp/humidity on second line
   - 24h: HH:MM (no seconds due to space constraints)

2. **Mode 1:** Large Time Display
   - 5×16 font for maximum visibility
   - 12-hour format only

3. **Mode 2:** Time + Date
   - Time on top line
   - Date (DD/MMM/YY) on bottom line

## Power Management

Smart display control with priority hierarchy:

1. **Startup grace period** (10 seconds): Always on
2. **Schedule OFF window**: Forced off during configured times
3. **Manual override**: User toggle via web (5-minute timeout)
4. **Motion detection**: Auto-on with 60-second countdown
5. **Idle timeout**: Gradual fade to off

Brightness automatically adjusts based on ambient light (LDR sensor).

## Development

### Building

```bash
# Compile
pio run

# Upload
pio run --target upload

# Clean build
pio run --target clean
```

### Serial Debug

Enable debug output in `src/main.cpp`:

```cpp
#define DEBUG_ENABLED true
```

Serial monitor shows:
- Boot banner with version and build date
- WiFi connection status
- NTP sync events
- Sensor readings
- Display state changes
- 2-second status updates

### Project Structure

```
ESP_LEDMatrix_32x16_NTP_Clock/
├── platformio.ini          # Build configuration
├── README.md               # This file
├── CHANGELOG.md           # Version history
├── CLAUDE.md              # AI assistant documentation
├── src/
│   └── main.cpp           # Main application code
└── include/
    ├── max7219.h          # LED driver with rotation support
    ├── fonts.h            # PROGMEM font bitmaps
    └── timezones.h        # 88 POSIX timezone definitions
```

## Documentation

- **[CHANGELOG.md](CHANGELOG.md)** - Complete version history
- **[CLAUDE.md](CLAUDE.md)** - Detailed technical documentation
  - Architecture notes
  - Configuration reference
  - Pin mappings
  - API documentation
  - Development guide

## Troubleshooting

**Sensor not detected:**
- Verify BME280 I²C address (0x76 vs 0x77)
- Check wiring: SDA→D2, SCL→D1, VCC→3.3V
- Run I²C scanner to confirm address

**Display issues:**
- Ensure 5V supply can handle ~2A
- Add 100-470µF capacitor to VCC/GND
- Check MAX7219 wiring and module count

**WiFi connection fails:**
- Reset WiFi via web interface `/reset`
- Check AP name "LED_Clock_Setup" appears
- Try factory reset (re-upload firmware)

**Time not syncing:**
- Verify WiFi connection
- Check serial output for NTP errors
- Confirm timezone setting is correct

## Credits

**Original Code:** [@cbm80amiga](https://www.youtube.com/watch?v=2wJOdi0xzas&t=32s)
**MAX7219 Driver:** Pawel A. Hernik
**Modern Rewrite:** Anthony Clarke (2025-2026)

## License

See original project for license information.

## Links

- **GitHub:** [ESP_LEDMatrix_32x16_NTP_Clock](https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock)
- **Author:** [Anthony Clarke](https://bsky.app/profile/anthonyjclarke.bsky.social)

---

**Note:** This is an ESP8266 project (not ESP32). Uses D1 Mini Pro with 16MB flash.
