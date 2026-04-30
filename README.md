# ESP8266 LED Matrix Clock

**Version:** 2.9.0 | **Platform:** ESP8266 D1 Mini Pro | **Build:** PlatformIO

NTP-synchronized clock on a 32×16 LED matrix with automatic brightness, motion detection,
environmental monitoring, OTA updates, and a web configuration interface.

---

## Features

- **NTP Time Synchronization** — Automatic DST handling, 88 global timezone support
- **Environmental Monitoring** — BME280 sensor (temperature, humidity, pressure)
- **Smart Display Control** — PIR motion detection with auto-off and scheduled operation
- **Automatic Brightness** — LDR-based ambient light adjustment with smoothing and hysteresis
- **OTA Firmware Updates** — Upload new firmware over Wi-Fi (hostname `led-clock`)
- **Web Interface** — AJAX configuration with live LED matrix mirror
- **Multiple Display Modes** — Three rotating layouts (time+temp, large time, time+date)
- **Flexible Configuration** — 12/24-hour time, °C/°F, manual overrides

---

## Hardware

### Components

- ESP8266 D1 Mini Pro (WeMos)
- 8× MAX7219 LED matrix modules (4×2 = 32×16 pixels)
- BME280 environmental sensor (I²C)
- PIR motion sensor (HC-SR501 or similar)
- LDR photoresistor with 10 kΩ resistor
- 100–470 µF capacitor (LED power stability)

### Pin Connections

| Component    | ESP8266 | GPIO   | Notes                                      |
|:-------------|:--------|:-------|:-------------------------------------------|
| MAX7219 DIN  | D8      | GPIO15 | SPI MOSI                                   |
| MAX7219 CS   | D7      | GPIO13 | Chip Select                                |
| MAX7219 CLK  | D6      | GPIO12 | SPI Clock                                  |
| BME280 SDA   | D2      | GPIO4  | I²C Data                                   |
| BME280 SCL   | D1      | GPIO5  | I²C Clock                                  |
| PIR Sensor   | D3      | GPIO0  | Motion detect                              |
| LDR          | A0      | A0     | 3.3V → 10 kΩ → A0 → LDR → GND             |

**Power:** 5V USB, ~2A capacity for full LED brightness.

**Important:** BME280 I²C address is **0x76** (not the default 0x77). Run an I²C scanner if the sensor is not detected.

---

## Installation

```bash
git clone https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock.git
cd ESP_LEDMatrix_32x16_NTP_Clock
pio run --target upload
pio device monitor -b 115200
```

### First-Time WiFi Setup

1. Upload firmware
2. ESP creates AP: **LED\_Clock\_Setup**
3. Connect and configure via captive portal (or 192.168.4.1)
4. Device connects and stores credentials

To reset WiFi: `http://[device-ip]/reset`

---

## Configuration

### Web Interface

`http://[device-ip]/`

- Timezone (88 global cities)
- Temperature unit (°C / °F)
- Time format (12-hour / 24-hour)
- Display brightness (Auto / Manual 1–15)
- Display schedule (OFF window with start/end times)
- Manual display on/off (5-minute override)
- WiFi reset

### Compile-Time Settings (`include/config.h`)

All tuneable constants live in `include/config.h`. Key settings:

```cpp
#define MY_TZ        TZ_Australia_Sydney  // Default timezone (runtime selectable via web)
#define DISPLAY_TIMEOUT  60               // Seconds before auto-off (no motion)
#define MODE_CYCLE_TIME  20000            // Display mode rotation ms
#define OTA_HOSTNAME     "led-clock"      // mDNS hostname for OTA
#define OTA_PASSWORD     "ledclock"       // Change before untrusted network deployment
```

See [CLAUDE.md](CLAUDE.md) for the full configuration reference.

### OTA Updates

With the device on the network:

```bash
pio run --target upload --upload-port led-clock.local
```

Or use Arduino IDE: Sketch → Upload via OTA. The display blanks during the update.

---

## Web API

```bash
curl http://[device-ip]/api/all                         # All status data
curl http://[device-ip]/api/display                     # Pixel buffer (32×16)
curl http://[device-ip]/brightness?mode=toggle          # Auto ↔ Manual
curl http://[device-ip]/brightness?value=10             # Set manual brightness
curl http://[device-ip]/timeformat?mode=toggle          # 12h ↔ 24h
curl http://[device-ip]/temperature?mode=toggle         # °C ↔ °F
curl "http://[device-ip]/timezone?tz=13"                # Select timezone by index
curl http://[device-ip]/display?mode=toggle             # Display on/off
curl "http://[device-ip]/schedule?enabled=1&start_hour=22&start_min=0&end_hour=6&end_min=0"
```

---

## Display Modes

Three rotating layouts (20-second cycle):

1. **Mode 0:** Time + Temperature/Humidity
   - 12h: H:MM:SS with temp/humidity on second line
   - 24h: HH:MM (seconds omitted; 32 px width constraint)

2. **Mode 1:** Large Time (5×16 font, 12-hour only)

3. **Mode 2:** Time + Date (DD/MMM/YY on second line)

---

## Power Management

Priority hierarchy:

1. **Startup grace** (10 s): Always on
2. **Manual override**: User toggle via web (5-min timeout)
3. **Schedule OFF window**: Forced off during configured times
4. **Motion detection**: Auto-on with 60 s countdown
5. **Idle timeout**: Gradual fade to off

Brightness adjusts automatically via LDR.

---

## Development

```bash
pio run                        # Compile
pio run --target upload        # Upload (USB)
pio run --target upload --upload-port led-clock.local  # Upload (OTA)
pio run --target clean         # Clean
```

### Debug Output

Controlled by `-DDEBUG_LEVEL=N` in `platformio.ini` (default 3):

| Level | Output                     |
|:------|:---------------------------|
| 0     | Silent                     |
| 1     | `[E]` errors only          |
| 2     | + `[W]` warnings           |
| 3     | + `[I]` info (default)     |
| 4     | + `[V]` verbose            |

### Project Structure

```
ESP_LEDMatrix_32x16_NTP_Clock/
├── platformio.ini          # Build config; DEBUG_LEVEL=3
├── README.md               # This file
├── CHANGELOG.md            # Version history
├── CLAUDE.md               # Technical reference for AI/developers
├── src/
│   └── main.cpp            # Main application
└── include/
    ├── config.h            # User-tuneable constants (pins, timing, OTA)
    ├── debug.h             # Leveled DBG_* macros
    ├── max7219.h           # LED driver with rotation support
    ├── fonts.h             # PROGMEM font bitmaps
    └── timezones.h         # 88 POSIX timezone definitions
```

---

## Documentation

- [CHANGELOG.md](CHANGELOG.md) — Version history
- [CLAUDE.md](CLAUDE.md) — Architecture, configuration reference, API docs
