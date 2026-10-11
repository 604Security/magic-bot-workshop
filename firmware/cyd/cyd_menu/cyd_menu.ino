// Sparkle's Remote: the yellow touchscreen (CYD) is a menu for Sparkle.
// Tap a game: Sparkle switches to it, and this screen becomes that game's colourful helper.
// The action stays on Sparkle (shake, clap, swish); this screen shows scores, colours and buttons.
// Tap "< Menu" (top left) to go back.
//
// Flash: make cyd-flash CYD_SKETCH=firmware/cyd/cyd_menu

#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <XPT2046_Touchscreen.h>
#include "secrets.h"

#define SPARKLE_HOST "sparkle"
#define SPARKLE_IP_FALLBACK "192.168.86.189"

#define XPT_IRQ  36
#define XPT_MOSI 32
#define XPT_MISO 39
#define XPT_CLK  25
#define XPT_CS   33
#define LED_R 4  // RGB light (on when LOW)
#define LED_G 16
#define LED_B 17
#define SPEAKER 26

TFT_eSPI tft;
SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touch(XPT_CS, XPT_IRQ);
String sparkle;

uint16_t rgb(uint32_t c) { return tft.color565(c >> 16, (c >> 8) & 255, c & 255); }

// ---------------------------------------------------------------- light, sound, touch
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
  note(523, 150);
  note(659, 150);
  note(784, 150);
  note(1047, 450);
}

// a tap: true once per press, with the spot on the screen
bool tapped(int& x, int& y) {
  static bool down = false;
  if (!touch.touched()) {
    down = false;
    return false;
  }
  if (down) return false;
  down = true;
  TS_Point p = touch.getPoint();
  x = constrain(map(p.x, 200, 3800, 0, 320), 0, 319);
  y = constrain(map(p.y, 200, 3800, 0, 240), 0, 239);
  return true;
}

// ---------------------------------------------------------------- talking to Sparkle
String ask(const String& path) {
  HTTPClient http;
  http.setTimeout(2000);
  if (!http.begin(sparkle + path)) return "";
  String body = http.GET() == 200 ? http.getString() : "";
  http.end();
  return body;
}

long jsonNum(const String& j, const char* key) {
  int i = j.indexOf(String("\"") + key + "\":");
  return i < 0 ? -1 : j.substring(i + strlen(key) + 3).toInt();
}

String urlEncode(const char* s) {
  String out;
  for (; *s; s++) {
    if (isalnum(*s)) out += *s;
    else if (*s == ' ') out += "%20";
    else {
      char b[4];
      snprintf(b, sizeof(b), "%%%02X", (uint8_t)*s);
      out += b;
    }
  }
  return out;
}

