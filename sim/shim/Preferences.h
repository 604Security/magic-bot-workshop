// Pretend saved settings (forgotten when the emulator stops).
#pragma once
#include <Arduino.h>
struct Preferences {
  bool begin(const char*, bool) { return true; }
  void end() {}
  uint16_t getUShort(const char*, uint16_t d) { return d; }
  size_t getBytes(const char*, void*, size_t) { return 0; }
  void putUShort(const char*, uint16_t) {}
  void putBytes(const char*, const void*, size_t) {}
};
