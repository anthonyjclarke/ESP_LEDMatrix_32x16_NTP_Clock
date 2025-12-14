#include "Arduino.h"

/*
 * ESP8266 LED Matrix Clock - Modern Edition
 * Author: Rewritten from original by Anthony Clarke
 * Acknowledgements: Original by @cbm80amiga here - https://www.youtube.com/watch?v=2wJOdi0xzas&t=32s
 * 
 * =========================== TODO ==========================
 * - Enable switching between 12/24 hour formats
 * - Switch between Celsius/Fahrenheit
 * - Switch between different timezones via web interface
 * - Switch LDR sensitivity via web interface and On/Off
 * - Implement web interface for full configuration
 * - Add OTA firmware update capability
 * 
 * ======================== CHANGELOG ========================
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
    - Initial Github Repo
    - Migrated to PlatformIO
    - Sensor Code NOT Fully tested, Clock / Date and Time working
    - Move fonts to separate header file
    - Moved MAX7219 functions to separate header file
    - PlatformIO Version with WiFiManager

 * 
 * v1.0 - October 2025
 *        Complete rewrite with modern practices:
 *        - WiFiManager for easy WiFi setup (no hardcoded credentials)
 *        - SHT30 I2C temperature/humidity sensor
 *        - Automatic NTP time synchronization with DST support
 *        - PIR motion sensor for auto-off/on
 *        - LDR for automatic brightness control
 *        - Clean, well-structured code with proper error handling
 *        - Comprehensive debug output
 *        - Web interface for status and configuration Status
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
 * ==============================================================================================

 * DETAILED CHANGELOG SUMMARY (Co-Pilot)
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
bool brightnessManualOverride = true;  // Manual brightness mode flag
int manualBrightness = 4;               // Manual brightness value (1-15)

// Display Schedule Configuration
bool displayScheduleEnabled = true;    // Enable/disable scheduled on/off times
int scheduleStartHour = 6;              // Turn on at 6:00 AM
int scheduleStartMinute = 0;
int scheduleEndHour = 5;               // Turn off at 5:00 PM
int scheduleEndMinute = 0;

// Timing
unsigned long lastNTPUpdate = 0;
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

bool isWithinScheduleWindow() {
  if (!displayScheduleEnabled) return true;  // Schedule disabled, always within window
  
  // Convert current time to minutes since midnight
  int currentMinutes = hours * 60 + minutes;
  int scheduleStartMinutes = scheduleStartHour * 60 + scheduleStartMinute;
  int scheduleEndMinutes = scheduleEndHour * 60 + scheduleEndMinute;
  
  // Handle case where schedule spans midnight (e.g., 10 PM to 6 AM)
  if (scheduleStartMinutes <= scheduleEndMinutes) {
    // Normal case: start < end (e.g., 6 AM to 11 PM)
    return currentMinutes >= scheduleStartMinutes && currentMinutes < scheduleEndMinutes;
  } else {
    // Schedule spans midnight: start > end (e.g., 11 PM to 6 AM)
    return currentMinutes >= scheduleStartMinutes || currentMinutes < scheduleEndMinutes;
  }
}

void handleBrightnessAndMotion() {
  // Read ambient light
  lightLevel = analogRead(LDR_PIN);
  
  // Map to brightness (inverted: darker = dimmer)
  int ambientBrightness = 15 - map(constrain(lightLevel, 0, 1023), 0, 1023, 1, 15);
  
  // Check motion
  motionDetected = digitalRead(PIR_PIN);
  
  // Check if within schedule window
  bool withinSchedule = isWithinScheduleWindow();
  
  if (withinSchedule && motionDetected) {
    // Motion detected and within schedule - turn on and reset timer
    displayTimer = DISPLAY_TIMEOUT;
    displayOn = true;
    // Use manual brightness if override is enabled, otherwise use ambient
    brightness = brightnessManualOverride ? manualBrightness : ambientBrightness;
    sendCmdAll(CMD_SHUTDOWN, 1);
    sendCmdAll(CMD_INTENSITY, brightness);
  } else if (!withinSchedule) {
    // Outside schedule window - force display off
    if (displayOn) {
      displayOn = false;
      sendCmdAll(CMD_SHUTDOWN, 0);
    }
    displayTimer = 0;
  } else {
    // Within schedule but no motion - countdown
    if (displayTimer > 0) {
      displayTimer--;
      // Fade out gradually (respect manual override if set)
      int targetBrightness = brightnessManualOverride ? manualBrightness : ambientBrightness;
      brightness = map(displayTimer, 0, DISPLAY_TIMEOUT, 1, targetBrightness);
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
    html += "h1{color:#333;}</style>";
    html += "<script>";
    html += "function updateTime() {";
    html += "  fetch('/api/time').then(r => r.json()).then(d => {";
    html += "    document.getElementById('time-display').innerText = d.time;";
    html += "    document.getElementById('date-display').innerText = d.date;";
    html += "  });";
    html += "}";
    html += "function updateStatus() {";
    html += "  fetch('/api/status').then(r => r.json()).then(d => {";
    html += "    document.getElementById('display-status').innerText = d.display;";
    html += "    document.getElementById('motion-status').innerText = d.motion;";
    html += "    document.getElementById('brightness-status').innerText = d.brightness + '/15';";
    html += "    document.getElementById('light-status').innerText = d.light;";
    html += "    document.getElementById('brightness-mode').innerText = d.mode;";
    html += "    let scheduleNotice = document.getElementById('schedule-notice');";
    html += "    if (d.schedule_disabled || d.outside_schedule) {";
    html += "      scheduleNotice.style.display = 'block';";
    html += "      scheduleNotice.innerText = d.outside_schedule ? 'Display OFF: Outside scheduled hours (' + d.schedule_start + '-' + d.schedule_end + ')' : 'Schedule: Disabled';";
    html += "    } else {";
    html += "      scheduleNotice.style.display = 'none';";
    html += "    }";
    html += "  });";
    html += "}";
    html += "setInterval(updateTime, 1000);";
    html += "</script>";
    html += "</head><body>";
    html += "<h1>LED Matrix Clock</h1>";
    html += "<div class='card'><h2>Current Time & Environment</h2>";
    html += "<p style='font-size:24px;' id='time-display'>" + String(hours) + ":" + 
            (minutes < 10 ? "0" : "") + String(minutes) + ":" +
            (seconds < 10 ? "0" : "") + String(seconds) + "</p>";
    html += "<p id='date-display'>" + String(day) + "/" + String(month) + "/" + String(year) + "</p>";
    
    if (sensorAvailable) {
      html += "<p>Temperature: " + String(temperature) + "°C | Humidity: " + String(humidity) + "%</p>";
    }
    html += "</div>";
    
    html += "<div class='card'><h2>Status</h2>";
    html += "<p style='color:red;font-weight:bold;' id='schedule-notice' style='display:none;'></p>";
    html += "<p>Display: <span id='display-status'>" + String(displayOn ? "ON" : "OFF") + "</span></p>";
    html += "<p>Motion: <span id='motion-status'>" + String(motionDetected ? "Detected" : "None") + "</span></p>";
    html += "<p>Current Brightness: <span id='brightness-status'>" + String(brightness) + "/15</span></p>";
    html += "<p>Light Level: <span id='light-status'>" + String(lightLevel) + "</span></p></div>";
    
    html += "<div class='card'><h2>Brightness Control</h2>";
    html += "<p>Mode: <span id='brightness-mode'>" + String(brightnessManualOverride ? "Manual" : "Auto") + "</span></p>";
    if (brightnessManualOverride) {
      html += "<p><label>Manual Brightness: <input type='range' min='1' max='15' value='" + String(manualBrightness) + "' onchange=\"fetch('/brightness?value=' + this.value).then(() => updateStatus())\"></label></p>";
    }
    html += "<p><a href='/brightness?mode=toggle'>Toggle Auto/Manual</a></p></div>";
    
    html += "<div class='card'><h2>Display Schedule</h2>";
    html += "<form method='POST' action='/schedule'>";
    html += "<p><label><input type='checkbox' name='enabled' value='1' " + String(displayScheduleEnabled ? "checked" : "") + "> Enable Schedule</label></p>";
    html += "<p><label>Turn On: <input type='number' name='start_hour' min='0' max='23' value='" + String(scheduleStartHour) + "' style='width:50px;'>:";
    html += "<input type='number' name='start_min' min='0' max='59' value='" + String(scheduleStartMinute < 10 ? "0" : "") + String(scheduleStartMinute) + "' style='width:50px;'></label></p>";
    html += "<p><label>Turn Off: <input type='number' name='end_hour' min='0' max='23' value='" + String(scheduleEndHour) + "' style='width:50px;'>:";
    html += "<input type='number' name='end_min' min='0' max='59' value='" + String(scheduleEndMinute < 10 ? "0" : "") + String(scheduleEndMinute) + "' style='width:50px;'></label></p>";
    html += "<p><button type='submit'>Save Schedule</button></p>";
    html += "</form></div>";
    
    html += "<div class='card'><p><a href='/reset'>Reset WiFi Settings</a></p></div>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
  });
  
  // API endpoint for time data (JSON)
  server.on("/api/time", []() {
    String json = "{\"time\":\"";
    json += String(hours) + ":";
    json += (minutes < 10 ? "0" : "") + String(minutes) + ":";
    json += (seconds < 10 ? "0" : "") + String(seconds);
    json += "\",\"date\":\"";
    json += String(day) + "/" + String(month) + "/" + String(year);
    json += "\"}";
    server.send(200, "application/json", json);
  });
  
  // API endpoint for status data (JSON)
  server.on("/api/status", []() {
    bool withinSchedule = isWithinScheduleWindow();
    String json = "{\"display\":\"";
    json += String(displayOn ? "ON" : "OFF");
    json += "\",\"motion\":\"";
    json += String(motionDetected ? "Detected" : "None");
    json += "\",\"brightness\":";
    json += String(brightness);
    json += ",\"light\":";
    json += String(lightLevel);
    json += ",\"mode\":\"";
    json += String(brightnessManualOverride ? "Manual" : "Auto");
    json += "\",\"schedule_disabled\":";
    json += String(!displayScheduleEnabled ? "true" : "false");
    json += ",\"outside_schedule\":";
    json += String(!withinSchedule ? "true" : "false");
    json += ",\"schedule_start\":\"";
    json += (scheduleStartHour < 10 ? "0" : "") + String(scheduleStartHour) + ":";
    json += (scheduleStartMinute < 10 ? "0" : "") + String(scheduleStartMinute);
    json += "\",\"schedule_end\":\"";
    json += (scheduleEndHour < 10 ? "0" : "") + String(scheduleEndHour) + ":";
    json += (scheduleEndMinute < 10 ? "0" : "") + String(scheduleEndMinute);
    json += "\"}";
    server.send(200, "application/json", json);
  });
  
  // Brightness control endpoint
  server.on("/brightness", []() {
    if (server.hasArg("mode")) {
      // Toggle auto/manual mode
      brightnessManualOverride = !brightnessManualOverride;
      DEBUG(Serial.printf("Brightness mode: %s\n", brightnessManualOverride ? "Manual" : "Auto"));
      // Redirect to home page to refresh and show change
      server.sendHeader("Location", "/");
      server.send(302, "text/plain", "");
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
  
  // Schedule configuration endpoint
  server.on("/schedule", HTTP_POST, []() {
    displayScheduleEnabled = server.hasArg("enabled");
    
    if (server.hasArg("start_hour")) {
      scheduleStartHour = constrain(server.arg("start_hour").toInt(), 0, 23);
    }
    if (server.hasArg("start_min")) {
      scheduleStartMinute = constrain(server.arg("start_min").toInt(), 0, 59);
    }
    if (server.hasArg("end_hour")) {
      scheduleEndHour = constrain(server.arg("end_hour").toInt(), 0, 23);
    }
    if (server.hasArg("end_min")) {
      scheduleEndMinute = constrain(server.arg("end_min").toInt(), 0, 59);
    }
    
    DEBUG(Serial.printf("Schedule updated - Enabled: %s, Start: %02d:%02d, End: %02d:%02d\n",
                        displayScheduleEnabled ? "Yes" : "No", 
                        scheduleStartHour, scheduleStartMinute,
                        scheduleEndHour, scheduleEndMinute));
    
    // Redirect to home page to refresh
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
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