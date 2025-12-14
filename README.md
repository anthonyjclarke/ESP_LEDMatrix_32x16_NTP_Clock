
ESP8266 LED Matrix Clock - Modern Edition

Acknowledgements: 
Original code by @cbm80amiga here - https://www.youtube.com/watch?v=2wJOdi0xzas&t=32s

Libraries:
	tzapu/WiFiManager
	neptune2/simpleDSTadjust
    Wemos SHT3x - https://github.com/wemos/WEMOS_SHT3x_Arduino_Library

=========================== CHANGELOG ==========================

v2.0 - 14th December 2025
    - Added AJAX-based web interface with flicker-free updates
    - Implemented manual brightness control with Auto/Manual toggle (1-15 scale)
    - Added display scheduling feature with configurable on/off times
    - Introduced REST API endpoints for time, status, and control
    - Fixed display initialization issues and cleaned up code structure
    - Enhanced web UI with merged status cards and real-time updates

v1.1 - December 2025
    - Initial Github Repo
    - Migrated to PlatformIO
    - Sensor Code NOT Fully tested, Clock / Date and Time working

v1.0 - October 2025
    Complete rewrite with modern practices (assisted by ChatGPT):
    - WiFiManager for easy WiFi setup (no hardcoded credentials)
    - SHT30 I2C temperature/humidity sensor
    - Automatic NTP time synchronization with DST support
    - PIR motion sensor for auto-off/on
    - LDR for automatic brightness control
    - Clean, well-structured code with proper error handling
    - Comprehensive debug output
    - Web interface for status

======================== HARDWARE SETUP ========================

ESP8266 (NodeMCU 1.0 or D1 Mini)

2 x 4 Module MAX7219 LED Matrix (32x16):
  DIN  -> D8 (GPIO15)
  CS   -> D7 (GPIO13)
  CLK  -> D6 (GPIO12)
  VCC  -> 5V
  GND  -> GND
  Note: Add 100-470µF capacitor between VCC and GND

SHT30 Temperature/Humidity Sensor (I2C):
  VCC  -> 3.3V ⚠️ IMPORTANT: Use 3.3V NOT 5V!
  GND  -> GND
  SDA  -> D2 (GPIO4)
  SCL  -> D1 (GPIO5)

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