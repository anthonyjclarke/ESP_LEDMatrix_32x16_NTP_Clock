#include "Arduino.h"

/*
 * ESP8266 LED Matrix Clock - Modern Edition
 * Author: Rewritten from original by Anthony Clarke
 * Acknowledgements: Original by @cbm80amiga here - https://www.youtube.com/watch?v=2wJOdi0xzas&t=32s
 * 
 * =========================== TODO ==========================
 * - Switch LDR sensitivity via web interface and On/Off
 * - get weather from online API and display on matrix and webpage using https://openweathermap.org/
 * - Implement web interface for full configuration
 * - Add OTA firmware update capability
 * - tidy up web interface and combine all configuration into single page
 * 
 * ======================== CHANGELOG ========================
 *
 * v2.8 - 16th January 2026
 *   - Added comprehensive CLAUDE.md project documentation file
 *   - Fixed version banner to show v2.7 (was incorrectly showing v1.0)
 *   - Corrected I2C pin assignment comment (SDA/SCL labels were swapped)
 *   - Updated code comments for clarity (timezone access, PROGMEM notes)
 *   - Refactored header files: moved fonts.h, max7219.h, timezones.h to include/
 *   - Added TODO for PROGMEM optimization of timezone array (~2KB RAM savings)
 *
 * v2.7 - 19th December 2025
 *   - Added LED Display Mirror to Webserver (Bit of Fun!)
 *   - Timezone dropdown now auto-updates on selection change (no button needed)
 *   - Added current timezone name to "Current Time & Environment" heading
 *   - Timezone name dynamically updates via AJAX when changed
 *   - Removed "Update Timezone" button for cleaner UX
 *   - Enhanced /api/all endpoint with timezone_name field
 *
 * v2.6 - 17th December 2025
 *   - Added LDR raw reading display to Brightness Control section in web interface
 *   - Reordered brightness display to show "LDR Raw Reading: xxxx, calculating Display Brightness to: xx/15"
 *   - Added real-time JavaScript update for LDR reading value via AJAX
 *   - Improved user visibility into automatic brightness calculation from ambient light sensor
 * 
 * v2.5 - 16th December 2025
 *   - Refined light level UI so the bar shrinks as ambient light readings rise
 *     and swapped the gradient to run from dark (moon) to bright (sun).
 *   - Fixed light indicator layout by anchoring static 🌙/☀️ icons and
 *     removing the distracting raw LDR value readout.
 *   - Increased the digital time font size by 50% (48px -> 72px) for better
 *     readability on desktops and mobile devices.
 *   - Corrected the temperature unit label to render the real degree symbol and
 *     added dynamic °C/°F suffixes to the sensor data line.
 *   - Extended /api/status with a compact `temp_unit_short` field so the web UI
 *     can show the currently selected unit next to live readings.
 * 
 * v2.4 - 15th December 2025
 *   - Replaced obsolete simpleDSTadjust library with modern TZ.h timezone support
 *   - Implemented POSIX TZ strings for automatic DST handling
 *   - Added support for predefined timezones (TZ_Australia_Sydney, TZ_America_New_York, etc.)
 *   - Simplified time synchronization using built-in ESP8266 timezone functions
 *   - Removed manual DST rule configuration requirements
 *   - Enhanced timezone configuration with detailed POSIX TZ string documentation
 *   - Updated configTime() to use modern TZ string parameter
 *   - Improved code maintainability by removing deprecated library dependency
 *   - Added web-based timezone selection with dropdown menu
 *   - Expanded timezone options to 88 global timezones covering all major regions
 *   - Added /timezone endpoint for dynamic timezone changes
 *   - Enabled automatic NTP re-sync when timezone is changed
 *   - Added current timezone display in NTP sync debug output
 *   - Changed display schedule default to ENABLED (was disabled)
 *   - Fixed PSTR() compilation error by using string literals instead of TZ_* macros
 * 
 * v2.3 - 15th December 2025
 *   - Added visual light level indicator with gradient bar chart and dynamic icons
 *   - Implemented emoji icons for light conditions: ☀️ (bright), ☁️ (medium), 🌙 (dark)
 *   - Reversed light bar gradient to match LDR behavior (low=bright, high=dark)
 *   - Merged Status and Configuration sections into unified "Status & Configuration" card
 *   - Converted Display ON/OFF control to button format matching other controls
 *   - Added Display Brightness mode indicator (Manual/Automatic) to Status section
 *   - Moved brightness mode toggle from Configuration to Status section
 *   - Changed Temperature Unit control to button format for consistency
 *   - Reorganized layout with clear Status and Configuration subsections
 *   - Enhanced visual consistency across all control elements
 * 
 * v2.2 - 15th December 2025
 *   - Web UI reorganization: moved Light Level from Status to Current Time & Environment section
 *   - Moved Current Brightness to Configuration > Brightness Control section
 *   - Renamed Current Brightness to Display Brightness for clarity
 *   - Enhanced time/date display with digital-style Orbitron font
 *   - Added large 48px glowing green time display with LED clock aesthetic
 *   - Added 28px blue date display with matching digital font styling
 *   - Improved visual hierarchy and information grouping in web interface
 * 
 * v2.1 - 15th December 2025
 *   - Replaced BMP280 sensor with BME280 for humidity support
 *   - Added humidity display on LED matrix and web interface (T°C H%%)
 *   - Added pressure reading to web interface display
 *   - Fixed temperature/humidity/pressure display on webpage via API endpoint
 *   - Changed BME280 I2C address from 0x77 to 0x76 (common default for many modules) so BE AWARE
    - Fixed character encoding issue on webpage (UTF-8 charset)
 *   - Removed unused SHT3X sensor files from project
 *   - Enhanced /api/status endpoint with sensor data (temperature, humidity, pressure)
 *   - Updated JavaScript to dynamically display sensor readings
 * 
 * v2.0 - 2025-12-14
 *   - Web UI: replaced full-page refresh with AJAX updates for time and selective
 *     Status updates to eliminate flicker.
 *   - Web API: added `/api/time` and `/api/status` JSON endpoints.
 *   - Brightness: added manual brightness override with Auto/Manual toggle and
 *     a 1–15 slider in the web interface; `/brightness` endpoint to set/toggle.
 *   - Scheduling: added display schedule (enable, start/end times) with
 *     `/schedule` form + endpoint; display respects schedule and shows a
 *     schedule notification in the web Status card when off.
 *   - Display code: fixed `NUM_MAX` header-scope issue by reordering defines
 *     before `max7219.h` include; cleaned unused variables (`fwd`, `fht`).
 *   - UI: merged Current Time and Environment into a single card; added
 *     on-demand Status refresh when brightness changes.
 *   - Misc: improved debug output and added validation for schedule inputs.
 * 
 * v1.1 - December 2025
 *   - Initial Github Repo
 *   - Migrated to PlatformIO
 *   - Sensor Code NOT Fully tested, Clock / Date and Time working
 *   - Move fonts to separate header file
 *   - Moved MAX7219 functions to separate header file
 *   - PlatformIO Version with WiFiManager

 * v1.0 - October 2025
 *        Complete rewrite with modern practices:
 *        - WiFiManager for easy WiFi setup (no hardcoded credentials)
 *        - BMP/BME280 I2C temperature/pressure/humidity sensor
 *        - Automatic NTP time synchronization with DST support
 *        - PIR motion sensor for auto-off/on
 *        - LDR for automatic brightness control
 *        - Clean, well-structured code with proper error handling
 *        - Comprehensive debug output
 *        - Web interface for status and configuration Status


 * DETAILED CHANGELOG SUMMARY (Co-Pilot)
 * - 2025-12-19: Added auto-update to timezone dropdown selection (onChange event handler).
 * - 2025-12-19: Removed "Update Timezone" button for cleaner UX and instant timezone changes.
 * - 2025-12-19: Added timezone_name field to /api/all JSON endpoint.
 * - 2025-12-19: Updated "Current Time & Environment" heading to include timezone name dynamically.
 * - 2025-12-19: Added JavaScript to update timezone name in heading via updateAll() function.
 * - 2025-10-XX: Initial rewrite and features (see header above for full changelog).
 * - 2025-12-13: Removed unused variable 'fwd' from font helper to fix compiler warnings.
 * - 2025-12-13: Removed unused variable 'fht' from `charWidth()` to fix compiler warning.
 * - 2025-12-13: Added concise changelog comments section at top of code.
 * - 2025-12-14: Moved NUM_MAX, LINE_WIDTH, ROTATE, and pin definitions before max7219.h include.
 * - 2025-12-14: Fixed NUM_MAX scope error in max7219.h by reordering includes/defines.
 * - 2025-12-14: Removed full-page refresh, replaced with JavaScript AJAX updates to eliminate flickering.
 * - 2025-12-14: Added /api/time endpoint for JSON time data (1-second refresh).
 * - 2025-12-14: Added /api/status endpoint for JSON status data (brightness, motion, display state).
 * - 2025-12-14: Added manual brightness override with Auto/Manual toggle in web interface.
 * - 2025-12-14: Added brightness slider (1-15) for manual brightness control.
 * - 2025-12-14: Status section now updates on-demand when brightness is changed.
 * - 2025-12-14: Added HTTP 302 redirect after brightness mode toggle to refresh page.
 * - 2025-12-14: Added display schedule configuration (start/end times, enable/disable flag).
 * - 2025-12-14: Added isWithinScheduleWindow() function for schedule validation.
 * - 2025-12-14: Updated handleBrightnessAndMotion() to respect display schedule times.
 * - 2025-12-14: Added schedule notification in web status section (shows when display is off due to schedule).
 * - 2025-12-14: Enhanced /api/status endpoint with schedule_disabled, outside_schedule, schedule_start/end fields.
 * - 2025-12-14: Added Display Schedule web form with enable/disable, start/end time inputs.
 * - 2025-12-14: Added /schedule endpoint (POST) to handle display schedule configuration updates.
 * - 2025-12-14: Merged Current Time and Environment sections on web page into single card.
 * - 2025-12-15: Replaced Adafruit_BMP280 with Adafruit_BME280 library for humidity support.
 * - 2025-12-15: Updated sensor object from bmp280 to bme280 with humidity reading capability.
 * - 2025-12-15: Changed BME280 I2C address from 0x77 to 0x76 (common default for many modules).
 * - 2025-12-15: Updated testSensor() to read and validate humidity from BME280.
 * - 2025-12-15: Updated updateSensorData() to read humidity and validate against 0-100% range.
 * - 2025-12-15: Changed LED matrix display from "T°C P[hPa]" to "T°C H[%]" format.
 * - 2025-12-15: Replaced pressure with humidity in web interface sensor display.
 * - 2025-12-15: Updated serial debug output to show humidity instead of pressure.
 * - 2025-12-15: Added UTF-8 charset meta tag to HTML head for proper character encoding.
 * - 2025-12-15: Replaced degree symbol with HTML entity (&deg;) in all webpage outputs.
 * - 2025-12-15: Enhanced /api/status endpoint with temperature, humidity, pressure, and sensor_available fields.
 * - 2025-12-15: Updated updateStatus() JavaScript function to display sensor data from API.
 * - 2025-12-15: Added sensor-data element to HTML for dynamic sensor reading updates.
 * - 2025-12-15: Removed unused WEMOS_SHT3X.cpp and WEMOS_SHT3X.h files from project.
 * - 2025-12-15: Reorganized web interface layout - moved Light Level from Status to Current Time & Environment section.
 * - 2025-12-15: Moved Current Brightness from Status section to Configuration > Brightness Control section.
 * - 2025-12-15: Renamed Current Brightness to Display Brightness for improved clarity.
 * - 2025-12-15: Added Google Fonts Orbitron font family for digital-style time/date display.
 * - 2025-12-15: Enhanced time display with 48px font size, green color, and glowing text shadow effect.
 * - 2025-12-15: Enhanced date display with 28px font size and blue color using Orbitron font.
 * - 2025-12-15: Added CSS classes .digital-time and .digital-date for consistent digital clock styling.
 * - 2025-12-15: Added visual light level bar chart with gradient fill (0-100% based on LDR reading).
 * - 2025-12-15: Implemented dynamic emoji icons for light conditions (sun, cloud, moon).
 * - 2025-12-15: Created CSS classes .light-container, .light-icon, .light-bar-bg, .light-bar-fill for light visualization.
 * - 2025-12-15: Reversed light bar gradient from light-to-dark to match LDR behavior (low=bright, high=dark).
 * - 2025-12-15: Updated icon logic so low LDR values show sun (bright), high values show moon (dark).
 * - 2025-12-15: Modified JavaScript updateStatus() to dynamically update light bar width and icon.
 * - 2025-12-15: Merged Status and Configuration sections into single "Status & Configuration" card.
 * - 2025-12-15: Added Status and Configuration subsection headers (h3) for clear organization.
 * - 2025-12-15: Converted Display ON/OFF link to button format with "Turn ON"/"Turn OFF" labels.
 * - 2025-12-15: Added Display Brightness mode (Manual/Automatic) indicator to Status section.
 * - 2025-12-15: Moved brightness mode toggle button from Configuration to Status section.
 * - 2025-12-15: Changed Brightness Control from h3 to h4 level heading under Configuration.
 * - 2025-12-15: Converted Temperature Unit control from link to button format.
 * - 2025-12-15: Changed Temperature Unit from h3 to h4 level heading under Configuration.
 * - 2025-12-15: Updated temperature toggle button text to "Switch to Celsius"/"Switch to Fahrenheit".
 * - 2025-12-15: Changed Display Schedule from h3 to h4 level heading under Configuration.
 * - 2025-12-15: Removed redundant brightness-mode span element from Configuration section.
 * - 2025-12-15: Updated JavaScript to use brightness-mode-status for Status section updates.
 * - 2025-12-15: Replaced obsolete simpleDSTadjust library with modern TZ.h timezone support.
 * - 2025-12-15: Removed #include <simpleDSTadjust.h> and added #include <TZ.h> and #include <coredecls.h>.
 * - 2025-12-15: Replaced manual TIMEZONE_OFFSET and DST_START_RULE/DST_END_RULE with POSIX TZ strings.
 * - 2025-12-15: Implemented predefined timezone macros (TZ_Australia_Sydney, TZ_America_New_York, etc.).
 * - 2025-12-15: Added comprehensive POSIX TZ string format documentation in timezone configuration.
 * - 2025-12-15: Removed struct dstRule and simpleDSTadjust object declarations from globals.
 * - 2025-12-15: Updated syncNTP() to use configTime(MY_TZ, NTP_SERVERS) instead of manual offset calculation.
 * - 2025-12-15: Simplified updateTime() function to use time(nullptr) and localtime() directly.
 * - 2025-12-15: Removed dstAdjusted.time() call and replaced with standard time functions.
 * - 2025-12-15: Added support for additional timezones (Europe, Asia) with predefined TZ constants.
 * - 2025-12-15: Documented custom TZ string format for users with non-standard timezone requirements.
 * - 2025-12-15: Added currentTimezone global variable to track selected timezone index.
 * - 2025-12-15: Created TimezoneOption struct to store timezone name and TZ string pairs.
 * - 2025-12-15: Implemented timezones[] array with 20 predefined timezone options.
 * - 2025-12-15: Added numTimezones constant for array size calculation.
 * - 2025-12-15: Updated syncNTP() to use timezones[currentTimezone].tzString for configTime().
 * - 2025-12-15: Enhanced NTP sync debug output to display selected timezone name.
 * - 2025-12-15: Added Timezone configuration section to web interface under Configuration.
 * - 2025-12-15: Implemented timezone dropdown select menu with all available options.
 * - 2025-12-15: Created /timezone POST endpoint to handle timezone changes.
 * - 2025-12-15: Added automatic NTP re-sync when timezone is changed via web interface.
 * - 2025-12-15: Implemented timezone validation (0 to numTimezones-1) in endpoint handler.
 * - 2025-12-15: Expanded timezone coverage from 20 to 88 global timezones.
 * - 2025-12-15: Fixed PSTR() compilation error by using POSIX TZ string literals instead of TZ_* macros.
 * - 2025-12-15: Changed schedule default to enabled (scheduleOffEnabled=true).
 * - 2025-12-15: Updated timezone array to use PROGMEM for flash storage to save RAM.
 * - 2025-12-15: Reworked light level UI so the bar width shrinks as readings increase and
 *               swapped the gradient direction to run dark-to-bright.
 * - 2025-12-16: Locked moon/sun icons to left/right of the light bar and removed the raw LDR value label.
 * - 2025-12-16: Increased .digital-time font size from 48px to 72px for improved readability.
 * - 2025-12-16: Switched temperature unit label rendering to use the real ° symbol instead of &deg; placeholders.
 * - 2025-12-16: Added temp_unit_short to /api/status so the sensor readout can append °C/°F dynamically.
 * - 2025-12-17: Added LDR raw reading display to Brightness Control section in web interface.
 * - 2025-12-17: Reordered brightness display to show "LDR Raw Reading: xxxx, calculating Display Brightness to: xx/15".
 * - 2025-12-17: Added ldr-status span element for dynamic LDR value updates via JavaScript.
 * - 2025-12-17: Updated updateAll() JavaScript function to refresh LDR reading from API response.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include <TZ.h>
#include <coredecls.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

// ======================== CONFIGURATION ========================

// Display Configuration (must be before max7219.h include)
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

#include "max7219.h"
#include "fonts.h"
#include "timezones.h"

// Timing Configuration
#define DISPLAY_TIMEOUT 60        // Seconds before display turns off (no motion)
#define NTP_UPDATE_INTERVAL 600000 // NTP sync interval (10 minutes)
#define MODE_CYCLE_TIME 20000     // Display mode change interval (20 seconds)
#define SENSOR_UPDATE_WITH_NTP true // Update temp/humidity with NTP sync

// NTP Server Configuration
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov", "time.google.com"

// Timezone Configuration - Defaults to Sydney, Australia - can be changed on web interface
// Uses POSIX TZ strings for automatic DST handling
// Format: STD offset DST [offset],start[/time],end[/time]
//   STD = Standard timezone abbreviation
//   offset = Hours offset from UTC (negative for west of Greenwich)
//   DST = Daylight saving timezone abbreviation  
//   start/end = When DST starts/ends (Mmonth.week.day/hour format)

// Australia Eastern Time (Sydney, Melbourne)
// AEST (UTC+10) / AEDT (UTC+11) - DST first Sunday in October to first Sunday in April

#define MY_TZ TZ_Australia_Sydney

// Alternate timezones (comment out above and uncomment one below if needed):
// US Eastern Time (New York)
// #define MY_TZ TZ_America_New_York

// US Pacific Time (Los Angeles)
// #define MY_TZ TZ_America_Los_Angeles

// UK (London)
// #define MY_TZ TZ_Europe_London

// Central European Time (Paris, Berlin, Rome)
// #define MY_TZ TZ_Europe_Paris

// Japan (Tokyo) - No DST
// #define MY_TZ TZ_Asia_Tokyo

// Custom TZ string example if your timezone isn't predefined:
// #define MY_TZ "AEST-10AEDT,M10.1.0,M4.1.0/3"
// Format breakdown for Sydney:
//   AEST-10 = Australian Eastern Standard Time, UTC+10 (negative means east)
//   AEDT = Australian Eastern Daylight Time
//   M10.1.0 = Month 10 (October), 1st occurrence, Sunday (0), at 2am (default)
//   M4.1.0/3 = Month 4 (April), 1st occurrence, Sunday (0), at 3am

// Debug Output
#define DEBUG_ENABLED true        // Set to false to disable serial output
#if DEBUG_ENABLED
  #define DEBUG(x) x
#else
  #define DEBUG(x)
#endif

// ======================== OBJECTS & GLOBALS ========================

// Sensors
Adafruit_BME280 bme280;

// Web Server
ESP8266WebServer server(80);

// WiFi Manager
WiFiManager wifiManager;

// Time Variables
int hours, minutes, seconds;
int hours24;                       // 24-hour clock for schedule logic
int day, month, year, dayOfWeek;
bool showDots = true;

// Time format
// NOTE: In 24-hour mode we intentionally do NOT render seconds on the 32x16 LED matrix.
// With this project’s current fonts/layout, "HH:MM:SS" cannot reliably fit in 32px width.
bool use24HourFormat = false;      // false = 12-hour (default), true = 24-hour

// Display Variables
int xPos = 0, yPos = 0;
int currentMode = 0;
char txt[32];

// Sensor Data
int temperature = 0;
int humidity = 0;
int pressure = 0;              // Pressure in hPa (BME280/BMP280)
bool sensorAvailable = false;

// Display Control
int brightness = 8;
int lightLevel = 512;
int previousLightLevel = 512;    // Track previous light level for change detection
bool lightLevelChanged = false;  // Flag to trigger webpage refresh on significant light change
int displayTimer = DISPLAY_TIMEOUT;
bool displayOn = true;
bool motionDetected = false;
bool brightnessManualOverride = false;  // Manual brightness mode flag
int manualBrightness = 4;               // Manual brightness value (1-15)
bool displayManualOverride = false;     // Manual display on/off override flag
unsigned long displayManualOverrideTimeout = 0;  // Timeout for manual override (in milliseconds)
#define DISPLAY_MANUAL_OVERRIDE_DURATION 300000  // 5 minutes

// Display OFF Schedule (OFF window)
// When enabled, the display is forced OFF between the OFF start and OFF end times.
bool scheduleOffEnabled = true;
int scheduleOffStartHour = 22;          // OFF from 22:00
int scheduleOffStartMinute = 0;
int scheduleOffEndHour = 6;             // ...until 06:00
int scheduleOffEndMinute = 0;

// Temperature Unit Configuration
bool useFahrenheit = false;             // false = Celsius (default), true = Fahrenheit

// Timezone Configuration
int currentTimezone = 0;                // Index into timezone array (0 = Australia/Sydney by default)

// Predefined timezone options (name and TZ string pairs)
// Note: TZ strings are stored in flash memory to save RAM
struct TimezoneOption {
  const char* name;
  const char* tzString;
};

// Timing
unsigned long lastNTPUpdate = 0;
unsigned long startupTime = 0;          // Track startup time for grace period
#define STARTUP_GRACE_PERIOD 10000      // 10 seconds - keep display on at startup
unsigned long lastModeChange = 0;

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

// ======================== TEMPERATURE HELPER FUNCTION ========================

int getDisplayTemperature() {
  if (useFahrenheit) {
    return (temperature * 9 / 5) + 32;  // Convert Celsius to Fahrenheit
  }
  return temperature;
}

char getTempUnit() {
  return useFahrenheit ? 'F' : 'C';
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

// Centralized display power/intensity application
int computeAmbientBrightnessFromLdr(int ldrValue);
void applyDisplayHardwareState(bool on, int intensity);

// ======================== SETUP ========================

void setup() 
{
  Serial.begin(115200);
  delay(100);
  
  // Record startup time for grace period
  startupTime = millis();
  
  printBanner();
  
  // Initialize display
  DEBUG(Serial.println("Initializing LED matrix..."));
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 5);
  
  // Initialize I2C for BMP/BME280
  DEBUG(Serial.println("Initializing I2C and BMP/BME280 sensor..."));
  // Initialize I2C with SDA on D2 (GPIO4) and SCL on D1 (GPIO5)
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

  // Handle brightness and motion detection (may change displayOn)
  handleBrightnessAndMotion();

  // Only render/refresh when display is actually ON.
  // This prevents needless SPI updates and avoids any weird state thrashing.
  if (displayOn) {
    switch (currentMode) {
      case 0: displayTimeAndTemp(); break;
      case 1: displayTimeLarge(); break;
      case 2: displayTimeAndDate(); break;
    }
    refreshAll();
  }
  
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
  if (use24HourFormat) {
    // 24-hour mode: show HH:MM (no seconds) due to 32px width constraint.
    xPos = 0;
    sprintf(txt, "%02d", hours24);
    printString(txt, digits5x8rn);
    if (showDots) printCharX(':', digits5x8rn, xPos);
    xPos += 2;
    sprintf(txt, "%02d", minutes);
    printString(txt, digits5x8rn);
  } else {
    // 12-hour mode: show H:MM:SS (seconds fit because hour is not zero-padded)
    xPos = (hours > 9) ? 0 : 2;
    sprintf(txt, "%d", hours);
    printString(txt, digits5x8rn);
    if (showDots) printCharX(':', digits5x8rn, xPos);
    xPos += (hours >= 20) ? 1 : 2;
    sprintf(txt, "%02d", minutes);
    printString(txt, digits5x8rn);
    sprintf(txt, "%02d", seconds);
    printString(txt, digits3x5);
  }
  
  // Bottom line: Temperature and Humidity
  yPos = 1;
  xPos = 1;
  if (sensorAvailable) {
    int displayTemp = getDisplayTemperature();
    char tempUnit = getTempUnit();
    sprintf(txt, "T%d%c H%d%%", displayTemp, tempUnit, humidity);
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

  // This mode uses a larger time font. Keep existing 12-hour rendering.
  // (24-hour support here would require more layout work than the 8px modes.)
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
  if (use24HourFormat) {
    // 24-hour mode: show HH:MM (no seconds) due to 32px width constraint.
    xPos = 0;
    sprintf(txt, "%02d", hours24);
    printString(txt, digits5x8rn);
    if (showDots) printCharX(':', digits5x8rn, xPos);
    xPos += 2;
    sprintf(txt, "%02d", minutes);
    printString(txt, digits5x8rn);
  } else {
    // 12-hour mode: show H:MM:SS
    xPos = (hours > 9) ? 0 : 2;
    sprintf(txt, "%d", hours);
    printString(txt, digits5x8rn);
    if (showDots) printCharX(':', digits5x8rn, xPos);
    xPos += (hours >= 20) ? 1 : 2;
    sprintf(txt, "%02d", minutes);
    printString(txt, digits5x8rn);
    sprintf(txt, "%02d", seconds);
    printString(txt, digits3x5);
  }
  
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

  // Get timezone string for selected timezone
  const char* tzString = timezones[currentTimezone].tzString;
  
  // Set timezone using modern TZ.h method with selected timezone
  configTime(tzString, NTP_SERVERS);
  
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
  DEBUG(Serial.printf("Time synced: %02d:%02d:%02d %02d/%02d/%d (TZ: %s)\n",
                      hours, minutes, seconds, day, month, year,
                      timezones[currentTimezone].name));
  return true;
}

void updateTime() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  // Keep 24-hour time for schedule logic
  hours24 = timeinfo->tm_hour;

  // Convert to 12-hour format for display rendering
  hours = (hours24 == 0) ? 12 : (hours24 > 12) ? hours24 - 12 : hours24;

  minutes = timeinfo->tm_min;
  seconds = timeinfo->tm_sec;
  day = timeinfo->tm_mday;
  month = timeinfo->tm_mon + 1;
  year = timeinfo->tm_year + 1900;
  dayOfWeek = timeinfo->tm_wday;
}

// ======================== SENSOR FUNCTIONS ========================

void testSensor() {
  DEBUG(Serial.print("Testing BME280 sensor... "));
  
  // Initialize BME280 at address 0x76
  if (!bme280.begin(0x76)) {
    sensorAvailable = false;
    DEBUG(Serial.println("NOT FOUND!"));
    DEBUG(Serial.println("  Check wiring: SDA->D2, SCL->D1, VCC->3.3V"));
    return;
  }
  
  // Set default BME280 settings
  bme280.setSampling(Adafruit_BME280::MODE_NORMAL,
                     Adafruit_BME280::SAMPLING_X2,
                     Adafruit_BME280::SAMPLING_X16,
                     Adafruit_BME280::SAMPLING_X1,
                     Adafruit_BME280::FILTER_X16,
                     Adafruit_BME280::STANDBY_MS_500);
  
  // Read first values to verify
  temperature = (int)bme280.readTemperature();
  pressure = (int)bme280.readPressure() / 100;  // Convert to hPa
  humidity = (int)bme280.readHumidity();
  
  // Check if readings are valid
  if (temperature < -40 || temperature > 85) {
    sensorAvailable = false;
    DEBUG(Serial.println("FAILED!"));
  } else {
    sensorAvailable = true;
    DEBUG(Serial.println("OK!"));
    DEBUG(Serial.printf("  Temperature: %d°C\n", temperature));
    DEBUG(Serial.printf("  Humidity: %d%%\n", humidity));
    DEBUG(Serial.printf("  Pressure: %d hPa\n", pressure));
  }
}

void updateSensorData() {
  // Read sensor values
  temperature = (int)bme280.readTemperature();
  pressure = (int)bme280.readPressure() / 100;  // Convert Pa to hPa
  humidity = (int)bme280.readHumidity();
  
  // Check if readings are valid
  if (temperature < -40 || temperature > 85 || pressure < 300 || pressure > 1200 || humidity < 0 || humidity > 100) {
    sensorAvailable = false;
    DEBUG(Serial.println("Sensor read failed!"));
  } else {
    sensorAvailable = true;
    DEBUG(Serial.printf("Sensor: %d°C, %d%% humidity, %d hPa\n", temperature, humidity, pressure));
  }
}

// ======================== BRIGHTNESS & MOTION ========================

int computeAmbientBrightnessFromLdr(int ldrValue) {
  // Inverted mapping: darker -> lower brightness, brighter -> higher brightness
  return 15 - map(constrain(ldrValue, 0, 1023), 0, 1023, 1, 15);
}

void applyDisplayHardwareState(bool on, int intensity) {
  // Intensity is only meaningful when ON. Clamp defensively.
  int clamped = constrain(intensity, 0, 15);

  sendCmdAll(CMD_SHUTDOWN, on ? 1 : 0);
  if (on) {
    sendCmdAll(CMD_INTENSITY, clamped);
  }
}

// Returns true when we are inside the scheduled OFF period.
// NOTE: Schedule inputs are in 24-hour time.
bool isWithinScheduleOffWindow() {
  if (!scheduleOffEnabled) return false;  // Schedule disabled => no forced off window

  // If start==end, treat as "no scheduled off time" to avoid accidental 24h shutdown.
  if (scheduleOffStartHour == scheduleOffEndHour && scheduleOffStartMinute == scheduleOffEndMinute) {
    return false;
  }

  // Convert current time to minutes since midnight
  int currentMinutes = hours24 * 60 + minutes;
  int offStartMinutes = scheduleOffStartHour * 60 + scheduleOffStartMinute;
  int offEndMinutes = scheduleOffEndHour * 60 + scheduleOffEndMinute;

  // Handle case where OFF window spans midnight (e.g., 22:00 -> 06:00)
  if (offStartMinutes < offEndMinutes) {
    // Normal case: same-day OFF window
    return currentMinutes >= offStartMinutes && currentMinutes < offEndMinutes;
  }

  // Wrap-midnight case
  return currentMinutes >= offStartMinutes || currentMinutes < offEndMinutes;
}

void handleBrightnessAndMotion() {
  // During startup grace period, keep display on with fresh motion detection
  if (millis() - startupTime < STARTUP_GRACE_PERIOD) {
    displayOn = true;
    displayTimer = DISPLAY_TIMEOUT;  // Reset display timer to keep it on

    int ambientBrightness = computeAmbientBrightnessFromLdr(analogRead(LDR_PIN));
    brightness = brightnessManualOverride ? manualBrightness : ambientBrightness;
    applyDisplayHardwareState(true, brightness);
    return;
  }
  
  // Read ambient light
  lightLevel = analogRead(LDR_PIN);
  
  // Check if light level has changed by ±5%
  int lightDifference = abs(lightLevel - previousLightLevel);
  int changeThreshold = (previousLightLevel > 0) ? (previousLightLevel * 5 / 100) : 51;  // 5% of previous level, minimum 51
  if (lightDifference >= changeThreshold) {
    lightLevelChanged = true;
    DEBUG(Serial.printf("Light level changed: %d -> %d (diff: %d, threshold: %d)\n", previousLightLevel, lightLevel, lightDifference, changeThreshold));
  }
  previousLightLevel = lightLevel;  // Always update for next comparison
  
  // Map to brightness (inverted: darker = dimmer)
  int ambientBrightness = computeAmbientBrightnessFromLdr(lightLevel);
  
  // Check motion
  motionDetected = digitalRead(PIR_PIN);
  
  // Check if we are inside the scheduled OFF window
  bool withinOffWindow = isWithinScheduleOffWindow();
  
  // Check if manual override timeout has expired
  if (displayManualOverride && millis() > displayManualOverrideTimeout) {
    displayManualOverride = false;
    DEBUG(Serial.println("Display manual override timeout - reverting to automatic control"));
  }
  
  // If manual override is active, respect the user's choice
  if (displayManualOverride) {
    // Manual override is active - only adjust brightness if on, don't change on/off state
    if (displayOn) {
      brightness = brightnessManualOverride ? manualBrightness : ambientBrightness;
      applyDisplayHardwareState(true, brightness);
    }
    return;
  }
  
  // If inside OFF window, schedule always wins: force display off
  if (withinOffWindow) {
    if (displayOn) {
      displayOn = false;
      applyDisplayHardwareState(false, 0);
      DEBUG(Serial.printf("Display forced OFF by schedule (%02d:%02d-%02d:%02d)\n",
                          scheduleOffStartHour, scheduleOffStartMinute,
                          scheduleOffEndHour, scheduleOffEndMinute));
    }
    displayTimer = 0;
    return;
  }

  // Outside OFF window => normal motion/timer behavior
  if (motionDetected) {
    // Motion detected - turn on and reset timer
    displayTimer = DISPLAY_TIMEOUT;
    displayOn = true;
    // Use manual brightness if override is enabled, otherwise use ambient
    brightness = brightnessManualOverride ? manualBrightness : ambientBrightness;
    applyDisplayHardwareState(true, brightness);
  } else {
    // No motion - countdown
    if (displayTimer > 0) {
      displayTimer--;
      // Fade out gradually (respect manual override if set)
      int targetBrightness = brightnessManualOverride ? manualBrightness : ambientBrightness;
      brightness = map(displayTimer, 0, DISPLAY_TIMEOUT, 1, targetBrightness);
      applyDisplayHardwareState(true, brightness);
    } else {
      // Timer expired - turn off
      if (displayOn) {
        displayOn = false;
        applyDisplayHardwareState(false, 0);
      }
      displayTimer = 0;
    }
  }
}

// ======================== WEB SERVER ========================

void setupWebServer() {
  // Root page
  server.on("/", []() {
    String html = "<!DOCTYPE html><html><head><title>LED Clock</title>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<link rel='icon' href='data:,'>";
    html += "<link href='https://fonts.googleapis.com/css2?family=Orbitron:wght@700;900&display=swap' rel='stylesheet'>";
    html += "<style>body{font-family:Arial;margin:20px;background:#f0f0f0;}";
    html += ".card{background:white;padding:20px;margin:10px;border-radius:10px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
    html += "h1{color:#333;}";
    html += ".digital-time{font-family:'Orbitron',monospace;font-size:72px;font-weight:900;color:#00ff00;text-shadow:0 0 10px #00ff00;letter-spacing:0.1em;margin:10px 0;}";
    html += ".digital-date{font-family:'Orbitron',monospace;font-size:28px;font-weight:700;color:#0080ff;letter-spacing:0.05em;margin:10px 0;}";
    html += ".light-container{display:flex;align-items:center;gap:10px;margin:10px 0;}";
    html += ".light-icon{font-size:24px;min-width:30px;text-align:center;}";
    html += ".light-bar-bg{flex-grow:1;height:30px;background:#e0e0e0;border-radius:15px;overflow:hidden;position:relative;}";
    html += ".light-bar-fill{height:100%;background:linear-gradient(90deg,#1a1a1a 0%,#ffeb3b 50%,#fff9c4 100%);transition:width 0.3s ease;border-radius:15px;}";
    html += "#led-mirror{background:#000;padding:20px;border-radius:10px;display:inline-block;box-shadow:inset 0 0 20px rgba(0,0,0,0.5);position:relative;}";
    html += "#led-canvas{image-rendering:pixelated;image-rendering:-moz-crisp-edges;image-rendering:crisp-edges;}";
    html += "#display-off-msg{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);color:#ff0000;font-family:'Orbitron',monospace;font-size:120px;font-weight:900;text-align:center;text-shadow:0 0 20px #ff0000,0 0 40px #ff0000;line-height:1.1;display:none;pointer-events:none;}</style>";
    html += "<script>";
    html += "var connErr=document.getElementById('conn-error');";
    html += "var ledCanvas, ledCtx;";
    html += "var isDisplayOn=true;";
    html += "function showError(msg){if(connErr){connErr.style.display='block';connErr.innerText=msg;}}";
    html += "function hideError(){if(connErr)connErr.style.display='none';}";
    html += "function updateDisplay() {";
    html += "  if(!isDisplayOn){";
    html += "    if(!ledCanvas){ledCanvas=document.getElementById('led-canvas');if(ledCanvas) ledCtx=ledCanvas.getContext('2d');}";
    html += "    if(ledCtx&&ledCanvas){";
    html += "      ledCtx.clearRect(0,0,ledCanvas.width,ledCanvas.height);";
    html += "      ledCtx.fillStyle='#000';";
    html += "      ledCtx.fillRect(0,0,ledCanvas.width,ledCanvas.height);";
    html += "    }";
    html += "    return;";
    html += "  }";
    html += "  fetch('/api/display').then(r=>r.json()).then(d=>{";
    html += "    if(!ledCanvas){ledCanvas=document.getElementById('led-canvas');ledCtx=ledCanvas.getContext('2d');}";
    html += "    if(!ledCanvas) return;";
    html += "    let w=d.width,h=d.height;";
    html += "    ledCanvas.width=w;ledCanvas.height=h;";
    html += "    let imgData=ledCtx.createImageData(w,h);";
    html += "    let pixels=d.pixels;";
    html += "    for(let i=0;i<pixels.length;i++){";
    html += "      let isOn=pixels[i]==='1';";
    html += "      let r=isOn?255:0;";
    html += "      imgData.data[i*4]=r;";
    html += "      imgData.data[i*4+1]=0;";
    html += "      imgData.data[i*4+2]=0;";
    html += "      imgData.data[i*4+3]=255;";
    html += "    }";
    html += "    ledCtx.putImageData(imgData,0,0);";
    html += "  }).catch(e=>console.log('Display update failed'));";
    html += "}";
    html += "function updateAll() {";
    html += "  fetch('/api/all').then(r=>r.json()).then(d=>{";
    html += "    hideError();";
    html += "    document.getElementById('time-display').innerText = d.time;";
    html += "    document.getElementById('date-display').innerText = d.date;";
    html += "    document.getElementById('display-status').innerText = d.display;";
    html += "    let displayOn = d.display === 'ON';";
    html += "    isDisplayOn = displayOn;";
    html += "    let displayBtn = document.getElementById('display-toggle-button');";
    html += "    let offMsg = document.getElementById('display-off-msg');";
    html += "    if(offMsg) offMsg.style.display = displayOn ? 'none' : 'block';";
    html += "    if(!displayOn){";
    html += "      if(!ledCanvas){ledCanvas=document.getElementById('led-canvas');if(ledCanvas) ledCtx=ledCanvas.getContext('2d');}";
    html += "      if(ledCtx&&ledCanvas){";
    html += "        ledCtx.clearRect(0,0,ledCanvas.width,ledCanvas.height);";
    html += "        ledCtx.fillStyle='#000';";
    html += "        ledCtx.fillRect(0,0,ledCanvas.width,ledCanvas.height);";
    html += "      }";
    html += "    }";
    html += "    if (displayBtn) displayBtn.innerText = displayOn ? 'Turn OFF' : 'Turn ON';";
    html += "    document.getElementById('motion-status').innerText = d.motion;";
    html += "    document.getElementById('brightness-status').innerText = d.brightness + '/15';";
    html += "    document.getElementById('ldr-status').innerText = d.light;";
    html += "    let lightPercent = 100 - Math.round((d.light / 1023) * 100);";
    html += "    document.getElementById('light-bar').style.width = lightPercent + '%';";
    html += "    let manualMode = d.mode === 'Manual';";
    html += "    document.getElementById('brightness-mode-status').innerText = manualMode ? 'Manual' : 'Automatic';";
    html += "    document.getElementById('brightness-mode-button').innerText = manualMode ? 'Switch to Auto' : 'Switch to Manual';";
    html += "    let manualControl = document.getElementById('manual-brightness-control');";
    html += "    if (manualControl) {";
    html += "      manualControl.style.display = manualMode ? 'block' : 'none';";
    html += "      if (manualMode) {";
    html += "        let slider = document.getElementById('manual-brightness-slider');";
    html += "        if (slider) slider.value = d.manual_brightness;";
    html += "      }";
    html += "    }";
    html += "    let timeFormat = document.getElementById('time-format-display');";
    html += "    if (timeFormat) timeFormat.innerText = d.use_24_hour ? '24-hour' : '12-hour';";
    html += "    let timeFormatBtn = document.getElementById('time-format-button');";
    html += "    if (timeFormatBtn) timeFormatBtn.innerText = d.use_24_hour ? 'Switch to 12-hour' : 'Switch to 24-hour';";
    html += "    document.getElementById('temp-unit-display').innerHTML = d.temp_unit;";
    html += "    let tempBtn = document.getElementById('temperature-button');";
    html += "    if (tempBtn) tempBtn.innerText = d.temp_unit_short === 'F' ? 'Switch to Celsius' : 'Switch to Fahrenheit';";
    html += "    if (d.sensor_available) {";
    html += "      document.getElementById('sensor-data').innerHTML = 'Temperature: ' + d.temperature + '&deg;' + d.temp_unit_short + ' | Humidity: ' + d.humidity + '% | Pressure: ' + d.pressure + ' hPa';";
    html += "    }";
    html += "    let scheduleNotice = document.getElementById('schedule-notice');";
    html += "    if (!d.schedule_enabled || d.within_schedule) {";
    html += "      scheduleNotice.style.display = 'block';";
    html += "      scheduleNotice.innerText = d.within_schedule ? 'Display OFF: Scheduled (' + d.schedule_start + '-' + d.schedule_end + ')' : 'Schedule: Disabled';";
    html += "    } else {";
    html += "      scheduleNotice.style.display = 'none';";
    html += "    }";
    html += "    let tzName = document.getElementById('timezone-name');";
    html += "    if (tzName && d.timezone_name) tzName.innerText = d.timezone_name;";
    html += "    if(d.light_changed) updateAll();";
    html += "  }).catch(e=>showError('Connection lost - retrying...'));";
    html += "}";
    html += "function toggleDisplay() {";
    html += "  fetch('/display?mode=toggle').then(()=>updateAll()).catch(e=>showError('Request failed'));";
    html += "}";
    html += "function toggleBrightnessMode() {";
    html += "  fetch('/brightness?mode=toggle').then(()=>updateAll()).catch(e=>showError('Request failed'));";
    html += "}";
    html += "function setManualBrightness(value) {";
    html += "  fetch('/brightness?value=' + value).then(()=>updateAll()).catch(e=>showError('Request failed'));";
    html += "}";
    html += "function toggleTimeFormat() {";
    html += "  fetch('/timeformat?mode=toggle').then(()=>updateAll()).catch(e=>showError('Request failed'));";
    html += "}";
    html += "function toggleTemperatureUnit() {";
    html += "  fetch('/temperature?mode=toggle').then(()=>updateAll()).catch(e=>showError('Request failed'));";
    html += "}";
    html += "function setTimezone() {";
    html += "  let tz = document.getElementById('tz-select').value;";
    html += "  fetch('/timezone?tz=' + tz).then(()=>updateAll()).catch(e=>showError('Request failed'));";
    html += "}";
    html += "function saveSchedule() {";
    html += "  let en = document.getElementById('sched-enabled').checked ? '1' : '0';";
    html += "  let sh = document.getElementById('sched-start-hour').value;";
    html += "  let sm = document.getElementById('sched-start-min').value;";
    html += "  let eh = document.getElementById('sched-end-hour').value;";
    html += "  let em = document.getElementById('sched-end-min').value;";
    html += "  fetch('/schedule?enabled=' + en + '&start_hour=' + sh + '&start_min=' + sm + '&end_hour=' + eh + '&end_min=' + em)";
    html += "    .then(()=>updateAll()).catch(e=>showError('Request failed'));";
    html += "}";
    html += "window.addEventListener('load', function() {";
    html += "  connErr=document.getElementById('conn-error');";
    html += "  updateAll();";
    html += "  updateDisplay();";
    html += "  setInterval(updateAll, 2000);";
    html += "  setInterval(updateDisplay, 500);";
    html += "});";
    html += "</script>";
    html += "</head><body>";
    html += "<h1>LED Matrix Clock</h1>";
    html += "<div class='card'><h2>Current Time (<span id='timezone-name'>" + String(timezones[currentTimezone].name) + "</span>) &amp; Environment</h2>";
    html += "<p class='digital-time' id='time-display'>" + String(hours) + ":" +
            (minutes < 10 ? "0" : "") + String(minutes) + ":" +
            (seconds < 10 ? "0" : "") + String(seconds);
    if (!use24HourFormat) {
      html += (hours24 < 12) ? " AM" : " PM";
    }
    html += "</p>";
    html += "<p class='digital-date' id='date-display'>" + String(day) + "/" + String(month) + "/" + String(year) + "</p>";
    
    if (sensorAvailable) {
      int displayTemp = getDisplayTemperature();
      char tempUnit = getTempUnit();
      html += "<p id='sensor-data'>Temperature: " + String(displayTemp) + "&deg;" + String(tempUnit) + " | Humidity: " + String(humidity) + "% | Pressure: " + String(pressure) + " hPa</p>";
    } else {
      html += "<p id='sensor-data'>Sensor not available</p>";
    }
    
    // Light level bar chart with icons (reversed: low=bright, high=dark)
    int lightPercent = 100 - map(lightLevel, 0, 1023, 0, 100);
    html += "<div class='light-container'>";
    html += "<span class='light-icon'>🌙</span>";
    html += "<div class='light-bar-bg'>";
    html += "<div class='light-bar-fill' id='light-bar' style='width:" + String(lightPercent) + "%'></div>";
    html += "</div>";
    html += "<span class='light-icon'>☀️</span>";
    html += "</div>";
    html += "</div>";

    html += "<div class='card'><h2>LED Display Mirror</h2>";
    html += "<p style='color:#666;font-size:14px;'>Live display - Updates every 500ms | 32×16 LED Matrix</p>";
    html += "<div id='led-mirror'>";
    html += "<canvas id='led-canvas' width='32' height='16' style='width:640px;height:320px;'></canvas>";
    html += "<div id='display-off-msg'>Display<br>Off</div>";
    html += "</div></div>";

    html += "<div class='card'><h2>Status &amp; Configuration</h2>";
    html += "<p style='color:red;font-weight:bold;display:none;' id='conn-error'></p>";
    html += "<p style='color:red;font-weight:bold;display:none;' id='schedule-notice'></p>";
    
    // Status items
    html += "<h3 style='margin-top:0;'>Status</h3>";
    html += "<p>Display: <span id='display-status'>" + String(displayOn ? "ON" : "OFF") + "</span> ";
    html += "<button id='display-toggle-button' onclick=\"toggleDisplay()\" style='padding:5px 10px;cursor:pointer;'>";
    html += String(displayOn ? "Turn OFF" : "Turn ON");
    html += "</button></p>";
    html += "<p>Motion: <span id='motion-status'>" + String(motionDetected ? "Detected" : "None") + "</span></p>";
    html += "<p>Display Brightness: <span id='brightness-mode-status'>" + String(brightnessManualOverride ? "Manual" : "Automatic") + "</span> ";
    html += "<button id='brightness-mode-button' onclick=\"toggleBrightnessMode()\" style='padding:5px 10px;cursor:pointer;'>";
    html += String(brightnessManualOverride ? "Switch to Auto" : "Switch to Manual");
    html += "</button></p>";
    
    // Configuration items
    html += "<hr style='margin:15px 0;'>";
    html += "<h3>Configuration</h3>";
    
    // Brightness Control Section
    html += "<h4 style='margin-top:10px;margin-bottom:5px;'>Brightness Control</h4>";
    html += "<p>LDR Raw Reading: <span id='ldr-status'>" + String(lightLevel) + "</span>, calculating Display Brightness to: <span id='brightness-status'>" + String(brightness) + "/15</span></p>";
    html += "<div id='manual-brightness-control' style='" + String(brightnessManualOverride ? "" : "display:none;") + "margin-top:5px;'>";
    html += "<p><label>Manual Brightness: <input type='range' min='1' max='15' id='manual-brightness-slider' value='" + String(manualBrightness) + "' onchange=\"setManualBrightness(this.value)\"></label></p>";
    html += "</div>";
    
    // Time Format Section
    html += "<h4 style='margin-top:15px;margin-bottom:5px;'>Time Format</h4>";
    html += "<p>LED Matrix Format: <strong id='time-format-display'>" + String(use24HourFormat ? "24-hour" : "12-hour") + "</strong> ";
    html += "<button id='time-format-button' onclick=\"toggleTimeFormat()\" style='padding:5px 10px;cursor:pointer;'>";
    html += String(use24HourFormat ? "Switch to 12-hour" : "Switch to 24-hour");
    html += "</button></p>";
    html += "<p style='font-size:12px;color:#666;margin-top:-5px;'>";
    html += "Note: In 24-hour mode the LED matrix shows HH:MM (no seconds) due to 32px display width limitations.";
    html += "</p>";

    // Temperature Unit Section
    html += "<h4 style='margin-top:15px;margin-bottom:5px;'>Temperature Unit</h4>";
    html += "<p>Current Unit: <span id='temp-unit-display'>" + String(useFahrenheit ? "Fahrenheit (&deg;F)" : "Celsius (&deg;C)") + "</span> ";
    html += "<button id='temperature-button' onclick=\"toggleTemperatureUnit()\" style='padding:5px 10px;cursor:pointer;'>";
    html += String(useFahrenheit ? "Switch to Celsius" : "Switch to Fahrenheit");
    html += "</button></p>";
    
    // Timezone Selection Section (auto-updates on change)
    html += "<h4 style='margin-top:15px;margin-bottom:5px;'>Timezone</h4>";
    html += "<p><label>Select Timezone: <select id='tz-select' onchange='setTimezone()' style='padding:5px;'>";
    for (int i = 0; i < numTimezones; i++) {
      html += "<option value='" + String(i) + "'" + String(i == currentTimezone ? " selected" : "") + ">";
      html += String(timezones[i].name);
      html += "</option>";
    }
    html += "</select></label></p>";
    
    // Display Schedule Section (AJAX, no form POST)
    html += "<h4 style='margin-top:15px;margin-bottom:5px;'>Display Schedule</h4>";
    html += "<p><label><input type='checkbox' id='sched-enabled' " + String(scheduleOffEnabled ? "checked" : "") + "> Enable Schedule</label></p>";
    html += "<p><label>Turn OFF from: <input type='number' id='sched-start-hour' min='0' max='23' value='" + String(scheduleOffStartHour) + "' style='width:50px;'>:";
    html += "<input type='number' id='sched-start-min' min='0' max='59' value='" + String(scheduleOffStartMinute < 10 ? "0" : "") + String(scheduleOffStartMinute) + "' style='width:50px;'></label></p>";
    html += "<p><label>Turn ON at: <input type='number' id='sched-end-hour' min='0' max='23' value='" + String(scheduleOffEndHour) + "' style='width:50px;'>:";
    html += "<input type='number' id='sched-end-min' min='0' max='59' value='" + String(scheduleOffEndMinute < 10 ? "0" : "") + String(scheduleOffEndMinute) + "' style='width:50px;'></label></p>";
    html += "<p><button onclick='saveSchedule()' style='padding:5px 10px;cursor:pointer;'>Save Schedule</button></p>";
    html += "</div>";
    
    html += "<div class='card'><p><a href='/reset'>Reset WiFi Settings</a></p></div>";

    // Footer information section
    html += "<div class='card' style='text-align:center;padding:15px;margin-top:20px;'>";
    html += "<p style='margin:5px 0;font-size:14px;color:#666;'>ESP8266 LED Matrix Clock</p>";
    html += "<p style='margin:5px 0;'>";
    html += "<a href='https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock' target='_blank' style='color:#0066cc;text-decoration:none;margin:0 10px;'>GitHub</a> | ";
    html += "<a href='https://bsky.app/profile/anthonyjclarke.bsky.social' target='_blank' style='color:#0066cc;text-decoration:none;margin:0 10px;'>Bluesky</a>";
    html += "</p>";
    html += "<p style='margin:5px 0;font-size:12px;color:#999;'>Built with ❤️ by Anthony Clarke</p>";
    html += "</div>";

    html += "</body></html>";
    
    server.send(200, "text/html", html);
  });
  
  // Display buffer API endpoint - returns current LED matrix state
  // This mimics the refreshAllRot90() logic to properly decode the buffer
  server.on("/api/display", []() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");

    // Build a properly oriented pixel buffer based on rotation
    String pixelData = "";
    pixelData.reserve(LINE_WIDTH * 16);  // 32x16 = 512 pixels

#if ROTATE==90
    // For 90-degree rotation with 8 modules arranged as 4×2 (32×16 display)
    // Modules are numbered 0-7, with 0-3 being top row, 4-7 being bottom row
    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < LINE_WIDTH; x++) {
        // Determine which module (4 modules across, 2 modules down)
        int moduleCol = x / 8;        // 0-3
        int moduleRow = y / 8;        // 0-1
        int moduleIdx = moduleRow * 4 + moduleCol;  // 0-7

        int colInModule = x % 8;
        int rowInModule = y % 8;

        // For rotation 90, we need to invert the bit order
        // The mask should start from 0x01 and shift left, not from 0x80
        byte mask = 0x01 << rowInModule;
        int byteOffset = moduleIdx * 8 + colInModule;

        bool isOn = (scr[byteOffset] & mask) != 0;
        pixelData += isOn ? "1" : "0";
      }
    }
#else
    // For no rotation
    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < LINE_WIDTH; x++) {
        int moduleCol = x / 8;
        int moduleRow = y / 8;
        int moduleIdx = moduleRow * 4 + moduleCol;

        int colInModule = x % 8;
        int rowInModule = y % 8;

        byte mask = 1 << rowInModule;
        int byteOffset = moduleIdx * 8 + colInModule;

        bool isOn = (scr[byteOffset] & mask) != 0;
        pixelData += isOn ? "1" : "0";
      }
    }
#endif

    String json = "{\"pixels\":\"";
    json += pixelData;
    json += "\",\"width\":32,\"height\":16}";

    server.send(200, "application/json", json);
  });

  // Combined API endpoint for all data (time + status + light change)
  server.on("/api/all", []() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");

    bool withinOffWindow = isWithinScheduleOffWindow();
    int displayTemp = getDisplayTemperature();

    String json = "{\"time\":\"";
    json += String(hours) + ":";
    json += (minutes < 10 ? "0" : "") + String(minutes) + ":";
    json += (seconds < 10 ? "0" : "") + String(seconds);
    // Add AM/PM for 12-hour mode
    if (!use24HourFormat) {
      json += (hours24 < 12) ? " AM" : " PM";
    }
    json += "\",\"date\":\"";
    json += String(day) + "/" + String(month) + "/" + String(year);
    json += "\",\"display\":\"";
    json += String(displayOn ? "ON" : "OFF");
    json += "\",\"motion\":\"";
    json += String(motionDetected ? "Detected" : "None");
    json += "\",\"brightness\":";
    json += String(brightness);
    json += ",\"manual_brightness\":";
    json += String(manualBrightness);
    json += ",\"use_24_hour\":";
    json += String(use24HourFormat ? "true" : "false");
    json += ",\"light\":";
    json += String(lightLevel);
    json += ",\"light_changed\":";
    json += String(lightLevelChanged ? "true" : "false");
    json += ",\"mode\":\"";
    json += String(brightnessManualOverride ? "Manual" : "Auto");
    json += "\",\"temp_unit\":\"";
    json += String(useFahrenheit ? "Fahrenheit (&deg;F)" : "Celsius (&deg;C)");
    json += "\",\"temp_unit_short\":\"";
    json += String(useFahrenheit ? "F" : "C");
    json += "\",\"temperature\":";
    json += String(displayTemp);
    json += ",\"humidity\":";
    json += String(humidity);
    json += ",\"pressure\":";
    json += String(pressure);
    json += ",\"sensor_available\":";
    json += String(sensorAvailable ? "true" : "false");
    json += ",\"schedule_enabled\":";
    json += String(scheduleOffEnabled ? "true" : "false");
    json += ",\"within_schedule\":";
    json += String(withinOffWindow ? "true" : "false");
    json += ",\"schedule_start\":\"";
    json += (scheduleOffStartHour < 10 ? "0" : "") + String(scheduleOffStartHour) + ":";
    json += (scheduleOffStartMinute < 10 ? "0" : "") + String(scheduleOffStartMinute);
    json += "\",\"schedule_end\":\"";
    json += (scheduleOffEndHour < 10 ? "0" : "") + String(scheduleOffEndHour) + ":";
    json += (scheduleOffEndMinute < 10 ? "0" : "") + String(scheduleOffEndMinute);
    json += "\",\"timezone_name\":\"";
    json += String(timezones[currentTimezone].name);
    json += "\"}";


    // Reset light changed flag after reading
    if (lightLevelChanged) {
      lightLevelChanged = false;
    }

    server.send(200, "application/json", json);
  });
  
  // Brightness control endpoint
  server.on("/brightness", []() {
    if (server.hasArg("mode")) {
      // Toggle auto/manual mode
      brightnessManualOverride = !brightnessManualOverride;
      DEBUG(Serial.printf("Brightness mode: %s\n", brightnessManualOverride ? "Manual" : "Auto"));
      server.send(200, "text/plain", "OK");
      return;
    }
    if (server.hasArg("value")) {
      // Set manual brightness
      int newBrightness = server.arg("value").toInt();
      manualBrightness = constrain(newBrightness, 1, 15);
      brightness = manualBrightness;
      sendCmdAll(CMD_INTENSITY, brightness);
      DEBUG(Serial.printf("Manual brightness set to: %d\n", manualBrightness));
    }
    server.send(200, "text/plain", "OK");
  });
  
  // Time format toggle endpoint
  server.on("/timeformat", []() {
    if (server.hasArg("mode")) {
      use24HourFormat = !use24HourFormat;
      DEBUG(Serial.printf("Time format: %s\n", use24HourFormat ? "24-hour" : "12-hour"));

      // Force an immediate redraw so the user sees the change instantly on the matrix
      if (displayOn) {
        switch (currentMode) {
          case 0: displayTimeAndTemp(); break;
          case 1: displayTimeLarge(); break;
          case 2: displayTimeAndDate(); break;
        }
        refreshAll();
      }

      server.send(200, "text/plain", "OK");
      return;
    }
    server.send(200, "text/plain", "OK");
  });

  // Temperature unit toggle endpoint
  server.on("/temperature", []() {
    if (server.hasArg("mode")) {
      // Toggle Celsius/Fahrenheit
      useFahrenheit = !useFahrenheit;
      DEBUG(Serial.printf("Temperature unit: %s\n", useFahrenheit ? "Fahrenheit" : "Celsius"));

      // Force an immediate redraw so the user sees the change instantly on the matrix
      if (displayOn) {
        switch (currentMode) {
          case 0: displayTimeAndTemp(); break;
          case 1: displayTimeLarge(); break;
          case 2: displayTimeAndDate(); break;
        }
        refreshAll();
      }

      server.send(200, "text/plain", "OK");
      return;
    }
    server.send(200, "text/plain", "OK");
  });
  
  // Timezone configuration endpoint (now accepts GET for AJAX)
  server.on("/timezone", []() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    if (server.hasArg("tz")) {
      int newTimezone = server.arg("tz").toInt();
      if (newTimezone >= 0 && newTimezone < numTimezones) {
        currentTimezone = newTimezone;
        DEBUG(Serial.printf("Timezone changed to: %s\n", timezones[currentTimezone].name));
        
        // Re-sync time with new timezone
        syncNTP();
      }
    }
    server.send(200, "text/plain", "OK");
  });
  
  // Display on/off toggle endpoint
  server.on("/display", []() {
    if (server.hasArg("mode")) {
      // Toggle display on/off
      displayOn = !displayOn;
      displayManualOverride = true;
      displayManualOverrideTimeout = millis() + DISPLAY_MANUAL_OVERRIDE_DURATION;

      // Take a fresh LDR reading so turning ON picks a sane intensity immediately,
      // rather than relying on a potentially stale `lightLevel` value.
      int ldrNow = analogRead(LDR_PIN);
      lightLevel = ldrNow;

      if (displayOn) {
        int ambientBrightness = computeAmbientBrightnessFromLdr(ldrNow);
        brightness = brightnessManualOverride ? manualBrightness : ambientBrightness;
        applyDisplayHardwareState(true, brightness);
        DEBUG(Serial.printf("Display toggled ON (manual override for 5 minutes)\n"));
      } else {
        applyDisplayHardwareState(false, 0);
        DEBUG(Serial.printf("Display toggled OFF (manual override for 5 minutes)\n"));
      }
      server.send(200, "text/plain", "OK");
      return;
    }
    server.send(200, "text/plain", "OK");
  });
  
  // Schedule configuration endpoint (now accepts GET for AJAX)
  server.on("/schedule", []() {
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    
    // Check for 'enabled' param - if present and '1', enable; otherwise disable
    if (server.hasArg("enabled")) {
      scheduleOffEnabled = (server.arg("enabled") == "1");
    }
    
    if (server.hasArg("start_hour")) {
      scheduleOffStartHour = constrain(server.arg("start_hour").toInt(), 0, 23);
    }
    if (server.hasArg("start_min")) {
      scheduleOffStartMinute = constrain(server.arg("start_min").toInt(), 0, 59);
    }
    if (server.hasArg("end_hour")) {
      scheduleOffEndHour = constrain(server.arg("end_hour").toInt(), 0, 23);
    }
    if (server.hasArg("end_min")) {
      scheduleOffEndMinute = constrain(server.arg("end_min").toInt(), 0, 59);
    }
    
    DEBUG(Serial.printf("Schedule updated - Enabled: %s, OFF: %02d:%02d-%02d:%02d\n",
                        scheduleOffEnabled ? "Yes" : "No", 
                        scheduleOffStartHour, scheduleOffStartMinute,
                        scheduleOffEndHour, scheduleOffEndMinute));
    
    server.send(200, "text/plain", "OK");
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
  Serial.println("║   ESP8266 LED Matrix Clock v2.8        ║");
  Serial.println("║   PlatformIO Edition                   ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
}

void printStatus() {
  Serial.println("--- Status ---");

  // Time and Date
  if (use24HourFormat) {
    Serial.printf("Time: %02d:%02d:%02d | Date: %02d/%02d/%d",
                  hours24, minutes, seconds, day, month, year);
  } else {
    Serial.printf("Time: %02d:%02d:%02d %s | Date: %02d/%02d/%d",
                  hours, minutes, seconds, (hours24 < 12) ? "AM" : "PM", day, month, year);
  }

  // Sensor data
  if (sensorAvailable) {
    Serial.printf(" | Temp: %d°C | Humidity: %d%%", temperature, humidity);
  } else {
    Serial.print(" | Sensor Not Available");
  }
  Serial.println();

  // Light and Brightness
  Serial.printf("Light: %d | Bright: %d\n", lightLevel, brightness);

  // Motion, Display, Timer, Schedule
  bool withinOffWindow = isWithinScheduleOffWindow();
  String scheduleStatus;
  if (!scheduleOffEnabled) {
    scheduleStatus = "DISABLED";
  } else if (withinOffWindow) {
    scheduleStatus = "ACTIVE-OFF";  // Inside OFF window
  } else {
    scheduleStatus = "ACTIVE";      // Outside OFF window
  }

  Serial.printf("Motion: %s | Display: %s | Timer: %d | Schedule: %s (%02d:%02d-%02d:%02d)\n",
                motionDetected ? "YES" : "NO",
                displayOn ? "ON" : "OFF",
                displayTimer,
                scheduleStatus.c_str(),
                scheduleOffStartHour, scheduleOffStartMinute,
                scheduleOffEndHour, scheduleOffEndMinute);
  Serial.println();
}

// ======================== END OF CODE ========================