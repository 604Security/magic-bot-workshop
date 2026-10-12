// Pretend TFT_eSPI: draws into a 320x240 picture in memory instead of a real screen.
// Text uses TFT_eSPI's own font tables (Font16 = font 2, Font32rle = font 4) from the installed library,
// decoded the same way the real drawChar() does. pushImage() copies the real byte-order rule: without
// setSwapBytes(true), 16-bit colours arrive byte-swapped, exactly like on the device.
#pragma once
#include <Arduino.h>
#include <Fonts/Font16.h>
#include <Fonts/Font32rle.h>

#define TFT_BLACK   0x0000
#define TFT_WHITE   0xFFFF
#define TFT_RED     0xF800
#define TFT_GREEN   0x07E0
#define TFT_BLUE    0x001F
#define TFT_YELLOW  0xFFE0
#define TFT_MAGENTA 0xF81F
#define TFT_CYAN    0x07FF

class TFT_eSPI {
 public:
  static const int W = 320, H = 240;
  uint16_t fb[W * H] = {0};
  uint16_t textcolor = 0xFFFF, textbg = 0xFFFF;
  int textsize = 1;
  bool swapBytes = false;

  void init() {}
  void setRotation(int) {}
  int width() const { return W; }
  int height() const { return H; }
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
  void setSwapBytes(bool s) { swapBytes = s; }

  // ---- shapes
  void drawPixel(int x, int y, uint16_t c) { if (x >= 0 && x < W && y >= 0 && y < H) fb[y * W + x] = c; }
  void fillRect(int x, int y, int w, int h, uint16_t c) {
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) drawPixel(i, j, c);
  }
  void fillScreen(uint16_t c) { fillRect(0, 0, W, H, c); }
  void drawFastHLine(int x, int y, int w, uint16_t c) { fillRect(x, y, w, 1, c); }
  void drawFastVLine(int x, int y, int h, uint16_t c) { fillRect(x, y, 1, h, c); }
  void drawRect(int x, int y, int w, int h, uint16_t c) {
    drawFastHLine(x, y, w, c); drawFastHLine(x, y + h - 1, w, c);
    drawFastVLine(x, y, h, c); drawFastVLine(x + w - 1, y, h, c);
  }
  void fillCircle(int cx, int cy, int r, uint16_t c) {
    for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++) if (x * x + y * y <= r * r + r) drawPixel(cx + x, cy + y, c);
  }
  void drawCircle(int cx, int cy, int r, uint16_t c) {
    for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++) {
      int d = x * x + y * y;
      if (d <= r * r + r && d >= (r - 1) * (r - 1) + (r - 1)) drawPixel(cx + x, cy + y, c);
    }
  }
  void fillEllipse(int cx, int cy, int rx, int ry, uint16_t c) {
    for (int y = -ry; y <= ry; y++) for (int x = -rx; x <= rx; x++)
      if ((float)x * x / (rx * rx) + (float)y * y / (ry * ry) <= 1.0f) drawPixel(cx + x, cy + y, c);
  }
  void fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
    fillRect(x + r, y, w - 2 * r, h, c);
    fillRect(x, y + r, w, h - 2 * r, c);
    fillCircle(x + r, y + r, r, c); fillCircle(x + w - r - 1, y + r, r, c);
    fillCircle(x + r, y + h - r - 1, r, c); fillCircle(x + w - r - 1, y + h - r - 1, r, c);
  }
  void drawRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
    drawFastHLine(x + r, y, w - 2 * r, c); drawFastHLine(x + r, y + h - 1, w - 2 * r, c);
    drawFastVLine(x, y + r, h - 2 * r, c); drawFastVLine(x + w - 1, y + r, h - 2 * r, c);
    corner(x + r, y + r, r, -1, -1, c); corner(x + w - r - 1, y + r, r, 1, -1, c);
    corner(x + r, y + h - r - 1, r, -1, 1, c); corner(x + w - r - 1, y + h - r - 1, r, 1, 1, c);
  }
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) {
    int minx = std::min({x0, x1, x2}), maxx = std::max({x0, x1, x2}), miny = std::min({y0, y1, y2}), maxy = std::max({y0, y1, y2});
    auto e = [](int ax, int ay, int bx, int by, int px, int py) { return (long)(bx - ax) * (py - ay) - (long)(by - ay) * (px - ax); };
    long area = e(x0, y0, x1, y1, x2, y2);
    if (!area) return;
    for (int y = miny; y <= maxy; y++) for (int x = minx; x <= maxx; x++) {
      long a = e(x1, y1, x2, y2, x, y), b = e(x2, y2, x0, y0, x, y), d = e(x0, y0, x1, y1, x, y);
      if ((area > 0 && a >= 0 && b >= 0 && d >= 0) || (area < 0 && a <= 0 && b <= 0 && d <= 0)) drawPixel(x, y, c);
    }
  }

  // ---- pictures (with the device's byte-order behaviour)
  void pushImage(int x, int y, int w, int h, const uint16_t* data) { pushImage(x, y, w, h, data, 0, false); }
  void pushImage(int x, int y, int w, int h, const uint16_t* data, uint16_t transp) { pushImage(x, y, w, h, data, transp, true); }
  void pushImage(int x, int y, int w, int h, uint16_t* data) { pushImage(x, y, w, h, (const uint16_t*)data, 0, false); }

  // ---- text (fonts 2 and 4, like TFT_eSPI)
  void setTextColor(uint16_t c) { textcolor = textbg = c; }
  void setTextColor(uint16_t c, uint16_t bg) { textcolor = c; textbg = bg; }
  void setTextSize(int s) { textsize = s < 1 ? 1 : s; }
  int fontHeight(int font) { return (font == 4 ? chr_hgt_f32 : font == 2 ? chr_hgt_f16 : 8) * textsize; }
  int textWidth(const char* s, int font) {
    int w = 0;
    for (; *s; s++) w += charWidth(*s, font);
    return w;
  }
  int textWidth(const String& s, int font) { return textWidth(s.c_str(), font); }
  int drawString(const char* s, int x, int y, int font) {
    int x0 = x;
    for (; *s; s++) x += drawChar(*s, x, y, font);
    return x - x0;
  }
  int drawString(const String& s, int x, int y, int font) { return drawString(s.c_str(), x, y, font); }
  int drawCentreString(const char* s, int x, int y, int font) { return drawString(s, x - textWidth(s, font) / 2, y, font); }
  int drawCentreString(const String& s, int x, int y, int font) { return drawCentreString(s.c_str(), x, y, font); }
  int drawRightString(const char* s, int x, int y, int font) { return drawString(s, x - textWidth(s, font), y, font); }
  int drawRightString(const String& s, int x, int y, int font) { return drawRightString(s.c_str(), x, y, font); }

  // set by the emulator: is font 4 really loaded on the device? (checks the screen settings file)
