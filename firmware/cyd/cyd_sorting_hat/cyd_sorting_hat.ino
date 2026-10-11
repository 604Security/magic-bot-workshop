// Colour Sorting Hat (Megaspy picked 2 on pickers/06-sparkle-friend.html)
// Shake Sparkle (or tap this screen) and the Sorting Hat thinks... then this screen shows the house in full colour,
// with a crest, sparkles, a ta-da sound and the RGB light glowing the house colour.
// It asks Sparkle's /api/hat what's happening, a few times a second.
//
// Flash: make cyd-flash CYD_SKETCH=firmware/cyd/cyd_sorting_hat

#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "secrets.h"

#define SPARKLE_HOST "sparkle"
#define SPARKLE_IP_FALLBACK "192.168.86.189"

#define XPT_IRQ  36
#define XPT_MOSI 32
#define XPT_MISO 39
#define XPT_CLK  25
#define XPT_CS   33
#define LED_R 4    // RGB light (on when LOW)
#define LED_G 16
#define LED_B 17
#define SPEAKER 26

TFT_eSPI tft;
SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touch(XPT_CS, XPT_IRQ);
String sparkle;

enum { GRYFFINDOR, RAVENCLAW, HUFFLEPUFF, SLYTHERIN };
const char* NAMES[4] = {"GRYFFINDOR!", "RAVENCLAW!", "HUFFLEPUFF!", "SLYTHERIN!"};
const char* ANIMALS[4] = {"Brave like a lion", "Clever like an eagle", "Kind like a badger", "Sly like a snake"};
const char LETTERS[4] = {'G', 'R', 'H', 'S'};

// ---- light and sound
void glow(bool r, bool g, bool b) {
  digitalWrite(LED_R, !r);
  digitalWrite(LED_G, !g);
  digitalWrite(LED_B, !b);
}

void note(int freq, int ms) {
  ledcWriteTone(0, freq);
  delay(ms);
  ledcWriteTone(0, 0);
  delay(30);
}

void fanfare() {
  note(523, 150);  // C
  note(659, 150);  // E
  note(784, 150);  // G
  note(1047, 450); // high C!
}

// ---- talking to Sparkle
String ask(const String& path) {
  HTTPClient http;
  http.setTimeout(2000);
  if (!http.begin(sparkle + path)) return "";
  String body = http.GET() == 200 ? http.getString() : "";
  http.end();
  return body;
}

int jsonInt(const String& j, const char* key) {
  int i = j.indexOf(String("\"") + key + "\":");
  return i < 0 ? -1 : j.substring(i + strlen(key) + 3).toInt();
}

// ---- drawing
uint16_t rgb(uint32_t c) { return tft.color565(c >> 16, (c >> 8) & 255, c & 255); }
const uint32_t MAIN[4] = {0xAE0001, 0x1E3A8A, 0xF2C230, 0x1F6B3A};
const uint32_t TRIM[4] = {0xFFC93C, 0xC9A66B, 0x222222, 0xC0C0C0};

void drawHat(int dx, bool talking) {
  int cx = 160 + dx;
  uint16_t brown = rgb(0x6B4423), dark = rgb(0x3A2410);
  tft.fillTriangle(cx - 55, 160, cx + 55, 160, cx + 20, 40, brown);  // the pointy hat (bent tip)
  tft.fillTriangle(cx + 20, 40, cx + 32, 66, cx + 50, 52, brown);
  tft.fillRoundRect(cx - 85, 156, 170, 18, 9, brown);                // brim
  tft.fillEllipse(cx - 18, 118, 7, 4, dark);                         // sleepy eyes
  tft.fillEllipse(cx + 18, 118, 7, 4, dark);
  if (talking) tft.fillEllipse(cx, 140, 16, 7, dark);                // mouth open
  else tft.fillRect(cx - 18, 140, 36, 4, dark);
}

void waitingScreen() {
  tft.fillScreen(rgb(0x2A1A4A));
  drawHat(0, false);
  tft.setTextColor(TFT_YELLOW);
  tft.drawCentreString("Shake Sparkle!", 160, 186, 4);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("(or tap me)", 160, 214, 2);
  glow(0, 0, 0);
}

