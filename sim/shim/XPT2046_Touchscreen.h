// Pretend touch: the emulator "taps" by setting g_tap; reading the point lifts the finger.
#pragma once
#include <SPI.h>
struct TS_Point { int16_t x, y, z; };
struct SimTap { bool down = false; int x = 0, y = 0; };
extern SimTap g_tap;
struct XPT2046_Touchscreen {
  XPT2046_Touchscreen(int, int) {}
  void begin(SPIClass&) {}
  void setRotation(int) {}
  bool touched() { return g_tap.down; }
  TS_Point getPoint() {
    g_tap.down = false;
    // the sketch maps raw 200..3800 to the screen, so go the other way
    return TS_Point{(int16_t)(200 + g_tap.x * 3600 / 320), (int16_t)(200 + g_tap.y * 3600 / 240), 500};
  }
};
