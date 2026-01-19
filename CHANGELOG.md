# Changelog

All notable changes to the ESP8266 LED Matrix Clock project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.8.0] - 2026-01-16

### Added
- Comprehensive CLAUDE.md project documentation file for AI assistant context
- Version numbering system with VERSION, BUILD_DATE, and BUILD_TIME defines
- Enhanced serial banner on bootup displaying version, build date/time, and author
- CHANGELOG.md file following Keep a Changelog format
- TODO notes for future PROGMEM optimization of timezone array (~2KB RAM savings)

### Changed
- Version banner now shows version dynamically from VERSION define
- Updated code comments for improved clarity (timezone access, PROGMEM usage)

### Fixed
- Version banner previously showed v1.0 instead of actual version
- I2C pin assignment comment corrected (SDA/SCL labels were swapped in comment only)

### Documentation
- Documented all compile-time configuration options in CLAUDE.md
- Added comprehensive pin mapping table with GPIO and functional descriptions
- Detailed architecture notes covering event loop, state machine, and memory optimization
- Complete web API documentation with curl examples
- Development notes for building, debugging, and customization

## [2.7.0] - 2025-12-19

### Added
- LED Display Mirror to web interface showing live 32×16 pixel display
- Current timezone name display in "Current Time & Environment" heading
- Enhanced `/api/all` endpoint with `timezone_name` field
- Live canvas rendering with 20× pixel scaling for LED mirror
- 500ms polling for real-time display updates on webpage

### Changed
- Timezone dropdown now auto-updates on selection change (no button press required)
- Timezone name dynamically updates via AJAX when changed
- Removed "Update Timezone" button for cleaner, more intuitive UX
- Refactored project structure: moved fonts.h, max7219.h to include/ directory
- Created new timezones.h header consolidating 88 timezone definitions

### Improved
- Code organization and formatting for better maintainability
- Web interface user experience with instant feedback

## [2.6.0] - 2025-12-17

### Added
- LDR raw reading display in Brightness Control section of web interface
- Real-time JavaScript AJAX updates for LDR reading value

### Changed
- Reordered brightness display to show "LDR Raw Reading: xxxx, calculating Display Brightness to: xx/15"
- Improved user visibility into automatic brightness calculation from ambient light sensor

## [2.5.0] - 2025-12-16

### Changed
- Light indicator behavior now matches LDR response (dark → short bar)
- Reversed gradient on light indicator for intuitive visual feedback
- Digital time display enlarged to 72px for better readability
- Static 🌙/☀️ icons added to light indicator

### Fixed
- Temperature unit labels now render true ° symbol
- Temperature displays append °C/°F to live data
- `/api/status` endpoint exposes short unit code

### Removed
- Raw LDR reading removed from main display (moved to brightness section)

## [2.4.0] - 2025-12-15

### Added
- 88 predefined POSIX timezone strings covering global cities
- Timezone dropdown selection via `/timezone` endpoint
- Automatic NTP re-sync when timezone changes
- Custom timezone string documentation
- Schedule handling enabled by default

### Changed
- Migrated from deprecated `simpleDSTadjust` library to native ESP8266 `TZ.h`
- Timezone system now uses full POSIX timezone strings
- Expanded debug output for timezone operations

### Removed
- `simpleDSTadjust` library dependency

## [2.3.0] - 2025-12-15

### Changed
- Overhauled Status & Configuration card UI
- Reversed light bar gradient to match sensor behavior
- Improved brightness/mode messaging clarity

### Added
- Emoji light level indicator
- Buttonized controls for better UX
- Visual consistency improvements across web interface

## [2.2.0] - 2025-12-15

### Changed
- Reorganized web UI layout: moved light level near time display
- Clarified brightness controls with better labeling
- Applied Orbitron digital font styling to time/date displays

### Added
- Large glowing clock/date treatments for improved visual hierarchy
- Better typography for enhanced readability

## [2.1.0] - 2025-12-15

### Added
- BME280 sensor support (replaces BMP280) for humidity readings
- Humidity and pressure readouts in web interface
- Humidity display on LED matrix (Mode 0)

### Changed
- Updated web interface and API outputs for BME280 data
- Improved sensor data streaming to web UI

### Fixed
- UTF-8 and degree symbol handling in web interface

### Removed
- BMP280-specific sensor files (replaced with BME280)

## [2.0.0] - 2025-12-14

### Added
- AJAX-based web UI with automatic updates (no page refresh)
- `/api/time` endpoint for time data
- `/api/status` endpoint for system status
- `/api/all` endpoint for consolidated data (2s polling)
- Manual brightness control via web interface
- Display scheduling controls
- Configuration validation

### Changed
- Rebuilt entire web UI around AJAX updates
- Merged time and environment data into single live-updating card
- Cleaned display initialization code

### Improved
- Web interface responsiveness
- Real-time data updates without page flicker

## [1.1.0] - 2025-12

### Added
- Initial GitHub release under PlatformIO
- WiFiManager captive portal setup
- Basic web interface for configuration

### Changed
- Migrated from Arduino IDE to PlatformIO build system
- Improved project structure

### Fixed
- Sensor stack stabilization
- Core clock/date functions operational

## [1.0.0] - 2025-10

### Added
- Complete modern rewrite of original @cbm80amiga code
- WiFiManager onboarding for easy WiFi configuration
- BMP280/BME280 temperature and pressure sensing
- NTP time synchronization with automatic DST handling
- PIR motion sensor integration for auto-off functionality
- LDR-based automatic brightness adjustment
- Three rotating display modes (time+temp, large time, time+date)
- 60-second motion detection timeout
- Scheduled display OFF window (22:00-06:00 default)
- Web-based configuration interface
- 12-hour and 24-hour time format support
- Celsius/Fahrenheit temperature switching
- Non-blocking event loop using `millis()` timing
- Display power management state machine
- 88 timezone support with POSIX TZ strings

### Technical Features
- ESP8266 D1 Mini Pro support
- MAX7219 LED matrix driver (8 modules, 32×16 display)
- 90° software rotation for 4×2 module arrangement
- PROGMEM optimization for fonts
- I²C sensor communication
- RESTful web API
- Memory-optimized for ESP8266 80KB SRAM

---

## Version History Summary

- **2.8.x** - Documentation and code quality improvements
- **2.7.x** - Web interface enhancements (LED mirror, timezone UX)
- **2.6.x** - Brightness control visibility improvements
- **2.5.x** - UI polish and temperature display fixes
- **2.4.x** - Modern timezone system migration
- **2.3.x** - Web UI overhaul
- **2.2.x** - Typography and layout improvements
- **2.1.x** - BME280 sensor integration
- **2.0.x** - AJAX web interface rebuild
- **1.x** - Initial modern rewrite and GitHub release

---

[2.8.0]: https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock/compare/v2.7.0...v2.8.0
[2.7.0]: https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock/compare/v2.6.0...v2.7.0
[2.6.0]: https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock/compare/v2.5.0...v2.6.0
[2.5.0]: https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock/compare/v2.4.0...v2.5.0
[2.4.0]: https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock/compare/v2.3.0...v2.4.0
[2.3.0]: https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock/compare/v2.2.0...v2.3.0
[2.2.0]: https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock/compare/v2.1.0...v2.2.0
[2.1.0]: https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock/compare/v2.0.0...v2.1.0
[2.0.0]: https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock/compare/v1.1.0...v2.0.0
[1.1.0]: https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/anthonyjclarke/ESP_LEDMatrix_32x16_NTP_Clock/releases/tag/v1.0.0
