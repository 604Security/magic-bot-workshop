// Spy Camera: the yellow touchscreen (CYD) asks Sparkle for photos over Wi-Fi and shows them, like a spy TV.
// Tap the screen to freeze the picture ("SNAP!"); tap again to go back to live.
//
// Flash: make cyd-flash CYD_SKETCH=firmware/cyd/cyd_spycam
// Needs secrets.h (same home Wi-Fi as Sparkle) and the TJpg_Decoder library.

#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <XPT2046_Touchscreen.h>
#include "secrets.h"

#define SPARKLE_HOST "sparkle"               // finds Sparkle at sparkle.local
#define SPARKLE_IP_FALLBACK "192.168.86.189"  // used if sparkle.local can't be found

// CYD touch panel: its own SPI bus
#define XPT_IRQ  36
#define XPT_MOSI 32
#define XPT_MISO 39
#define XPT_CLK  25
#define XPT_CS   33

TFT_eSPI tft;
SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touch(XPT_CS, XPT_IRQ);
String sparkleUrl;
bool frozen = false;
int photos = 0;

// TJpg_Decoder hands us the picture in small blocks; draw each one on the screen
bool drawBlock(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return false;
  tft.pushImage(x, y, w, h, bitmap);
  return true;
}

void banner(const char* text, uint16_t color) {
  tft.fillRect(0, 0, tft.width(), 22, TFT_BLACK);
  tft.setTextColor(color, TFT_BLACK);
  tft.drawCentreString(text, tft.width() / 2, 3, 2);
}

void findSparkle() {
  banner("Looking for Sparkle...", TFT_YELLOW);
  MDNS.begin("sparkle-friend");
  IPAddress ip = MDNS.queryHost(SPARKLE_HOST, 3000);
  String host = ip == IPAddress(0, 0, 0, 0) ? String(SPARKLE_IP_FALLBACK) : ip.toString();
  sparkleUrl = "http://" + host + "/capture";
  Serial.println("Sparkle camera: " + sparkleUrl);
}

// Ask Sparkle for one photo and show it. Returns false if Sparkle didn't answer.
bool grabPhoto() {
  HTTPClient http;
  http.setTimeout(4000);
  if (!http.begin(sparkleUrl)) return false;
  int code = http.GET();
  if (code != 200) {
    http.end();
    return false;
  }
  int len = http.getSize();
  if (len <= 0 || len > 120000) {
    http.end();
    return false;
  }
  uint8_t* jpg = (uint8_t*)malloc(len);
  if (!jpg) {
    http.end();
    return false;
  }
  WiFiClient* stream = http.getStreamPtr();
  int got = 0;
  unsigned long start = millis();
  while (got < len && millis() - start < 4000) {
    int n = stream->available();
    if (n > 0) got += stream->readBytes(jpg + got, min(n, len - got));
    else delay(1);
  }
  http.end();
  if (got == len) {
    TJpgDec.drawJpg(0, 0, jpg, len);  // 640x480 photo, shrunk by half = 320x240, fills the screen
    photos++;
  }
  free(jpg);
  return got == len;
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawCentreString("Spy Camera", tft.width() / 2, 90, 4);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("Sparkle's eye, on the big screen", tft.width() / 2, 130, 2);

  TJpgDec.setJpgScale(2);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(drawBlock);

  touchSpi.begin(XPT_CLK, XPT_MISO, XPT_MOSI, XPT_CS);
  touch.begin(touchSpi);

  banner("Joining Wi-Fi...", TFT_YELLOW);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(200);
  findSparkle();
}

void loop() {
  if (touch.touched()) {  // tap: freeze / unfreeze
    frozen = !frozen;
    if (frozen) {
      tft.fillScreen(TFT_WHITE);  // camera flash!
      delay(80);
      grabPhoto();
      banner("SNAP! Tap to go live", TFT_MAGENTA);
    }
    while (touch.touched()) delay(20);
  }
  if (frozen) return;
  if (!grabPhoto()) {
    banner("Can't see Sparkle... is it on?", TFT_RED);
    delay(1000);
    findSparkle();
  }
}
