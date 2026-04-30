#pragma once

// Leveled serial debug output.
// Set DEBUG_LEVEL in platformio.ini build_flags:
//   -DDEBUG_LEVEL=0  silent
//   -DDEBUG_LEVEL=1  errors only
//   -DDEBUG_LEVEL=2  errors + warnings
//   -DDEBUG_LEVEL=3  + info (default)
//   -DDEBUG_LEVEL=4  + verbose (per-loop noise)

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL 3
#endif

// Use do-while(0) wrapper so macro behaves like a statement in all contexts.
// fmt must be a string literal so the prefix can be concatenated at compile time.
#define DBG_ERROR(fmt, ...)   do { if (DEBUG_LEVEL >= 1) Serial.printf("[E] " fmt "\n", ##__VA_ARGS__); } while(0)
#define DBG_WARN(fmt, ...)    do { if (DEBUG_LEVEL >= 2) Serial.printf("[W] " fmt "\n", ##__VA_ARGS__); } while(0)
#define DBG_INFO(fmt, ...)    do { if (DEBUG_LEVEL >= 3) Serial.printf("[I] " fmt "\n", ##__VA_ARGS__); } while(0)
#define DBG_VERBOSE(fmt, ...) do { if (DEBUG_LEVEL >= 4) Serial.printf("[V] " fmt "\n", ##__VA_ARGS__); } while(0)
