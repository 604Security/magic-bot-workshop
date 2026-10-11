// CYD screen test: which screen chip does this Cheap Yellow Display have?
// Shows colour bars with their names. If the names match the colours, the screen setting is right.
// Tap the screen to draw dots (checks the touch panel too).
//
// Flash: make cyd-flash CYD_SKETCH=firmware/cyd/cyd_screen_test CYD_SCREEN=st7789   (or ili9341)
// The screen settings come from SquachWatch-CYD (cyd_user_setup.h / cyd_ili9341_user_setup.h).

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// CYD touch panel: its own SPI bus
#define XPT_IRQ  36
#define XPT_MOSI 32
#define XPT_MISO 39
#define XPT_CLK  25
#define XPT_CS   33

TFT_eSPI tft;
SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touch(XPT_CS, XPT_IRQ);

void bar(int i, uint16_t color, const char* name, uint16_t ink) {
  int w = tft.width() / 5;
  tft.fillRect(i * w, 60, w, 120, color);
  tft.setTextColor(ink, color);
  tft.drawCentreString(name, i * w + w / 2, 112, 2);
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawCentreString("Magic Bot Workshop", tft.width() / 2, 8, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(USER_SETUP_INFO, tft.width() / 2, 38, 2);

  bar(0, TFT_RED, "RED", TFT_WHITE);
  bar(1, TFT_GREEN, "GREEN", TFT_BLACK);
  bar(2, TFT_BLUE, "BLUE", TFT_WHITE);
  bar(3, TFT_YELLOW, "YELLOW", TFT_BLACK);
  bar(4, TFT_WHITE, "WHITE", TFT_BLACK);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("Tap me! Hi from Sparkle's new friend", tft.width() / 2, 200, 2);

  touchSpi.begin(XPT_CLK, XPT_MISO, XPT_MOSI, XPT_CS);
  touch.begin(touchSpi);
  touch.setRotation(1);
  Serial.printf("CYD screen test: %s, %dx%d\n", USER_SETUP_INFO, tft.width(), tft.height());
}

void loop() {
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    // raw touch is roughly 200..3800 on each axis
    int x = map(p.x, 200, 3800, 0, tft.width());
    int y = map(p.y, 200, 3800, 0, tft.height());
    tft.fillCircle(x, y, 4, TFT_MAGENTA);
    Serial.printf("touch raw %d,%d -> %d,%d\n", p.x, p.y, x, y);
  }
  delay(15);
}
