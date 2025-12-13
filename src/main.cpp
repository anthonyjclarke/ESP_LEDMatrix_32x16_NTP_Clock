#include "Arduino.h"

/*
 * ESP8266 LED Matrix Clock - Modern Edition
 * Author: Rewritten from original by Anthony Clarke
 * Acknowledgements: Original by @cbm80amiga here - https://www.youtube.com/watch?v=2wJOdi0xzas&t=32s
 * 
 * PlatformIO Version with WiFiManager
 * 
 * ======================== CHANGELOG ========================
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
 * ===========================================================
 * 
 * ======================== PLATFORMIO SETUP ========================
 * 1. Create a new PlatformIO project
 * 2. Copy this file to src/main.cpp
 * 3. Use the platformio.ini configuration below
 * 4. Build and upload
 * 
 * platformio.ini contents:
 * -----------------------
 * [env:d1_mini]
 * platform = espressif8266
 * board = d1_mini
 * framework = arduino
 * monitor_speed = 115200
 * lib_deps = 
 *     tzapu/WiFiManager@^2.0.16-rc.2
 *     wemos/WEMOS SHT3x Arduino Library@^1.0.0
 *     neptune2/simpleDSTadjust@^1.0.1
 * -----------------------
 * ==================================================================
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
 * 
 * ======================== FIRST TIME SETUP ========================
 * 1. Upload this sketch to your ESP8266
 * 2. ESP will create WiFi access point: "LED_Clock_Setup"
 * 3. Connect to this AP with your phone/computer
 * 4. Captive portal will open automatically (or go to 192.168.4.1)
 * 5. Select your WiFi network and enter password
 * 6. Click Save - ESP will connect and remember settings
 * 7. To reset WiFi settings, visit http://[device-ip]/reset
 * ==================================================================
 */