void thinkingFrame(unsigned long t) {
  static const char* WORDS[4] = {"Hmm...", "Tricky...", "VERY tricky...", "Hmm..."};
  int dx = (t / 200) % 2 ? 6 : -6;
  tft.fillRect(0, 30, 320, 150, rgb(0x2A1A4A));
  drawHat(dx, (t / 150) % 2);
  tft.fillRect(0, 180, 320, 60, rgb(0x2A1A4A));
  tft.setTextColor(TFT_YELLOW);
  tft.drawCentreString(WORDS[(t / 900) % 4], 160, 196, 4);
  glow((t / 300) % 3 == 0, (t / 300) % 3 == 1, (t / 300) % 3 == 2);  // the light swirls while it thinks
}

void houseScreen(int h) {
  uint16_t main = rgb(MAIN[h]), trim = rgb(TRIM[h]);
  tft.fillScreen(main);
  // the crest: a shield with the house letter
  tft.fillRoundRect(110, 18, 100, 80, 10, trim);
  tft.fillTriangle(110, 90, 210, 90, 160, 140, trim);
  tft.fillRoundRect(118, 26, 84, 70, 8, main);
  tft.fillTriangle(118, 88, 202, 88, 160, 130, main);
  tft.setTextColor(trim);
  tft.setTextSize(2);
  char letter[2] = {LETTERS[h], 0};
  tft.drawCentreString(letter, 160, 40, 4);
  tft.setTextSize(1);
  tft.drawCentreString(NAMES[h], 160, 158, 4);
  tft.drawCentreString(ANIMALS[h], 160, 196, 2);
  // IF the house is ... THEN glow its colour (Megaspy's first IF/THEN!)
  if (h == GRYFFINDOR) glow(1, 0, 0);
  else if (h == RAVENCLAW) glow(0, 0, 1);
  else if (h == HUFFLEPUFF) glow(1, 1, 0);
  else glow(0, 1, 0);
  fanfare();
  for (int i = 0; i < 40; i++) {  // sparkles!
    int x = random(320), y = random(240);
    tft.drawFastHLine(x - 4, y, 9, trim);
    tft.drawFastVLine(x, y - 4, 9, trim);
    delay(25);
  }
}

void findSparkle() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.drawCentreString("Looking for Sparkle...", 160, 110, 4);
  IPAddress ip = MDNS.queryHost(SPARKLE_HOST, 3000);
  sparkle = "http://" + (ip == IPAddress(0, 0, 0, 0) ? String(SPARKLE_IP_FALLBACK) : ip.toString());
  Serial.println("Sparkle: " + sparkle);
  ask("/api/screen?view=hat");  // put Sparkle's tiny screen on the Sorting Hat too
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  glow(0, 0, 0);
  ledcSetup(0, 2000, 8);
  ledcAttachPin(SPEAKER, 0);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(1);
  touchSpi.begin(XPT_CLK, XPT_MISO, XPT_MOSI, XPT_CS);
  touch.begin(touchSpi);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_MAGENTA);
  tft.drawCentreString("Joining Wi-Fi...", 160, 110, 4);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(200);
  MDNS.begin("sparkle-hat");
  findSparkle();
  waitingScreen();
}

void loop() {
  static String shown = "waiting";
  static int shownStart = -1;
  static unsigned long thinkSince = 0;

  if (touch.touched()) {  // tap = sort me!
    ask("/api/hat?go=1");
    while (touch.touched()) delay(20);
  }

  String j = ask("/api/hat");
  if (j == "") {
    delay(1000);
    findSparkle();
    shown = "";
    return;
  }
  String state = j.indexOf("\"thinking\"") >= 0 ? "thinking" : j.indexOf("\"done\"") >= 0 ? "done" : "waiting";
  int start = jsonInt(j, "start");

  if (state == "waiting" && shown != "waiting") waitingScreen();
  if (state == "thinking") {
    if (shown != "thinking") {
      tft.fillScreen(rgb(0x2A1A4A));
      thinkSince = millis();
    }
    thinkingFrame(millis() - thinkSince);
  }
  if (state == "done" && (shown != "done" || start != shownStart)) {
    houseScreen(jsonInt(j, "house"));
    shownStart = start;
  }
  shown = state;
  delay(state == "thinking" ? 60 : 250);
}
