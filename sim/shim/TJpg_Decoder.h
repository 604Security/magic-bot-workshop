// Pretend JPEG decoder: instead of a real photo it draws a "camera picture" placeholder in blocks,
// through the same callback the sketch gives the real decoder.
#pragma once
#include <Arduino.h>
typedef bool (*SketchCallback)(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
struct TJpgShim {
  SketchCallback cb = nullptr;
  bool swap = false;
  void setJpgScale(int) {}
  void setSwapBytes(bool s) { swap = s; }
  void setCallback(SketchCallback c) { cb = c; }
  int drawJpg(int x0, int y0, const uint8_t*, uint32_t) {
    static uint16_t block[16 * 16];
    for (int by = 0; by < 240; by += 16)
      for (int bx = 0; bx < 320; bx += 16) {
        for (int i = 0; i < 256; i++) {
          int x = bx + i % 16, y = by + i / 16;
          int r = 60 + x / 4, g = 90 + y / 3, b = 140 + (x + y) / 8;
          bool sun = (x - 230) * (x - 230) + (y - 70) * (y - 70) < 900;
          if (sun) { r = 255; g = 220; b = 90; }
          if (y > 170) { r = 70; g = 140 + (x % 20 < 10) * 10; b = 70; }
          uint16_t v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
          block[i] = swap ? (uint16_t)(v >> 8 | v << 8) : v;  // the real decoder swaps for the screen
        }
        if (cb && !cb(x0 + bx, y0 + by, 16, 16, block)) return 0;
      }
    return 1;
  }
};
extern TJpgShim TJpgDec;
