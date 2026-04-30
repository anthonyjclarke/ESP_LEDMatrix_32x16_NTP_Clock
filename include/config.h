#pragma once

// ======================== DISPLAY ========================
#define NUM_MAX    8   // MAX7219 module count
#define LINE_WIDTH 32  // Display width in pixels
#define ROTATE     90  // Rotation: 0, 90, or 270

// ======================== PINS ========================
#define DIN_PIN 15   // MAX7219 data in  (D8)
#define CS_PIN  13   // MAX7219 chip select (D7)
#define CLK_PIN 12   // MAX7219 clock (D6)
#define PIR_PIN D3   // PIR motion sensor
#define LDR_PIN A0   // LDR light sensor (analog)

// ======================== TIMING ========================
#define DISPLAY_TIMEOUT        60      // Seconds before display off with no motion
#define NTP_UPDATE_INTERVAL    600000  // NTP sync interval ms (10 min)
#define MODE_CYCLE_TIME        20000   // Display mode change interval ms (20 s)
#define SENSOR_UPDATE_WITH_NTP true    // Read sensor on each NTP sync

// ======================== BRIGHTNESS ========================
#define LDR_FILTER_WEIGHT          8    // EMA weight; higher = slower response
#define LDR_BRIGHTNESS_HYSTERESIS  35   // ADC delta before accepting new brightness
#define BRIGHTNESS_UPDATE_INTERVAL 500  // Minimum ms between auto brightness changes

// ======================== DISPLAY MANAGEMENT ========================
#define DISPLAY_MANUAL_OVERRIDE_DURATION 300000  // Manual override timeout ms (5 min)
#define STARTUP_GRACE_PERIOD             10000   // ms to keep display on after boot

// ======================== NTP ========================
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov", "time.google.com"

// ======================== TIMEZONE ========================
// Default compile-time timezone. Runtime default comes from timezones[0] (Sydney).
// Requires TZ.h (included in main.cpp before this is used).
// See include/timezones.h or CLAUDE.md for other options.
#define MY_TZ TZ_Australia_Sydney

// ======================== WIFI ========================
#define WIFI_AP_NAME "LED_Clock_Setup"  // Captive portal AP name on first boot

// ======================== OTA ========================
#define OTA_HOSTNAME "led-clock"  // mDNS hostname for OTA updates
#define OTA_PASSWORD "ledclock"   // Change before deploying on an untrusted network