#ifdef LOAD_FONT4
  static const bool font4Loaded = true;
#else
  static const bool font4Loaded = false;
#endif

 private:
  void corner(int cx, int cy, int r, int sx, int sy, uint16_t c) {
    for (int y = 0; y <= r; y++) for (int x = 0; x <= r; x++) {
      int d = x * x + y * y;
      if (d <= r * r + r && d >= (r - 1) * (r - 1) + (r - 1)) drawPixel(cx + sx * x, cy + sy * y, c);
    }
  }
  void pushImage(int x, int y, int w, int h, const uint16_t* data, uint16_t transp, bool useTransp) {
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
      uint16_t v = data[j * w + i];
      if (useTransp && v == transp) continue;
      // on the ESP32 the raw bytes go out low byte first unless swapBytes is on
      drawPixel(x + i, y + j, swapBytes ? v : (uint16_t)(v >> 8 | v << 8));
    }
  }
  int charWidth(char ch, int font) {
    if (ch < 32 || ch > 127) return 0;
    if (font == 2) return widtbl_f16[ch - 32] * textsize;
    if (font == 4) return font4Loaded ? widtbl_f32[ch - 32] * textsize : 0;
    return 6 * textsize;
  }
  void dot(int x, int y) { fillRect(x, y, textsize, textsize, textcolor); }
  int drawChar(char ch, int x, int y, int font) {
    if (ch < 32 || ch > 127) return 0;
    int u = ch - 32;
    if (font == 2) {
      int width = widtbl_f16[u], bytes = (width + 6) / 8;
      const uint8_t* p = chrtbl_f16[u];
      if (textcolor != textbg) fillRect(x, y, width * textsize, chr_hgt_f16 * textsize, textbg);
      for (int i = 0; i < chr_hgt_f16; i++)
        for (int k = 0; k < bytes; k++) {
          uint8_t line = p[bytes * i + k];
          for (int b = 0; b < 8; b++) if (line & (0x80 >> b)) dot(x + (k * 8 + b) * textsize, y + i * textsize);
        }
      return width * textsize;
    }
    if (font == 4) {
      if (!font4Loaded) return 0;  // exactly like the device: an unloaded font draws nothing
      int width = widtbl_f32[u], total = width * chr_hgt_f32, pc = 0;
      const uint8_t* p = chrtbl_f32[u];
      if (textcolor != textbg) fillRect(x, y, width * textsize, chr_hgt_f32 * textsize, textbg);
      while (pc < total) {
        uint8_t line = *p++;
        bool on = line & 0x80;
        int run = (line & 0x7F) + 1;
        for (int r = 0; r < run && pc < total; r++, pc++)
          if (on) dot(x + (pc % width) * textsize, y + (pc / width) * textsize);
      }
      return width * textsize;
    }
    return 6 * textsize;
  }
};
