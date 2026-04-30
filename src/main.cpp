// Version Information
#define VERSION "2.9.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

/*
 * ESP8266 LED Matrix Clock - Modern Edition
 * Version: 2.9.0
 * Author: Rewritten from original by Anthony Clarke
 * Acknowledgements: Original by @cbm80amiga here - https://www.youtube.com/watch?v=2wJOdi0xzas&t=32s
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

#include <ArduinoOTA.h>
#include "config.h"   // user-tuneable constants — must come before max7219.h
#include "debug.h"
#include "max7219.h"
#include "fonts.h"
#include "timezones.h"

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
int filteredLightLevel = 512;     // Smoothed LDR value used for auto brightness
int previousLightLevel = 512;    // Track previous light level for change detection
bool lightLevelChanged = false;  // Flag to trigger webpage refresh on significant light change
bool ldrFilterInitialized = false;
bool autoBrightnessInitialized = false;
int autoBrightnessLevel = 8;
int autoBrightnessLdrReference = 512;
unsigned long lastAutoBrightnessChange = 0;
bool displayHardwareStateInitialized = false;
bool lastHardwareDisplayOn = false;
int lastHardwareIntensity = -1;
int displayTimer = DISPLAY_TIMEOUT;
bool displayOn = true;
bool motionDetected = false;
bool brightnessManualOverride = false;  // Manual brightness mode flag
int manualBrightness = 4;               // Manual brightness value (1-15)
bool displayManualOverride = false;     // Manual display on/off override flag
unsigned long displayManualOverrideTimeout = 0;

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

// Timing
unsigned long lastNTPUpdate = 0;
unsigned long startupTime = 0;
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
int updateAmbientLightReading();
int computeAmbientBrightnessFromLdr(int ldrValue);
int computeStableAmbientBrightnessFromLdr(int filteredLdrValue);
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
  DBG_INFO("Initializing LED matrix");
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 5);

  // Initialize I2C and BME280
  DBG_INFO("Initializing I2C and BME280 sensor");
  Wire.begin();
  delay(100);
  testSensor();

  // Initialize PIR
  pinMode(PIR_PIN, INPUT);
  DBG_INFO("PIR sensor initialized");

  // WiFiManager setup
  showMessage("WIFI...");
  DBG_INFO("Starting WiFi Manager");
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect(WIFI_AP_NAME)) {
    DBG_ERROR("WiFi failed to connect, restarting");
    showMessage("WIFI FAIL");
    delay(3000);
    ESP.restart();
  }

  DBG_INFO("WiFi connected: %s", WiFi.localIP().toString().c_str());
  showMessage(WiFi.localIP().toString().c_str());
  delay(2000);

  // OTA setup
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    DBG_INFO("OTA update starting");
    sendCmdAll(CMD_SHUTDOWN, 0);
  });
  ArduinoOTA.onEnd([]() {
    DBG_INFO("OTA complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DBG_VERBOSE("OTA: %u%%", progress * 100 / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DBG_ERROR("OTA error [%u]", error);
  });
  ArduinoOTA.begin();
  DBG_INFO("OTA ready: hostname=%s", OTA_HOSTNAME);

  // NTP sync
  showMessage("SYNC TIME");
  if (syncNTP()) {
    DBG_INFO("Time synchronized");
  } else {
    DBG_WARN("Time sync failed, will retry");
  }

  updateSensorData();

  setupWebServer();
  server.begin();
  DBG_INFO("Web server started");

  showMessage("READY!");
  delay(1000);

  DBG_INFO("Setup complete");
}

// ======================== MAIN LOOP ========================

void loop()
{
  unsigned long currentMillis = millis();
  
  // Handle web server and OTA requests
  server.handleClient();
  ArduinoOTA.handle();

  // Periodic NTP sync
  if (currentMillis - lastNTPUpdate >= NTP_UPDATE_INTERVAL) {
    lastNTPUpdate = currentMillis;
    DBG_INFO("Periodic update");
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
    DBG_VERBOSE("Display mode: %d", currentMode);
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
  
  // Status output (throttled, gated by DBG_INFO inside printStatus)
  static unsigned long lastDebug = 0;
  if (currentMillis - lastDebug > 2000) {
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
  DBG_INFO("Syncing NTP");

  const char* tzString = timezones[currentTimezone].tzString;
  configTime(tzString, NTP_SERVERS);

  int attempts = 0;
  while (!time(nullptr) && attempts < 20) {
    delay(500);
    attempts++;
  }

  if (attempts >= 20) {
    DBG_WARN("NTP sync failed");
    return false;
  }

  updateTime();
  DBG_INFO("Time synced: %02d:%02d:%02d (TZ: %s)", hours, minutes, seconds, timezones[currentTimezone].name);
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
  DBG_INFO("Testing BME280 sensor");

  if (!bme280.begin(0x76)) {
    sensorAvailable = false;
    DBG_ERROR("BME280 not found - check SDA->D2, SCL->D1, VCC->3.3V");
    return;
  }

  bme280.setSampling(Adafruit_BME280::MODE_NORMAL,
                     Adafruit_BME280::SAMPLING_X2,
                     Adafruit_BME280::SAMPLING_X16,
                     Adafruit_BME280::SAMPLING_X1,
                     Adafruit_BME280::FILTER_X16,
                     Adafruit_BME280::STANDBY_MS_500);

  temperature = (int)bme280.readTemperature();
  pressure = (int)bme280.readPressure() / 100;
  humidity = (int)bme280.readHumidity();

  if (temperature < -40 || temperature > 85) {
    sensorAvailable = false;
    DBG_ERROR("BME280 read validation failed");
  } else {
    sensorAvailable = true;
    DBG_INFO("BME280 OK: %dC, %d%% RH, %d hPa", temperature, humidity, pressure);
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
    DBG_WARN("Sensor read failed");
  } else {
    sensorAvailable = true;
    DBG_VERBOSE("Sensor: %dC, %d%% RH, %d hPa", temperature, humidity, pressure);
  }
}

// ======================== BRIGHTNESS & MOTION ========================

int updateAmbientLightReading() {
  lightLevel = analogRead(LDR_PIN);

  if (!ldrFilterInitialized) {
    filteredLightLevel = lightLevel;
    ldrFilterInitialized = true;
  } else {
    filteredLightLevel = ((filteredLightLevel * (LDR_FILTER_WEIGHT - 1)) + lightLevel) / LDR_FILTER_WEIGHT;
  }

  return filteredLightLevel;
}

int computeAmbientBrightnessFromLdr(int ldrValue) {
  // Current divider reads higher ADC values in darker rooms; keep display brightness lower there.
  return map(constrain(ldrValue, 0, 1023), 0, 1023, 15, 0);
}

int computeStableAmbientBrightnessFromLdr(int filteredLdrValue) {
  int candidateBrightness = computeAmbientBrightnessFromLdr(filteredLdrValue);

  if (!autoBrightnessInitialized) {
    autoBrightnessLevel = candidateBrightness;
    autoBrightnessLdrReference = filteredLdrValue;
    lastAutoBrightnessChange = millis();
    autoBrightnessInitialized = true;
    return autoBrightnessLevel;
  }

  if (candidateBrightness != autoBrightnessLevel) {
    int ldrDifference = abs(filteredLdrValue - autoBrightnessLdrReference);
    unsigned long now = millis();
    if (ldrDifference >= LDR_BRIGHTNESS_HYSTERESIS &&
        now - lastAutoBrightnessChange >= BRIGHTNESS_UPDATE_INTERVAL) {
      autoBrightnessLevel = candidateBrightness;
      autoBrightnessLdrReference = filteredLdrValue;
      lastAutoBrightnessChange = now;
    }
  }

  return autoBrightnessLevel;
}

void applyDisplayHardwareState(bool on, int intensity) {
  // Intensity is only meaningful when ON. Clamp defensively.
  int clamped = constrain(intensity, 0, 15);

  bool displayStateChanged = !displayHardwareStateInitialized || lastHardwareDisplayOn != on;
  bool intensityChanged = !displayHardwareStateInitialized || lastHardwareIntensity != clamped;

  if (displayStateChanged) {
    sendCmdAll(CMD_SHUTDOWN, on ? 1 : 0);
  }
  if (on && (displayStateChanged || intensityChanged)) {
    sendCmdAll(CMD_INTENSITY, clamped);
  }

  displayHardwareStateInitialized = true;
  lastHardwareDisplayOn = on;
  if (on) {
    lastHardwareIntensity = clamped;
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

    int filteredLdr = updateAmbientLightReading();
    int ambientBrightness = computeStableAmbientBrightnessFromLdr(filteredLdr);
    brightness = brightnessManualOverride ? manualBrightness : ambientBrightness;
    applyDisplayHardwareState(true, brightness);
    return;
  }
  
  // Read ambient light
  int filteredLdr = updateAmbientLightReading();
  
  // Check if light level has changed by ±5%
  int lightDifference = abs(lightLevel - previousLightLevel);
  int changeThreshold = (previousLightLevel > 0) ? (previousLightLevel * 5 / 100) : 51;  // 5% of previous level, minimum 51
  if (lightDifference >= changeThreshold) {
    lightLevelChanged = true;
    DBG_VERBOSE("Light level: %d->%d (diff=%d)", previousLightLevel, lightLevel, lightDifference);
  }
  previousLightLevel = lightLevel;  // Always update for next comparison
  
  // Map smoothed LDR readings to brightness with hysteresis to avoid flicker.
  int ambientBrightness = computeStableAmbientBrightnessFromLdr(filteredLdr);
  
  // Check motion
  motionDetected = digitalRead(PIR_PIN);
  
  // Check if we are inside the scheduled OFF window
  bool withinOffWindow = isWithinScheduleOffWindow();
  
  // Check if manual override timeout has expired
  if (displayManualOverride && millis() > displayManualOverrideTimeout) {
    displayManualOverride = false;
    DBG_INFO("Display manual override expired");
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
      DBG_INFO("Display OFF by schedule (%02d:%02d-%02d:%02d)",
               scheduleOffStartHour, scheduleOffStartMinute,
               scheduleOffEndHour, scheduleOffEndMinute);
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
      DBG_INFO("Brightness mode: %s", brightnessManualOverride ? "Manual" : "Auto");
      server.send(200, "text/plain", "OK");
      return;
    }
    if (server.hasArg("value")) {
      // Set manual brightness
      int newBrightness = server.arg("value").toInt();
      manualBrightness = constrain(newBrightness, 1, 15);
      brightness = manualBrightness;
      if (displayOn) {
        applyDisplayHardwareState(true, brightness);
      }
      DBG_INFO("Manual brightness: %d", manualBrightness);
    }
    server.send(200, "text/plain", "OK");
  });
  
  // Time format toggle endpoint
  server.on("/timeformat", []() {
    if (server.hasArg("mode")) {
      use24HourFormat = !use24HourFormat;
      DBG_INFO("Time format: %s", use24HourFormat ? "24-hour" : "12-hour");

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
      DBG_INFO("Temp unit: %s", useFahrenheit ? "Fahrenheit" : "Celsius");

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
        DBG_INFO("Timezone: %s", timezones[currentTimezone].name);
        
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
      int filteredLdr = updateAmbientLightReading();

      if (displayOn) {
        int ambientBrightness = computeStableAmbientBrightnessFromLdr(filteredLdr);
        brightness = brightnessManualOverride ? manualBrightness : ambientBrightness;
        applyDisplayHardwareState(true, brightness);
        DBG_INFO("Display ON (manual, 5 min)");
      } else {
        applyDisplayHardwareState(false, 0);
        DBG_INFO("Display OFF (manual, 5 min)");
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
    
    DBG_INFO("Schedule: %s, %02d:%02d-%02d:%02d",
             scheduleOffEnabled ? "ON" : "OFF",
             scheduleOffStartHour, scheduleOffStartMinute,
             scheduleOffEndHour, scheduleOffEndMinute);
    
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
  DBG_INFO("WiFi config mode - connect to AP: %s", WIFI_AP_NAME);
  DBG_INFO("Config portal IP: %s", WiFi.softAPIP().toString().c_str());
  showMessage("SETUP AP");
  delay(2000);
  showMessage("LED CLOCK");
}

void printBanner() {
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║   ESP8266 LED Matrix Clock             ║");
  Serial.println("║   PlatformIO Edition                   ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  Serial.printf("Version: %s\n", VERSION);
  Serial.printf("Build Date: %s %s\n", BUILD_DATE, BUILD_TIME);
  Serial.printf("Author: Anthony Clarke\n");
  Serial.println();
}

void printStatus() {
  if (use24HourFormat) {
    DBG_INFO("Time: %02d:%02d:%02d | Date: %02d/%02d/%d",
             hours24, minutes, seconds, day, month, year);
  } else {
    DBG_INFO("Time: %02d:%02d:%02d %s | Date: %02d/%02d/%d",
             hours, minutes, seconds, (hours24 < 12) ? "AM" : "PM", day, month, year);
  }
  if (sensorAvailable) {
    DBG_INFO("Sensor: %dC, %d%% RH", temperature, humidity);
  } else {
    DBG_INFO("Sensor not available");
  }
  DBG_INFO("Light: %d | Bright: %d", lightLevel, brightness);
  bool withinOffWindow = isWithinScheduleOffWindow();
  const char* schedStat = !scheduleOffEnabled ? "DISABLED" : (withinOffWindow ? "ACTIVE-OFF" : "ACTIVE");
  DBG_INFO("Motion: %s | Display: %s | Timer: %d | Sched: %s (%02d:%02d-%02d:%02d)",
           motionDetected ? "YES" : "NO", displayOn ? "ON" : "OFF", displayTimer,
           schedStat, scheduleOffStartHour, scheduleOffStartMinute,
           scheduleOffEndHour, scheduleOffEndMinute);
}

// ======================== END OF CODE ========================