/*
 * CHANGELOG SUMMARY
 * - 2025-10-XX: Initial rewrite and features (see header above for full changelog).
 * - 2025-12-13: Removed unused variable 'fwd' from font helper to fix compiler warnings.
 * - 2025-12-13: Removed unused variable 'fht' from `charWidth()` to fix compiler warning.
 * - 2025-12-13: Added concise changelog comments section at top of code.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include <simpleDSTadjust.h>
#include <Wire.h>
#include <WEMOS_SHT3X.h>

// ======================== CONFIGURATION ========================

// Display Configuration
#define NUM_MAX 8                 // Number of MAX7219 modules
#define LINE_WIDTH 32             // Display width in pixels
#define ROTATE 90                 // Display rotation (0, 90, or 270)

// Pin Definitions - MAX7219
#define DIN_PIN 15                // D8
#define CS_PIN  13                // D7
#define CLK_PIN 12                // D6

// Pin Definitions - Sensors
#define PIR_PIN D3                // Motion sensor
#define LDR_PIN A0                // Light sensor (analog)

// Timing Configuration
#define DISPLAY_TIMEOUT 60        // Seconds before display turns off (no motion)
#define NTP_UPDATE_INTERVAL 600000 // NTP sync interval (10 minutes)
#define MODE_CYCLE_TIME 20000     // Display mode change interval (20 seconds)
#define SENSOR_UPDATE_WITH_NTP true // Update temp/humidity with NTP sync

// NTP Server Configuration
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov", "time.google.com"

// Timezone Configuration - Select your region below
// Australia Eastern Time (Sydney, Melbourne)
#define TIMEZONE_OFFSET 10        // UTC offset in hours
#define DST_START_RULE {"AEDT", First, Sun, Oct, 2, 3600}  // UTC+11
#define DST_END_RULE {"AEST", First, Sun, Apr, 2, 0}       // UTC+10

// Alternate timezones (comment out above and uncomment one below if needed):
// US Eastern Time (New York)
// #define TIMEZONE_OFFSET -5
// #define DST_START_RULE {"EDT", Second, Sun, Mar, 2, 3600}  // UTC-4
// #define DST_END_RULE {"EST", First, Sun, Nov, 2, 0}        // UTC-5

// US Pacific Time (Los Angeles)
// #define TIMEZONE_OFFSET -8
// #define DST_START_RULE {"PDT", Second, Sun, Mar, 2, 3600}  // UTC-7
// #define DST_END_RULE {"PST", First, Sun, Nov, 2, 0}        // UTC-8

// UK (London)
// #define TIMEZONE_OFFSET 0
// #define DST_START_RULE {"BST", Last, Sun, Mar, 1, 3600}    // UTC+1
// #define DST_END_RULE {"GMT", Last, Sun, Oct, 1, 0}         // UTC+0

// Debug Output
#define DEBUG_ENABLED true        // Set to false to disable serial output
#if DEBUG_ENABLED
  #define DEBUG(x) x
#else
  #define DEBUG(x)
#endif

// ======================== OBJECTS & GLOBALS ========================

// DST Rules
struct dstRule startRule = DST_START_RULE;
struct dstRule endRule = DST_END_RULE;
simpleDSTadjust dstAdjusted(startRule, endRule);

// Sensors
SHT3X sht30(0x45);

// Web Server
ESP8266WebServer server(80);

// WiFi Manager
WiFiManager wifiManager;

// Time Variables
int hours, minutes, seconds;
int day, month, year, dayOfWeek;
bool showDots = true;

// Display Variables
int xPos = 0, yPos = 0;
int currentMode = 0;
char txt[32];

// Sensor Data
int temperature = 0;
int humidity = 0;
bool sensorAvailable = false;

// Display Control
int brightness = 8;
int lightLevel = 512;
int displayTimer = DISPLAY_TIMEOUT;
bool displayOn = true;
bool motionDetected = false;

// Timing
unsigned long lastNTPUpdate = 0;
unsigned long lastModeChange = 0;

// ======================== MAX7219.H ========================
// MAX7219 Commands
#define CMD_NOOP   0
#define CMD_DIGIT0 1
#define CMD_DIGIT1 2
#define CMD_DIGIT2 3
#define CMD_DIGIT3 4
#define CMD_DIGIT4 5
#define CMD_DIGIT5 6
#define CMD_DIGIT6 7
#define CMD_DIGIT7 8
#define CMD_DECODEMODE  9
#define CMD_INTENSITY   10
#define CMD_SCANLIMIT   11
#define CMD_SHUTDOWN    12
#define CMD_DISPLAYTEST 15

byte scr[NUM_MAX * 8 + 8];

void sendCmd(int addr, byte cmd, byte data) {
  digitalWrite(CS_PIN, LOW);
  for (int i = NUM_MAX - 1; i >= 0; i--) {
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, i == addr ? cmd : 0);
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, i == addr ? data : 0);
  }
  digitalWrite(CS_PIN, HIGH);
}

void sendCmdAll(byte cmd, byte data) {
  digitalWrite(CS_PIN, LOW);
  for (int i = NUM_MAX - 1; i >= 0; i--) {
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, cmd);
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, data);
  }
  digitalWrite(CS_PIN, HIGH);
}

void refresh(int addr) {
  for (int i = 0; i < 8; i++)
    sendCmd(addr, i + CMD_DIGIT0, scr[addr * 8 + i]);
}

void refreshAllRot270() {
  byte mask = 0x01;
  for (int c = 0; c < 8; c++) {
    digitalWrite(CS_PIN, LOW);
    for (int i = NUM_MAX - 1; i >= 0; i--) {
      byte bt = 0;
      for (int b = 0; b < 8; b++) {
        bt <<= 1;
        if (scr[i * 8 + b] & mask) bt |= 0x01;
      }
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, CMD_DIGIT0 + c);
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, bt);
    }
    digitalWrite(CS_PIN, HIGH);
    mask <<= 1;
  }
}

void refreshAllRot90() {
  byte mask = 0x80;
  for (int c = 0; c < 8; c++) {
    digitalWrite(CS_PIN, LOW);
    for (int i = NUM_MAX - 1; i >= 0; i--) {
      byte bt = 0;
      for (int b = 0; b < 8; b++) {
        bt >>= 1;
        if (scr[i * 8 + b] & mask) bt |= 0x80;
      }
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, CMD_DIGIT0 + c);
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, bt);
    }
    digitalWrite(CS_PIN, HIGH);
    mask >>= 1;
  }
}

void refreshAll() {
#if ROTATE == 270
  refreshAllRot270();
#elif ROTATE == 90
  refreshAllRot90();
#else
  for (int c = 0; c < 8; c++) {
    digitalWrite(CS_PIN, LOW);
    for (int i = NUM_MAX - 1; i >= 0; i--) {
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, CMD_DIGIT0 + c);
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, scr[i * 8 + c]);
    }
    digitalWrite(CS_PIN, HIGH);
  }
#endif
}

void clr() {
  for (int i = 0; i < NUM_MAX * 8; i++) scr[i] = 0;
}

void initMAX7219() {
  pinMode(DIN_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  sendCmdAll(CMD_DISPLAYTEST, 0);
  sendCmdAll(CMD_SCANLIMIT, 7);
  sendCmdAll(CMD_DECODEMODE, 0);
  sendCmdAll(CMD_INTENSITY, 0);
  sendCmdAll(CMD_SHUTDOWN, 0);
  clr();
  refreshAll();
}

// ======================== FONTS.H ========================

const uint8_t digits5x16rn[] PROGMEM = {5, 16, '0', ':',
  0x05, 0xFE, 0x7F, 0x01, 0x80, 0x01, 0x80, 0xFF, 0xFF, 0xFE, 0x7F,
  0x04, 0x04, 0x00, 0x02, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
  0x05, 0x02, 0xFF, 0x81, 0x80, 0x81, 0x80, 0xFF, 0x80, 0x7E, 0x80,
  0x05, 0x02, 0x40, 0x81, 0x80, 0x81, 0x80, 0xFF, 0xFF, 0x7E, 0x7F,
  0x05, 0xFF, 0x01, 0x00, 0x01, 0x00, 0x01, 0xFE, 0xFF, 0xFE, 0xFF,
  0x05, 0xFF, 0x40, 0x81, 0x80, 0x81, 0x80, 0x81, 0xFF, 0x01, 0x7F,
  0x05, 0xFE, 0x7F, 0x81, 0x80, 0x81, 0x80, 0x81, 0xFF, 0x02, 0x7F,
  0x05, 0x01, 0x00, 0x01, 0xFC, 0xC1, 0xFF, 0xFF, 0x03, 0x3F, 0x00,
  0x05, 0x7E, 0x7F, 0x81, 0x80, 0x81, 0x80, 0xFF, 0xFF, 0x7E, 0x7F,
  0x05, 0x7E, 0x40, 0x81, 0x80, 0x81, 0x80, 0xFF, 0xFF, 0xFE, 0x7F,
  0x01, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t digits5x8rn[] PROGMEM = {5, 8, ' ', ':',
  0, 0, 0, 0, 0, 0,
  1, B01011111, B00000000, B00000000, 0, 0,
  3, B00000011, B00000000, B00000011, 0, 0,
  3, B00000010, B01111111, B00000010, 0, 0,
  3, B00100000, B01111111, B00100000, 0, 0,
  3, B01100001, B00011100, B01000011, 0, 0,
  0, 0, 0, 0, 0, 0,
  1, B00000001, B00000000, B00000000, 0, 0,
  2, B00111110, B01000001, B00000000, 0, 0,
  2, B01000001, B00111110, B00000000, 0, 0,
  0, 0, 0, 0, 0, 0,
  5, B00001000, B00001000, B00111110, B00001000, B00001000,
  2, B10000000, B01000000, B00000000, 0, 0,
  5, B00001000, B00001000, B00001000, B00001000, B00001000,
  1, B01000000, B00000000, B00000000, 0, 0,
  3, B01100000, B00011100, B00000011, 0, 0,
  0x05, 0x7E, 0x81, 0x81, 0xFF, 0x7E,
  0x04, 0x04, 0x02, 0xFF, 0xFF, 0x00,
  0x05, 0xF1, 0x89, 0x89, 0x8F, 0x86,
  0x05, 0x81, 0x89, 0x89, 0xFF, 0x76,
  0x05, 0x1F, 0x10, 0x10, 0xFE, 0xFE,
  0x05, 0x8F, 0x89, 0x89, 0xF9, 0x71,
  0x05, 0x7E, 0x89, 0x89, 0xF9, 0x70,
  0x05, 0x01, 0xC1, 0xF1, 0x3F, 0x0F,
  0x05, 0x76, 0x89, 0x89, 0xFF, 0x76,
  0x05, 0x0E, 0x91, 0x91, 0xFF, 0x7E,
  1, B00100100, B00000000, B00000000, B00000000, B00000000,
};

const uint8_t font3x7[] PROGMEM = {5, 7, ' ', '_',
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x40, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x7F, 0x41, 0x7F, 0x00, 0x00,
  0x03, 0x04, 0x02, 0x7F, 0x00, 0x00,
  0x03, 0x79, 0x49, 0x4F, 0x00, 0x00,
  0x03, 0x41, 0x49, 0x7F, 0x00, 0x00,
  0x03, 0x1F, 0x10, 0x7C, 0x00, 0x00,
  0x03, 0x4F, 0x49, 0x79, 0x00, 0x00,
  0x03, 0x7F, 0x49, 0x79, 0x00, 0x00,
  0x03, 0x01, 0x71, 0x0F, 0x00, 0x00,
  0x03, 0x7F, 0x49, 0x7F, 0x00, 0x00,
  0x03, 0x4F, 0x49, 0x7F, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x78, 0x24, 0x78, 0x00, 0x00,
  0x03, 0x7C, 0x54, 0x28, 0x00, 0x00,
  0x03, 0x38, 0x44, 0x44, 0x00, 0x00,
  0x03, 0x7C, 0x44, 0x38, 0x00, 0x00,
  0x03, 0x7C, 0x54, 0x44, 0x00, 0x00,
  0x03, 0x7C, 0x14, 0x04, 0x00, 0x00,
  0x03, 0x38, 0x44, 0x74, 0x00, 0x00,
  0x03, 0x7C, 0x10, 0x7C, 0x00, 0x00,
  0x01, 0x7C, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x44, 0x44, 0x3C, 0x00, 0x00,
  0x03, 0x7C, 0x10, 0x6C, 0x00, 0x00,
  0x03, 0x7C, 0x40, 0x40, 0x00, 0x00,
  0x05, 0x78, 0x04, 0x38, 0x04, 0x78,
  0x03, 0x7C, 0x04, 0x78, 0x00, 0x00,
  0x03, 0x38, 0x44, 0x38, 0x00, 0x00,
  0x03, 0x7C, 0x24, 0x18, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x7C, 0x24, 0x58, 0x00, 0x00,
  0x03, 0x48, 0x54, 0x24, 0x00, 0x00,
  0x03, 0x04, 0x7C, 0x04, 0x00, 0x00,
  0x03, 0x3C, 0x40, 0x7C, 0x00, 0x00,
  0x03, 0x3C, 0x40, 0x3C, 0x00, 0x00,
  0x05, 0x3C, 0x40, 0x38, 0x40, 0x3C,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x1C, 0x70, 0x1C, 0x00, 0x00,
  0x03, 0x64, 0x54, 0x4C, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t digits3x5[] PROGMEM = {3, 5, '0', '9',
  0x03, 0xF8, 0x88, 0xF8,
  0x03, 0, 0x10, 0xF8,
  0x03, 0xE8, 0xA8, 0xB8,
  0x03, 0x88, 0xA8, 0xF8,
  0x03, 0x38, 0x20, 0xF8,
  0x03, 0xB8, 0xA8, 0xE8,
  0x03, 0xF8, 0xA8, 0xE8,
  0x03, 0x08, 0x08, 0xF8,
  0x03, 0xF8, 0xA8, 0xF8,
  0x03, 0xB8, 0xA8, 0xF8,
};

// ======================== FONT HELPER FUNCTIONS ========================

int charWidth(char c, const uint8_t* font) {
  int offs = pgm_read_byte(font + 2);
  int last = pgm_read_byte(font + 3);
  if (c < offs || c > last) return 0;
  c -= offs;
  int len = pgm_read_byte(font + 4);
  return pgm_read_byte(font + 5 + c * len);
}

int printCharX(char ch, const uint8_t* font, int x) {
  int fwd = pgm_read_byte(font);
  int fht = pgm_read_byte(font + 1);
  int offs = pgm_read_byte(font + 2);
  int last = pgm_read_byte(font + 3);
  if (ch < offs || ch > last) return 0;
  ch -= offs;
  int fht8 = (fht + 7) / 8;
  font += 4 + ch * (fht8 * fwd + 1);
  int j, i, w = pgm_read_byte(font);
  for (j = 0; j < fht8; j++) {
    for (i = 0; i < w; i++) 
      scr[x + LINE_WIDTH * (j + yPos) + i] = pgm_read_byte(font + 1 + fht8 * i + j);
    if (x + i < LINE_WIDTH) 
      scr[x + LINE_WIDTH * (j + yPos) + i] = 0;
  }
  return w;
}

void printChar(unsigned char c, const uint8_t* font) {
  if (xPos > NUM_MAX * 8) return;
  int w = printCharX(c, font, xPos);
  xPos += w + 1;
}

void printString(const char* s, const uint8_t* font) {
  while (*s) printChar(*s++, font);
}

// ======================== FORWARD DECLARATIONS ========================

void printBanner();
void showMessage(const char* message);
void testSensor();
void updateSensorData();
bool syncNTP();
void updateTime();
void handleBrightnessAndMotion();
void setupWebServer();
void configModeCallback(WiFiManager* myWiFiManager);
void printStatus();
void displayTimeAndTemp();
void displayTimeLarge();
void displayTimeAndDate();

// ======================== SETUP ========================

void setup() 
{
  Serial.begin(115200);
  delay(100);
  
  printBanner();
  
  // Initialize display
  DEBUG(Serial.println("Initializing LED matrix..."));
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 5);
  
  // Initialize I2C for SHT30
  DEBUG(Serial.println("Initializing I2C and SHT30 sensor..."));
  Wire.begin();
  delay(100);
  testSensor();
  
  // Initialize sensors
  pinMode(PIR_PIN, INPUT);
  DEBUG(Serial.println("PIR sensor initialized"));
  
  // Display WiFi setup message
  showMessage("WIFI...");
  
  // WiFiManager Setup
  DEBUG(Serial.println("\nStarting WiFi Manager..."));
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setAPCallback(configModeCallback);
  
  // Try to connect, if fails start config portal
  if (!wifiManager.autoConnect("LED_Clock_Setup")) {
    DEBUG(Serial.println("Failed to connect, restarting..."));
    showMessage("WIFI FAIL");
    delay(3000);
    ESP.restart();
  }
  
  // Connected!
  DEBUG(Serial.println("\nWiFi connected!"));
  DEBUG(Serial.print("IP: "); Serial.println(WiFi.localIP()));
  
  showMessage(WiFi.localIP().toString().c_str());
  delay(2000);
  
  // Initialize NTP
  showMessage("SYNC TIME");
  if (syncNTP()) {
    DEBUG(Serial.println("Time synchronized!"));
  } else {
    DEBUG(Serial.println("Time sync failed, will retry..."));
  }
  
  // Read initial sensor data
  updateSensorData();
  
  // Start web server
  setupWebServer();
  server.begin();
  DEBUG(Serial.println("Web server started"));
  
  showMessage("READY!");
  delay(1000);
  
  DEBUG(Serial.println("\n=== Setup Complete ===\n"));
}

// ======================== MAIN LOOP ========================

void loop()
{
  unsigned long currentMillis = millis();
  
  // Handle web server requests
  server.handleClient();
  
  // Periodic NTP sync
  if (currentMillis - lastNTPUpdate >= NTP_UPDATE_INTERVAL) {
    lastNTPUpdate = currentMillis;
    DEBUG(Serial.println("\n--- Periodic Update ---"));
    syncNTP();
    if (SENSOR_UPDATE_WITH_NTP) {
      updateSensorData();
    }
  }
  
  // Update current time
  updateTime();
  
  // Blink dots (2 Hz)
  showDots = (currentMillis % 1000) < 500;
  
  // Cycle display modes
  int newMode = (currentMillis % (MODE_CYCLE_TIME * 3)) / MODE_CYCLE_TIME;
  if (newMode != currentMode) {
    currentMode = newMode;
    DEBUG(Serial.print("Display mode: "); Serial.println(currentMode));
  }
  
  // Update display based on current mode
  switch (currentMode) {
    case 0: displayTimeAndTemp(); break;
    case 1: displayTimeLarge(); break;
    case 2: displayTimeAndDate(); break;
  }
  refreshAll();
  
  // Handle brightness and motion detection
  handleBrightnessAndMotion();
  
  // Debug output (throttled)
  static unsigned long lastDebug = 0;
  if (DEBUG_ENABLED && currentMillis - lastDebug > 2000) {
    lastDebug = currentMillis;
    printStatus();
  }
  
  delay(100);
}

// ======================== DISPLAY FUNCTIONS ========================

void displayTimeAndTemp() {
  clr();
  
  // Top line: Time
  yPos = 0;
  xPos = (hours > 9) ? 0 : 2;
  sprintf(txt, "%d", hours);
  printString(txt, digits5x8rn);
  if (showDots) printCharX(':', digits5x8rn, xPos);
  xPos += (hours >= 20) ? 1 : 2;
  sprintf(txt, "%02d", minutes);
  printString(txt, digits5x8rn);
  sprintf(txt, "%02d", seconds);
  printString(txt, digits3x5);
  
  // Bottom line: Temperature and Humidity
  yPos = 1;
  xPos = 1;
  if (sensorAvailable) {
    sprintf(txt, "T%d H%d", temperature, humidity);
  } else {
    sprintf(txt, "NO SENSOR");
  }
  printString(txt, font3x7);
  
  // Shift bottom line slightly
  for (int i = 0; i < LINE_WIDTH; i++) scr[LINE_WIDTH + i] <<= 1;
}

void displayTimeLarge() {
  clr();
  yPos = 0;
  xPos = (hours > 9) ? 0 : 3;
  sprintf(txt, "%d", hours);
  printString(txt, digits5x16rn);
  if (showDots) printCharX(':', digits5x16rn, xPos);
  xPos += (hours >= 20) ? 1 : 2;
  sprintf(txt, "%02d", minutes);
  printString(txt, digits5x16rn);
  sprintf(txt, "%02d", seconds);
  printString(txt, font3x7);
}

void displayTimeAndDate() {
  clr();
  
  // Top line: Time
  yPos = 0;
  xPos = (hours > 9) ? 0 : 2;
  sprintf(txt, "%d", hours);
  printString(txt, digits5x8rn);
  if (showDots) printCharX(':', digits5x8rn, xPos);
  xPos += (hours >= 20) ? 1 : 2;
  sprintf(txt, "%02d", minutes);
  printString(txt, digits5x8rn);
  sprintf(txt, "%02d", seconds);
  printString(txt, digits3x5);
  
  // Bottom line: Date
  yPos = 1;
  xPos = 1;
  const char* months[] = {"JAN","FEB","MAR","APR","MAY","JUN",
                          "JUL","AUG","SEP","OCT","NOV","DEC"};
  sprintf(txt, "%d&%s&%d", day, months[month - 1], year % 100);
  printString(txt, font3x7);
  
  // Shift bottom line slightly
  for (int i = 0; i < LINE_WIDTH; i++) scr[LINE_WIDTH + i] <<= 1;
}

void showMessage(const char* message) {
  clr();
  xPos = 0;
  yPos = 0;
  printString(message, font3x7);
  refreshAll();
}

// ======================== TIME FUNCTIONS ========================

bool syncNTP() {
  DEBUG(Serial.println("Syncing with NTP servers..."));
  
  configTime(TIMEZONE_OFFSET * 3600, 0, NTP_SERVERS);
  
  // Wait for time sync (max 10 seconds)
  int attempts = 0;
  while (!time(nullptr) && attempts < 20) {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  Serial.println();
  
  if (attempts >= 20) {
    DEBUG(Serial.println("NTP sync failed!"));
    return false;
  }
  
  updateTime();
  DEBUG(Serial.printf("Time synced: %02d:%02d:%02d %02d/%02d/%d\n",
                      hours, minutes, seconds, day, month, year));
  return true;
}

void updateTime() {
  char* dstAbbrev;
  time_t t = dstAdjusted.time(&dstAbbrev);
  struct tm* timeinfo = localtime(&t);
  
  // Convert to 12-hour format
  int h24 = timeinfo->tm_hour;
  hours = (h24 == 0) ? 12 : (h24 > 12) ? h24 - 12 : h24;
  minutes = timeinfo->tm_min;
  seconds = timeinfo->tm_sec;
  day = timeinfo->tm_mday;
  month = timeinfo->tm_mon + 1;
  year = timeinfo->tm_year + 1900;
  dayOfWeek = timeinfo->tm_wday;
}

// ======================== SENSOR FUNCTIONS ========================

void testSensor() {
  DEBUG(Serial.print("Testing SHT30 sensor... "));
  
  // Try to read from sensor - get() returns void, so we check the values
  sht30.get();
  
  // Check if readings are valid (NaN check or reasonable range)
  if (isnan(sht30.cTemp) || isnan(sht30.humidity) || 
      sht30.cTemp < -40 || sht30.cTemp > 125) {
    sensorAvailable = false;
    DEBUG(Serial.println("NOT FOUND!"));
    DEBUG(Serial.println("  Check wiring: SDA->D2, SCL->D1, VCC->3.3V"));
  } else {
    sensorAvailable = true;
    temperature = (int)sht30.cTemp;
    humidity = (int)sht30.humidity;
    DEBUG(Serial.println("OK!"));
    DEBUG(Serial.printf("  Temperature: %d°C\n", temperature));
    DEBUG(Serial.printf("  Humidity: %d%%\n", humidity));
  }
}

void updateSensorData() {
  // Read sensor - get() returns void
  sht30.get();
  
  // Check if readings are valid
  if (isnan(sht30.cTemp) || isnan(sht30.humidity) ||
      sht30.cTemp < -40 || sht30.cTemp > 125) {
    sensorAvailable = false;
    DEBUG(Serial.println("Sensor read failed!"));
  } else {
    temperature = (int)sht30.cTemp;
    humidity = (int)sht30.humidity;
    sensorAvailable = true;
    DEBUG(Serial.printf("Sensor: %d°C, %d%%\n", temperature, humidity));
  }
}

// ======================== BRIGHTNESS & MOTION ========================

void handleBrightnessAndMotion() {
  // Read ambient light
  lightLevel = analogRead(LDR_PIN);
  
  // Map to brightness (inverted: darker = dimmer)
  int ambientBrightness = 15 - map(constrain(lightLevel, 0, 1023), 0, 1023, 1, 15);
  
  // Check motion
  motionDetected = digitalRead(PIR_PIN);
  
  if (motionDetected) {
    // Motion detected - turn on and reset timer
    displayTimer = DISPLAY_TIMEOUT;
    displayOn = true;
    brightness = ambientBrightness;
    sendCmdAll(CMD_SHUTDOWN, 1);
    sendCmdAll(CMD_INTENSITY, brightness);
  } else {
    // No motion - countdown
    if (displayTimer > 0) {
      displayTimer--;
      // Fade out gradually
      brightness = map(displayTimer, 0, DISPLAY_TIMEOUT, 1, ambientBrightness);
      sendCmdAll(CMD_INTENSITY, brightness);
    } else {
      // Timer expired - turn off
      if (displayOn) {
        displayOn = false;
        sendCmdAll(CMD_SHUTDOWN, 0);
      }
      displayTimer = 0;
    }
  }
}

// ======================== WEB SERVER ========================

void setupWebServer() {
  // Root page
  server.on("/", []() {
    String html = "<html><head><title>LED Clock</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0;}";
    html += ".card{background:white;padding:20px;margin:10px;border-radius:10px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
    html += "h1{color:#333;}</style></head><body>";
    html += "<h1>LED Matrix Clock</h1>";
    html += "<div class='card'><h2>Current Time</h2>";
    html += "<p style='font-size:24px;'>" + String(hours) + ":" + 
            (minutes < 10 ? "0" : "") + String(minutes) + ":" +
            (seconds < 10 ? "0" : "") + String(seconds) + "</p>";
    html += "<p>" + String(day) + "/" + String(month) + "/" + String(year) + "</p></div>";
    
    if (sensorAvailable) {
      html += "<div class='card'><h2>Environment</h2>";
      html += "<p>Temperature: " + String(temperature) + "°C</p>";
      html += "<p>Humidity: " + String(humidity) + "%</p></div>";
    }
    
    html += "<div class='card'><h2>Status</h2>";
    html += "<p>Display: " + String(displayOn ? "ON" : "OFF") + "</p>";
    html += "<p>Motion: " + String(motionDetected ? "Detected" : "None") + "</p>";
    html += "<p>Brightness: " + String(brightness) + "/15</p>";
    html += "<p>Light Level: " + String(lightLevel) + "</p></div>";
    
    html += "<div class='card'><p><a href='/reset'>Reset WiFi Settings</a></p></div>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
  });
  
  // Reset WiFi
  server.on("/reset", []() {
    server.send(200, "text/html", 
      "<html><body><h1>WiFi Reset</h1><p>WiFi settings cleared. Device will restart...</p></body></html>");
    delay(1000);
    wifiManager.resetSettings();
    ESP.restart();
  });
}

// ======================== HELPER FUNCTIONS ========================

void configModeCallback(WiFiManager* myWiFiManager) {
  DEBUG(Serial.println("\n=== WiFi Config Mode ==="));
  DEBUG(Serial.println("Connect to AP: LED_Clock_Setup"));
  DEBUG(Serial.print("Config portal IP: "));
  DEBUG(Serial.println(WiFi.softAPIP()));
  
  showMessage("SETUP AP");
  delay(2000);
  showMessage("LED CLOCK");
}

void printBanner() {
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║   ESP8266 LED Matrix Clock v1.0        ║");
  Serial.println("║   PlatformIO Edition                   ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
}

void printStatus() {
  Serial.println("--- Status ---");
  Serial.printf("Time: %02d:%02d:%02d | Date: %02d/%02d/%d\n",
                hours, minutes, seconds, day, month, year);
  if (sensorAvailable) {
    Serial.printf("Temp: %d°C | Humid: %d%% | ", temperature, humidity);
  }
  Serial.printf("Light: %d | Bright: %d\n", lightLevel, brightness);
  Serial.printf("Motion: %s | Display: %s | Timer: %d\n",
                motionDetected ? "YES" : "NO",
                displayOn ? "ON" : "OFF",
                displayTimer);
  Serial.println();
}

// ======================== END OF CODE ========================