// ---------------------------------------------------------------- shared drawing
void header(const char* title, uint32_t bg) {
  tft.fillScreen(rgb(bg));
  tft.fillRoundRect(4, 4, 72, 26, 8, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString("< Menu", 40, 9, 2);
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString(title, 196, 6, 4);
}

void bigText(const char* text, int y, uint16_t color, uint32_t bg, int font = 4) {
  tft.fillRect(0, y, 320, tft.fontHeight(font) + 4, rgb(bg));
  tft.setTextColor(color);
  tft.drawCentreString(text, 160, y, font);
}

bool backTapped(int x, int y) { return y < 36 && x < 90; }

// ---------------------------------------------------------------- the games
enum Game { G_HAT, G_CAM, G_SNITCH, G_DANCE, G_OWL, G_WAND, G_POTION, G_DISCO, G_COUNT, G_MENU };
const char* TITLES[G_COUNT] = {"Sorting Hat", "Spy Camera", "Snitch", "Dance", "Owl Post", "Wand Wave", "Potions", "Disco"};
const char* HINTS[G_COUNT] = {"shake!", "spy!", "clap!", "clap!", "send!", "swish!", "mix!", "party!"};
const uint32_t COLORS[G_COUNT] = {0x6B3FA0, 0x222222, 0xB8860B, 0xC2185B, 0x3A5A8A, 0x1E5AA8, 0x2E7D32, 0xE65100};
const char* VIEWS[G_COUNT] = {"hat", "eyes", "snitch", "dance", "owl", "wand", "potion", "lights"};
Game game = G_MENU;

// --- Sorting Hat: the house in full colour (sorting only happens when you shake Sparkle!)
const char* HOUSES[4] = {"GRYFFINDOR!", "RAVENCLAW!", "HUFFLEPUFF!", "SLYTHERIN!"};
const char* ANIMALS[4] = {"Brave like a lion", "Clever like an eagle", "Kind like a badger", "Sly like a snake"};
const char LETTERS[4] = {'G', 'R', 'H', 'S'};
const uint32_t HOUSE_MAIN[4] = {0xAE0001, 0x1E3A8A, 0xF2C230, 0x1F6B3A};
const uint32_t HOUSE_TRIM[4] = {0xFFC93C, 0xC9A66B, 0x222222, 0xC0C0C0};
enum { GRYFFINDOR, RAVENCLAW, HUFFLEPUFF, SLYTHERIN };
String hatShown;
long hatShownStart = -2;

void drawHat(int dx, bool talking) {
  int cx = 160 + dx;
  uint16_t brown = rgb(0x6B4423), dark = rgb(0x3A2410);
  tft.fillTriangle(cx - 55, 170, cx + 55, 170, cx + 20, 50, brown);
  tft.fillTriangle(cx + 20, 50, cx + 32, 76, cx + 50, 62, brown);
  tft.fillRoundRect(cx - 85, 166, 170, 18, 9, brown);
  tft.fillEllipse(cx - 18, 128, 7, 4, dark);
  tft.fillEllipse(cx + 18, 128, 7, 4, dark);
  if (talking) tft.fillEllipse(cx, 150, 16, 7, dark);
  else tft.fillRect(cx - 18, 150, 36, 4, dark);
}

void houseReveal(int h) {
  uint16_t main = rgb(HOUSE_MAIN[h]), trim = rgb(HOUSE_TRIM[h]);
  header("Sorting Hat", HOUSE_MAIN[h]);
  tft.fillRoundRect(115, 40, 90, 70, 10, trim);
  tft.fillTriangle(115, 104, 205, 104, 160, 146, trim);
  tft.fillRoundRect(122, 47, 76, 60, 8, main);
  tft.fillTriangle(122, 100, 198, 100, 160, 138, main);
  tft.setTextColor(trim);
  tft.setTextSize(2);
  char letter[2] = {LETTERS[h], 0};
  tft.drawCentreString(letter, 160, 56, 4);
  tft.setTextSize(1);
  tft.drawCentreString(HOUSES[h], 160, 160, 4);
  tft.drawCentreString(ANIMALS[h], 160, 196, 2);
  tft.drawCentreString("Shake Sparkle for the next person!", 160, 220, 2);
  // IF the house is ... THEN glow its colour (Megaspy's first IF/THEN!)
  if (h == GRYFFINDOR) glow(1, 0, 0);
  else if (h == RAVENCLAW) glow(0, 0, 1);
  else if (h == HUFFLEPUFF) glow(1, 1, 0);
  else glow(0, 1, 0);
  fanfare();
  for (int i = 0; i < 30; i++) {
    int x = random(320), y = random(36, 150);
    tft.drawFastHLine(x - 4, y, 9, trim);
    tft.drawFastVLine(x, y - 4, 9, trim);
    delay(20);
  }
}

void hatTick() {
  static unsigned long thinkSince = 0;
  String j = ask("/api/hat");
  if (j == "") return;
  String state = j.indexOf("\"thinking\"") >= 0 ? "thinking" : j.indexOf("\"done\"") >= 0 ? "done" : "waiting";
  long start = jsonNum(j, "start");
  if (state == "waiting" && hatShown != "waiting") {
    header("Sorting Hat", 0x2A1A4A);
    drawHat(0, false);
    bigText("Shake Sparkle!", 196, TFT_YELLOW, 0x2A1A4A);
    glow(0, 0, 0);
  }
  if (state == "thinking") {
    static const char* WORDS[4] = {"Hmm...", "Tricky...", "VERY tricky...", "Hmm..."};
    if (hatShown != "thinking") {
      header("Sorting Hat", 0x2A1A4A);
      thinkSince = millis();
    }
    unsigned long t = millis() - thinkSince;
    tft.fillRect(0, 40, 320, 148, rgb(0x2A1A4A));
    drawHat((t / 200) % 2 ? 6 : -6, (t / 150) % 2);
    bigText(WORDS[(t / 900) % 4], 196, TFT_YELLOW, 0x2A1A4A);
    glow((t / 300) % 3 == 0, (t / 300) % 3 == 1, (t / 300) % 3 == 2);
  }
  if (state == "done" && (hatShown != "done" || start != hatShownStart)) {
    // only celebrate a sort that happened while we were watching
    if (hatShownStart != -2) houseReveal(jsonNum(j, "house"));
    else {
      header("Sorting Hat", 0x2A1A4A);
      drawHat(0, false);
      bigText("Shake Sparkle!", 196, TFT_YELLOW, 0x2A1A4A);
    }
    hatShownStart = start;
  }
  if (hatShownStart == -2) hatShownStart = start;
  hatShown = state;
}

// --- Spy Camera: Sparkle's camera on the big screen
bool drawBlock(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return false;
  tft.pushImage(x, y, w, h, bitmap);
  return true;
}

void camTick() {
  HTTPClient http;
  http.setTimeout(4000);
  if (!http.begin(sparkle + "/capture") || http.GET() != 200) {
    http.end();
    return;
  }
  int len = http.getSize();
  uint8_t* jpg = len > 0 && len < 120000 ? (uint8_t*)malloc(len) : nullptr;
  if (jpg) {
    WiFiClient* stream = http.getStreamPtr();
    int got = 0;
    unsigned long start = millis();
    while (got < len && millis() - start < 4000) {
      int n = stream->available();
      if (n > 0) got += stream->readBytes(jpg + got, min(n, len - got));
      else delay(1);
    }
    if (got == len) {
      TJpgDec.drawJpg(0, 0, jpg, len);
      tft.fillRoundRect(4, 4, 72, 26, 8, TFT_WHITE);  // keep the way back visible
      tft.setTextColor(TFT_BLACK);
      tft.drawCentreString("< Menu", 40, 9, 2);
    }
    free(jpg);
  }
  http.end();
}

// --- Snitch: a giant score
void snitchTick() {
  static long lastScore = -1;
  String j = ask("/api/snitch");
  long score = jsonNum(j, "score"), best = jsonNum(j, "best");
  if (score < 0 || score == lastScore) return;
  if (lastScore >= 0 && score > lastScore) {  // a catch!
    tft.fillScreen(rgb(0xFFD700));
    note(1047, 120);
    note(1319, 200);
    header("Snitch", 0x3B2A00);
  }
  lastScore = score;
  char buf[24];
  snprintf(buf, sizeof(buf), "%ld", score);
  tft.fillRect(0, 50, 320, 110, rgb(0x3B2A00));
  tft.setTextColor(rgb(0xFFD700));
  tft.setTextSize(2);
  tft.drawCentreString(buf, 160, 60, 4);
  tft.setTextSize(1);
  snprintf(buf, sizeof(buf), "Best: %ld", best);
  bigText(buf, 140, TFT_WHITE, 0x3B2A00, 2);
  bigText("Clap when it's in the box!", 190, TFT_YELLOW, 0x3B2A00);
}

// --- Dance: count and hearts
void danceTick() {
  static long lastCount = -1;
  String j = ask("/api/dance");
  long left = jsonNum(j, "left"), count = jsonNum(j, "count");
  if (left > 0) {  // dancing: hearts everywhere!
    for (int i = 0; i < 6; i++) {
      int x = random(20, 300), y = random(50, 220), r = random(4, 10);
      uint16_t c = rgb(i % 2 ? 0xFF4FB8 : 0xFF9EC4);
      tft.fillCircle(x - r, y, r, c);
      tft.fillCircle(x + r, y, r, c);
      tft.fillTriangle(x - 2 * r, y + 2, x + 2 * r, y + 2, x, y + 2 * r + 4, c);
    }
    glow((millis() / 200) % 2, 0, 1);
  } else if (count != lastCount || left == 0) {
    if (count != lastCount) {
      header("Dance", 0x4A0E2E);
      char buf[32];
      snprintf(buf, sizeof(buf), "Danced %ld times!", count < 0 ? 0 : count);
      bigText(buf, 90, TFT_WHITE, 0x4A0E2E);
      bigText("Clap to make", 140, TFT_YELLOW, 0x4A0E2E);
      bigText("Sparkle dance!", 170, TFT_YELLOW, 0x4A0E2E);
      lastCount = count;
    }
    glow(0, 0, 0);
  }
}

// --- Owl Post: letter buttons
const char* LETTERS_TO_SEND[6] = {"I LOVE YOU!", "HOOT HOOT!", "HELLO SPARKLE", "YOU ARE MAGIC", "UNICORNS RULE", "TOP SECRET!"};
void owlEnter() {
  header("Owl Post", 0x1D3346);
  for (int i = 0; i < 6; i++) {
    int x = 8 + (i % 2) * 156, y = 44 + (i / 2) * 64;
    tft.fillRoundRect(x, y, 148, 56, 10, rgb(0x8FD3FF));
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString(LETTERS_TO_SEND[i], x + 74, y + 20, 2);
  }
}
void owlTap(int x, int y) {
  if (y < 44) return;
  int i = (y - 44) / 64 * 2 + (x >= 160);
  if (i < 0 || i > 5) return;
  ask("/api/owl?msg=" + urlEncode(LETTERS_TO_SEND[i]));
  header("Owl Post", 0x1D3346);
  bigText("The owl is flying!", 90, TFT_YELLOW, 0x1D3346);
  bigText("Look at Sparkle!", 130, TFT_WHITE, 0x1D3346);
  note(880, 100);
  note(660, 160);
  delay(2500);
  owlEnter();
}

// --- Wand Wave: the spell in colour
const char* SPELLS[5] = {"LUMOS!", "ACCIO!", "LEVIOSA!", "EXPELLIARMUS!", "STUPEFY!"};
const uint32_t SPELL_COLORS[5] = {0xFFF59D, 0xFFD700, 0xB3E5FC, 0xFF5252, 0xE040FB};
void wandTick() {
  static long lastAt = -2;
  String j = ask("/api/wand");
  long spell = jsonNum(j, "spell"), at = jsonNum(j, "at");
  if (at == lastAt || j == "") return;
  bool fresh = lastAt != -2 && spell >= 0;
  lastAt = at;
  if (!fresh) {
    header("Wand Wave", 0x0D1B3A);
    bigText("Swish Sparkle", 100, TFT_YELLOW, 0x0D1B3A);
    bigText("like a wand!", 135, TFT_YELLOW, 0x0D1B3A);
    return;
  }
  header("Wand Wave", SPELL_COLORS[spell]);
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString(SPELLS[spell], 160, 100, 4);
  for (int i = 0; i < 25; i++) {
    int x = random(320), y = random(40, 240);
    tft.drawFastHLine(x - 5, y, 11, TFT_WHITE);
    tft.drawFastVLine(x, y - 5, 11, TFT_WHITE);
  }
  if (spell == 0) glow(1, 1, 1);  // Lumos lights this screen's light too
  note(988, 80);
  note(1319, 160);
  delay(1500);
  glow(0, 0, 0);
}

// --- Potions: tap 3 ingredients, then shake Sparkle to stir
const char* INGREDIENTS[8] = {"unicorn", "frog", "rainbow", "spider", "candy", "pepper", "feather", "star"};
const char* POTIONS[12] = {"MEOW! YOU'RE A CAT!", "PINK HAIR!", "YOU CAN FLY!", "BUBBLE BURPS!", "RIBBIT! FROG VOICE",
                           "YOU'RE INVISIBLE!", "YOU GLOW LIKE A STAR", "UNICORN HORN!", "SUPER SNEEZE! ACHOO",
                           "GIANT BUNNY EARS!", "YOU BREATHE FIRE!", "EVERYTHING TASTES SWEET"};
int picks[3], pickCount = 0;
void potionEnter() {
  header("Potions", 0x13261A);
  for (int i = 0; i < 8; i++) {
    int x = 6 + (i % 4) * 78, y = 44 + (i / 4) * 52;
    bool in = false;
    for (int k = 0; k < pickCount; k++) in |= picks[k] == i;
    tft.fillRoundRect(x, y, 72, 44, 8, rgb(in ? 0x7DFF9D : 0x2E7D32));
    tft.setTextColor(in ? TFT_BLACK : TFT_WHITE);
    tft.drawCentreString(INGREDIENTS[i], x + 36, y + 14, 2);
  }
  char buf[32];
  snprintf(buf, sizeof(buf), pickCount < 3 ? "Pick %d more!" : "Shake Sparkle to stir!", 3 - pickCount);
  bigText(buf, 160, TFT_YELLOW, 0x13261A);
  tft.fillRoundRect(110, 200, 100, 32, 8, rgb(0x555555));
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("Empty", 160, 208, 2);
}
// same recipe -> same potion, exactly like the web page
const char* potionFor() {
  int s[3] = {picks[0], picks[1], picks[2]};
  for (int a = 0; a < 3; a++)
    for (int b = a + 1; b < 3; b++)
      if (s[b] < s[a]) { int t = s[a]; s[a] = s[b]; s[b] = t; }
  long h = 0;
  for (int i = 0; i < 3; i++) h = h * 11 + s[i] + 1;
  return POTIONS[h % 12];
}
void potionTap(int x, int y) {
  if (y >= 200 && x > 100 && x < 220) {  // Empty
    pickCount = 0;
    ask("/api/potion?empty=1");
  } else if (y >= 44 && y < 148 && pickCount < 3) {
    int i = (y - 44) / 52 * 4 + x / 80;
    for (int k = 0; k < pickCount; k++)
      if (picks[k] == i) return;
    picks[pickCount++] = i;
    String q = "/api/potion?ing=";
    for (int k = 0; k < pickCount; k++) q += String(k ? "," : "") + picks[k];
    if (pickCount == 3) q += "&msg=" + urlEncode(potionFor());
    ask(q);
    note(600 + pickCount * 150, 90);
  }
  potionEnter();
}
void potionTick() {
  static long lastState = -1;
  long st = jsonNum(ask("/api/potion"), "state");
  if (st == lastState) return;
  if (st == 2) bigText("Bloop... blip...", 160, rgb(0x7DFF9D), 0x13261A);
  if (st == 3 && pickCount == 3) {
    header("Potions", 0x6A1B9A);
    bigText("Your potion:", 70, TFT_WHITE, 0x6A1B9A);
    bigText(potionFor(), 110, TFT_YELLOW, 0x6A1B9A, 2);
    bigText("Shake Sparkle to empty it", 190, TFT_WHITE, 0x6A1B9A, 2);
    fanfare();
  }
  if (st == 0 && lastState == 3) {
    pickCount = 0;
    potionEnter();
  }
  lastState = st;
}

// --- Disco: Sparkle's light and this screen party together
void discoEnter() {
  ask("/api/led?mode=disco");
  header("Disco", 0x000000);
}
void discoTick() {
  for (int i = 0; i < 8; i++)
    tft.fillCircle(random(320), random(40, 240), random(6, 22), rgb(random(0xFFFFFF) | 0x404040));
  glow(random(2), random(2), random(2));
}

// ---------------------------------------------------------------- the menu
void drawMenu() {
  tft.fillScreen(rgb(0x0D1410));
  tft.setTextColor(rgb(0x7DFF9D));
  tft.drawCentreString("Sparkle's Great Hall", 160, 6, 4);
  for (int i = 0; i < G_COUNT; i++) {
    int x = 4 + (i % 4) * 79, y = 38 + (i / 4) * 100;
    tft.fillRoundRect(x, y, 74, 94, 12, rgb(COLORS[i]));
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString(TITLES[i], x + 37, y + 30, 2);
    tft.setTextColor(TFT_YELLOW);
    tft.drawCentreString(HINTS[i], x + 37, y + 54, 2);
  }
  glow(0, 0, 0);
}

void openGame(int gi) {
  Game g = (Game)gi;
  game = g;
  ask(String("/api/screen?view=") + VIEWS[g]);
  if (game != G_DISCO) ask("/api/led?mode=off");
  hatShown = "";
  hatShownStart = -2;
  switch (g) {
    case G_HAT: header("Sorting Hat", 0x2A1A4A); break;
    case G_CAM: header("Spy Camera", 0x222222); break;
    case G_SNITCH: header("Snitch", 0x3B2A00); break;
    case G_DANCE: header("Dance", 0x4A0E2E); break;
    case G_OWL: owlEnter(); break;
    case G_WAND: header("Wand Wave", 0x0D1B3A); break;
    case G_POTION: potionEnter(); break;
    case G_DISCO: discoEnter(); break;
    default: break;
  }
}

void findSparkle() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.drawCentreString("Looking for Sparkle...", 160, 110, 4);
  IPAddress ip = MDNS.queryHost(SPARKLE_HOST, 3000);
  sparkle = "http://" + (ip == IPAddress(0, 0, 0, 0) ? String(SPARKLE_IP_FALLBACK) : ip.toString());
  Serial.println("Sparkle: " + sparkle);
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
  TJpgDec.setJpgScale(2);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(drawBlock);
  touchSpi.begin(XPT_CLK, XPT_MISO, XPT_MOSI, XPT_CS);
  touch.begin(touchSpi);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_MAGENTA);
  tft.drawCentreString("Joining Wi-Fi...", 160, 110, 4);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(200);
  MDNS.begin("sparkle-remote");
  findSparkle();
  ask("/api/screen?view=home");
  drawMenu();
}

void loop() {
  int x, y;
  if (tapped(x, y)) {
    if (game == G_MENU) {
      if (y >= 38) {
        int i = (y - 38) / 100 * 4 + x / 80;
        if (i >= 0 && i < G_COUNT) openGame((Game)i);
      }
    } else if (backTapped(x, y)) {
      if (game == G_DISCO) ask("/api/led?mode=off");
      ask("/api/screen?view=home");
      game = G_MENU;
      drawMenu();
    } else if (game == G_OWL) owlTap(x, y);
    else if (game == G_POTION) potionTap(x, y);
    // (no tap-to-sort on purpose: shaking Sparkle is the magic)
  }
  switch (game) {
    case G_HAT: hatTick(); break;
    case G_CAM: camTick(); break;
    case G_SNITCH: snitchTick(); break;
    case G_DANCE: danceTick(); break;
    case G_WAND: wandTick(); break;
    case G_POTION: potionTick(); break;
    case G_DISCO: discoTick(); break;
    default: break;
  }
  delay(game == G_HAT ? 60 : game == G_DISCO ? 120 : 200);
}
