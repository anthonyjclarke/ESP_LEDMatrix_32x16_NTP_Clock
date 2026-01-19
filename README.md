
ESP8266 LED Matrix Clock - Modern Edition

**Version:** 2.8.0
**Build System:** PlatformIO
**Full Changelog:** See [CHANGELOG.md](CHANGELOG.md)

Acknowledgements:
Original code by @cbm80amiga here - https://www.youtube.com/watch?v=2wJOdi0xzas&t=32s

Libraries:

	tzapu/WiFiManager

	neptune2/simpleDSTadjust

	adafruit/Adafruit BME280 Library

	adafruit/Adafruit Unified Sensor

=========================== CHANGELOG ==========================

 v2.8 - 16th January 2026
    - Added comprehensive CLAUDE.md project documentation for AI assistant context
    - Fixed version banner to correctly display v2.7 (was showing v1.0)
    - Corrected I2C pin assignment comment (SDA/SCL labels were swapped in comment only)
    - Updated code comments for improved clarity (timezone access, PROGMEM usage)
    - Documented all compile-time configuration options in CLAUDE.md
    - Added TODO notes for future PROGMEM optimization of timezone array (~2KB RAM savings)
    - Code quality improvements: removed misleading comments, improved documentation

 v2.7 - 19th December 2025
    - Added LED Display Mirror to Webserver
    - Refactored project structure: moved fonts.h, max7219.h to include/ directory and created new timezones.h header.
    - Timezone dropdown now auto-updates on selection change without requiring a button press.
    - Added current timezone name to "Current Time & Environment" heading with dynamic AJAX updates.
    - Enhanced /api/all endpoint with timezone_name field for better web interface integration.
    - Removed "Update Timezone" button for cleaner, more intuitive UX.
    - Improved code organization by consolidating 88 timezone definitions into dedicated timezones.h file.
    - Code refactoring and formatting improvements for better maintainability.

 v2.6 - 17th December 2025
    - Added LDR raw reading display to Brightness Control section in web interface.
    - Reordered brightness display to show "LDR Raw Reading: xxxx, calculating Display Brightness to: xx/15".
    - Added real-time JavaScript update for LDR reading value via AJAX.
    - Improved user visibility into automatic brightness calculation from ambient light sensor.

 v2.5 - 16th December 2025
    - Light indicator behavior now matches the LDR (dark -> short bar, reversed gradient, static 🌙/☀️ icons) and the raw reading was removed.
    - Digital time display enlarged to 72px for better readability on any screen.
    - Temperature unit labels now render the true ° symbol, append °C/°F to live data, and expose a short unit code in `/api/status`.

 v2.4 - 15th December 2025
    - Migrated from simpleDSTadjust to TZ.h with full POSIX timezone strings and 88 predefined regions plus custom TZ documentation.
    - Added timezone dropdown with `/timezone` endpoint, automatic NTP re-sync, expanded debug output, and default-enabled schedule handling.

 v2.3 - 15th December 2025
    - Overhauled the Status & Configuration card with emoji light indicator, buttonized controls, and clearer brightness/mode messaging.
    - Reversed the light bar gradient to match sensor behavior and improved overall UI consistency.

 v2.2 - 15th December 2025
    - Reorganized the web UI: light level moved near the time, brightness controls clarified, and digital Orbitron styling applied to time/date.
    - Added large glowing clock/date treatments to improve hierarchy.

 v2.1 - 15th December 2025
    - Swapped in the BME280 for humidity support, updated the web/API outputs, and fixed UTF-8/degree symbol handling.
    - Added humidity/pressure readouts, removed unused sensor files, and improved sensor data streaming to the web UI.

 v2.0 - 14th December 2025
    - Rebuilt the web UI around AJAX updates, added `/api/time` + `/api/status`, and introduced manual brightness + scheduling controls.
    - Cleaned display initialization, added validation, and merged time/environment data into a single card for live updates.

 v1.1 - December 2025
    - Initial GitHub release under PlatformIO with WiFiManager setup; core clock/date functions operational while sensor stack stabilized.

 v1.0 - October 2025
    - Complete modern rewrite featuring WiFiManager onboarding, BMP/BME280 sensing, NTP with DST, PIR-based auto-off, and LDR-driven brightness.

======================== HARDWARE SETUP ========================

ESP8266 (NodeMCU 1.0 or D1 Mini)

2 x 4 Module MAX7219 LED Matrix (32x16):
  DIN  -> D8 (GPIO15)
  CS   -> D7 (GPIO13)
  CLK  -> D6 (GPIO12)
  VCC  -> 5V
  GND  -> GND
  Note: Add 100-470µF capacitor between VCC and GND

BME280 Temperature/Humidity/Pressure Sensor (I2C):
  VCC  -> 3.3V
  GND  -> GND
  SDA  -> D2 (GPIO4)
  SCL  -> D1 (GPIO5)
  ⚠️  NOTE: I2C Address is 0x76 (common for many modules; default is 0x77)

PIR Motion Sensor:
  VCC  -> 5V
  GND  -> GND
  OUT  -> D3 (GPIO0)

LDR (Light Sensor) Circuit:
  3.3V -> 10kΩ resistor -> A0 -> LDR -> GND
  Optional: 100nF capacitor across LDR

======================== FIRST TIME SETUP ========================
1. Upload this sketch to your ESP8266
2. ESP will create WiFi access point: "LED_Clock_Setup"
3. Connect to this AP with your phone/computer
4. Captive portal will open automatically (or go to 192.168.4.1)
5. Select your WiFi network and enter password
6. Click Save - ESP will connect and remember settings
7. To reset WiFi settings, visit http://[device-ip]/reset
==================================================================
