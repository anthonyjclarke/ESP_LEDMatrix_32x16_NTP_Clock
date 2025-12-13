# ESP_LEDMatrix_32x16_NTP_Clock

ESP8266 LED Matrix Clock - Modern Edition
 * Author: Rewritten from original by Anthony Clarke
 * Acknowledgements: Original by @cbm80amiga here - https://www.youtube.com/watch?v=2wJOdi0xzas&t=32s
 * 
 * PlatformIO Version with WiFiManager
 * 
 * =========================== CHANGELOG ==========================
 * v1.0 - October 2025
 *        Complete rewrite with modern practices:
 *        - WiFiManager for easy WiFi setup (no hardcoded credentials)
 *        - SHT30 I2C temperature/humidity sensor
 *        - Automatic NTP time synchronization with DST support
 *        - PIR motion sensor for auto-off/on
 *        - LDR for automatic brightness control
 *        - Clean, well-structured code with proper error handling
 *        - Comprehensive debug output
 *        - Web interface for status
 * ================================================================
 * 
 * ======================== HARDWARE SETUP ========================
 * 
 * ESP8266 (NodeMCU 1.0 or D1 Mini)
 * 
 * MAX7219 LED Matrix (32x16 - 8 modules):
 *   DIN  -> D8 (GPIO15)
 *   CS   -> D7 (GPIO13)
 *   CLK  -> D6 (GPIO12)
 *   VCC  -> 5V
 *   GND  -> GND
 *   Note: Add 100-470µF capacitor between VCC and GND
 * 
 * SHT30 Temperature/Humidity Sensor (I2C):
 *   VCC  -> 3.3V ⚠️ IMPORTANT: Use 3.3V NOT 5V!
 *   GND  -> GND
 *   SDA  -> D2 (GPIO4)
 *   SCL  -> D1 (GPIO5)
 * 
 * PIR Motion Sensor:
 *   VCC  -> 5V
 *   GND  -> GND
 *   OUT  -> D3 (GPIO0)
 * 
 * LDR (Light Sensor) Circuit:
 *   3.3V -> 10kΩ resistor -> A0 -> LDR -> GND
 *   Optional: 100nF capacitor across LDR
 * 
 * ================================================================