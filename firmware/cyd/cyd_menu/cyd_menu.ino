// Sparkle's Remote: the yellow touchscreen (CYD) is a menu for Sparkle.
// Tap a game: Sparkle switches to it, and this screen becomes that game's colourful helper.
// The action stays on Sparkle (shake, clap, swish); this screen shows pictures, words and buttons.
// Tap "< Menu" (top left) to go back.
//
// Flash: make cyd-flash CYD_SKETCH=firmware/cyd/cyd_menu
// Pictures: icons.h (made by tools/emoji_icons.py from Noto Color Emoji)

#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <XPT2046_Touchscreen.h>
#include "secrets.h"
#include "icons.h"

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
Preferences prefs;
String sparkle;

uint16_t rgb(uint32_t c) { return tft.color565(c >> 16, (c >> 8) & 255, c & 255); }
void icon(int x, int y, const uint16_t* data, int size) { tft.pushImage(x, y, size, size, data, ICON_KEY); }

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

String jsonStr(const String& j, const char* key) {
  int i = j.indexOf(String("\"") + key + "\":\"");
  if (i < 0) return "";
  i += strlen(key) + 4;
  return j.substring(i, j.indexOf('"', i));
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

// word-wrapped text inside a box, centred on each line
void wrapText(const String& text, int x, int y, int w, int font, uint16_t color) {
  tft.setTextColor(color);
  String line, word;
  int lineH = tft.fontHeight(font) + 2;
  String rest = text + " ";
  while (rest.length()) {
    int sp = rest.indexOf(' ');
    word = rest.substring(0, sp);
    rest = rest.substring(sp + 1);
    String trial = line.length() ? line + " " + word : word;
    if (tft.textWidth(trial, font) > w && line.length()) {
      tft.drawCentreString(line, x + w / 2, y, font);
      y += lineH;
      line = word;
    } else {
      line = trial;
    }
  }
  if (line.length()) tft.drawCentreString(line, x + w / 2, y, font);
}

// a speech bubble with a little tail pointing left
void bubble(int x, int y, int w, int h, const String& text) {
  tft.fillRoundRect(x, y, w, h, 12, TFT_WHITE);
  tft.fillTriangle(x + 2, y + h - 30, x + 2, y + h - 12, x - 14, y + h - 8, TFT_WHITE);
  wrapText(text, x + 6, y + 10, w - 12, 2, TFT_BLACK);
}

bool backTapped(int x, int y) { return y < 36 && x < 90; }

// ---------------------------------------------------------------- the games
enum Game { G_HAT, G_CAM, G_SNITCH, G_DANCE, G_OWL, G_WAND, G_POTION, G_DISCO, G_COUNT, G_MENU };
const char* TITLES[G_COUNT] = {"Sorting Hat", "Spy Camera", "Snitch", "Dance", "Owl Post", "Wand Wave", "Potions", "Disco"};
const uint32_t COLORS[G_COUNT] = {0x6B3FA0, 0x333344, 0xB8860B, 0xC2185B, 0x3A5A8A, 0x1E5AA8, 0x2E7D32, 0xE65100};
const char* VIEWS[G_COUNT] = {"hat", "eyes", "score", "dance", "owl", "wand", "potion", "lights"};
const uint16_t* const MENU_ICONS[G_COUNT] = {HAT, CAMERA, TROPHY, DANCER, OWL, WAND, POTION, DISCO};
Game game = G_MENU;

// ================= Sorting Hat: the hat talks in a speech bubble; the house gets its crest
const char* HOUSES[4] = {"GRYFFINDOR!", "RAVENCLAW!", "HUFFLEPUFF!", "SLYTHERIN!"};
const char* ANIMALS[4] = {"Brave like a lion", "Clever like an eagle", "Kind like a badger", "Sly like a snake"};
const uint32_t HOUSE_MAIN[4] = {0xAE0001, 0x1E3A8A, 0xF2C230, 0x1F6B3A};
const uint32_t HOUSE_TRIM[4] = {0xFFC93C, 0xC9A66B, 0x222222, 0xC0C0C0};
const char* HAT_THINKING[5] = {"Hmm... difficult. VERY difficult.", "Plenty of courage, I see...",
                               "Not a bad mind, either...", "A kind heart, too!", "Where shall I put you?"};
enum { GRYFFINDOR, RAVENCLAW, HUFFLEPUFF, SLYTHERIN };
String hatShown;
long hatShownStart = -2;

void drawHat(int cx, bool talking) {
  uint16_t brown = rgb(0x6B4423), dark = rgb(0x3A2410);
  tft.fillTriangle(cx - 50, 190, cx + 50, 190, cx + 18, 80, brown);
  tft.fillTriangle(cx + 18, 80, cx + 30, 104, cx + 46, 92, brown);
  tft.fillRoundRect(cx - 78, 186, 156, 16, 8, brown);
  tft.fillEllipse(cx - 16, 150, 6, 4, dark);
  tft.fillEllipse(cx + 16, 150, 6, 4, dark);
  if (talking) tft.fillEllipse(cx, 170, 14, 6, dark);
  else tft.fillRect(cx - 16, 170, 32, 4, dark);
}

void hatScene(const char* words, int wobble, bool talking) {
  tft.fillRect(0, 36, 320, 204, rgb(0x2A1A4A));
  drawHat(88 + wobble, talking);
  bubble(178, 50, 136, 110, words);
}

void houseReveal(int h) {
  uint16_t main = rgb(HOUSE_MAIN[h]), trim = rgb(HOUSE_TRIM[h]);
  header("Sorting Hat", HOUSE_MAIN[h]);
  // our crest: a shield in the house colours with the house animal
  tft.fillRoundRect(115, 38, 90, 66, 10, trim);
  tft.fillTriangle(115, 98, 205, 98, 160, 140, trim);
  tft.fillRoundRect(121, 44, 78, 56, 8, main);
  tft.fillTriangle(121, 94, 199, 94, 160, 132, main);
  icon(136, 54, HOUSE_ICONS[h], 48);
  tft.setTextColor(trim);
  tft.drawCentreString(HOUSES[h], 160, 150, 4);
  tft.drawCentreString(ANIMALS[h], 160, 180, 2);
  tft.fillRoundRect(20, 202, 280, 32, 10, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  String say = String("Hat: \"Better be... ") + HOUSES[h] + "\"";
  tft.drawCentreString(say, 160, 210, 2);
  // IF the house is ... THEN glow its colour (Megaspy's first IF/THEN!)
  if (h == GRYFFINDOR) glow(1, 0, 0);
  else if (h == RAVENCLAW) glow(0, 0, 1);
  else if (h == HUFFLEPUFF) glow(1, 1, 0);
  else glow(0, 1, 0);
  fanfare();
  for (int i = 0; i < 30; i++) {
    int x = random(10, 310), y = random(40, 140);
    if (x > 105 && x < 215) continue;  // don't sparkle over the crest
    tft.drawFastHLine(x - 4, y, 9, trim);
    tft.drawFastVLine(x, y - 4, 9, trim);
    delay(15);
  }
}

void hatTick() {
  static unsigned long thinkSince = 0;
  String j = ask("/api/hat");
  if (j == "") return;
  String state = j.indexOf("\"thinking\"") >= 0 ? "thinking" : j.indexOf("\"done\"") >= 0 ? "done" : "waiting";
  long start = jsonNum(j, "start");
  bool fresh = hatShownStart != -2 && start != hatShownStart;
  if ((state == "waiting" && hatShown != "waiting") || (state == "done" && hatShownStart == -2)) {
    header("Sorting Hat", 0x2A1A4A);
    hatScene("Shake Sparkle, and I shall sort you!", 0, false);
    glow(0, 0, 0);
  }
  if (state == "thinking") {
    if (hatShown != "thinking") {
      header("Sorting Hat", 0x2A1A4A);
      thinkSince = millis();
    }
    unsigned long t = millis() - thinkSince;
    static int lastLine = -1;
    int line = (t / 900) % 5;
    if (line != lastLine || hatShown != "thinking") hatScene(HAT_THINKING[line], (t / 200) % 2 ? 4 : -4, true);
    lastLine = line;
    glow((t / 300) % 3 == 0, (t / 300) % 3 == 1, (t / 300) % 3 == 2);
  }
  if (state == "done" && fresh) houseReveal(jsonNum(j, "house"));
  hatShownStart = start;
  hatShown = state;
}

// ================= Spy Camera: Sparkle's camera on the big screen
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

// ================= Catch the Snitch: the game is here; Sparkle hears the claps and keeps the score
#define SNITCH_WIN 10
#define CATCH_GRACE_MS 2000  // Megaspy's rule: in the box 2 seconds ago still counts!
#define BOX 34               // half the size of the box in the middle
const uint32_t PITCH = 0x1B5E20;
float snPhase = 0;
int snX = 160, snY = 138, snScore = 0, snBest = 0;
long snClaps = -1;
unsigned long snLastInBox = 0, snLastPoll = 0, snLastMove = 0, snMsgUntil = 0;
bool snWon = false;

bool snInBox() { return abs(snX - 160) <= BOX && abs(snY - 138) <= BOX; }

void pitchBits() {
  uint16_t white = TFT_WHITE, gold = rgb(0xFFD700);
  tft.drawCircle(160, 138, 50, rgb(0x2E7D32));
  for (int side = 0; side < 2; side++) {  // goal hoops on poles
    int x = side ? 300 : 20;
    for (int k = 0; k < 3; k++) {
      int hx = x + (side ? -k * 12 : k * 12), top = 70 + k * 18;
      tft.drawCircle(hx, top, 8, white);
      tft.drawFastVLine(hx, top + 8, 230 - top - 8, white);
    }
  }
  tft.drawRect(160 - BOX - 12, 138 - BOX - 12, 2 * BOX + 24, 2 * BOX + 24, gold);
  tft.drawRect(160 - BOX - 11, 138 - BOX - 11, 2 * BOX + 22, 2 * BOX + 22, gold);
}

void snitchScoreText() {
  char buf[24];
  tft.fillRect(84, 4, 236, 28, rgb(PITCH));
  tft.setTextColor(rgb(0xFFD700));
  snprintf(buf, sizeof(buf), "Score %d/10  Best %d", snScore, snBest);
  tft.drawRightString(buf, 314, 9, 2);
}

void snitchEnter() {
  header("", PITCH);
  snScore = 0;
  snWon = false;
  snClaps = -1;
  snitchScoreText();
  pitchBits();
  ask("/api/score?score=0");
}

void drawSnitchAt(int x, int y, bool flap, bool erase) {
  uint16_t bg = rgb(PITCH);
  if (erase) {
    tft.fillRect(x - 20, y - 12, 41, 25, bg);
    return;
  }
  uint16_t white = TFT_WHITE;
  if (flap) {
    tft.fillTriangle(x - 7, y, x - 20, y - 10, x - 16, y + 2, white);
    tft.fillTriangle(x + 7, y, x + 20, y - 10, x + 16, y + 2, white);
  } else {
    tft.fillTriangle(x - 7, y, x - 20, y + 8, x - 16, y - 3, white);
    tft.fillTriangle(x + 7, y, x + 20, y + 8, x + 16, y - 3, white);
  }
  tft.fillCircle(x, y, 7, rgb(0xFFD700));
  tft.drawCircle(x, y, 7, rgb(0xB8860B));
  tft.drawPixel(x - 2, y - 2, white);
}

void snitchWin() {
  snWon = true;
  tft.fillScreen(rgb(0xFFD700));
  icon(136, 40, TROPHY, 48);
  tft.setTextColor(rgb(0x7A4A00));
  tft.setTextSize(2);
  tft.drawCentreString("YOU WIN!", 160, 100, 4);
  tft.setTextSize(1);
  tft.drawCentreString("10 snitches! You are a Seeker!", 160, 160, 2);
  tft.drawCentreString("Tap to play again", 160, 200, 4);
  for (int k = 0; k < 2; k++) fanfare();
}

void snitchTry() {
  unsigned long now = millis();
  bool got = snInBox() || (snLastInBox && now - snLastInBox <= CATCH_GRACE_MS);
  if (got) {
    snScore++;
    snLastInBox = 0;
    if (snScore > snBest) snBest = snScore;
    ask("/api/score?score=" + String(snScore) + "&best=" + String(snBest));
    if (snScore >= SNITCH_WIN) {
      snitchWin();
      return;
    }
    bigText("GOT IT!", 196, rgb(0xFFD700), PITCH);
    note(1047, 90);
    note(1319, 160);
  } else {
    bigText("miss! try again", 196, TFT_WHITE, PITCH);
    note(330, 120);
  }
  snitchScoreText();
  snMsgUntil = now + 900;
}

void snitchTick() {
  if (snWon) return;
  unsigned long now = millis();
  // ask Sparkle how many claps it heard
  if (now - snLastPoll > 150) {
    snLastPoll = now;
    long claps = jsonNum(ask("/api/score"), "claps");
    if (claps >= 0) {
      if (snClaps >= 0 && claps > snClaps) snitchTry();
      snClaps = claps;
    }
  }
  if (snWon) return;
  if (snMsgUntil && now > snMsgUntil) {
    tft.fillRect(0, 196, 320, 30, rgb(PITCH));
    snMsgUntil = 0;
    pitchBits();
  }
  // fly!
  unsigned long dt = snLastMove ? now - snLastMove : 0;
  snLastMove = now;
  float speed = 1.0f + snScore * 0.12f;  // faster after every catch
  if (snInBox()) speed *= 0.35f;          // ...but it hovers in the box
  drawSnitchAt(snX, snY, false, true);
  snPhase += dt * speed;
  snX = 160 + (int)(sinf(snPhase / 430.0f) * 135);
  snY = 138 + (int)(sinf(snPhase / 270.0f) * 80);
  if (snInBox()) snLastInBox = now;
  pitchBits();
  drawSnitchAt(snX, snY, (now / 80) % 2, false);
}

// ================= Owl Post: send letters, and Sparkle sends kind letters back
const char* LETTERS_TO_SEND[6] = {"I LOVE YOU!", "HOOT HOOT!", "HELLO SPARKLE", "YOU ARE MAGIC", "UNICORNS RULE", "TOP SECRET!"};
long owlSeen = -2;
bool owlReading = false;

void owlEnter() {
  owlReading = false;
  header("Owl Post", 0x1D3346);
  for (int i = 0; i < 6; i++) {
    int x = 8 + (i % 2) * 156, y = 40 + (i / 2) * 52;
    tft.fillRoundRect(x, y, 148, 46, 10, rgb(0x8FD3FF));
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString(LETTERS_TO_SEND[i], x + 74, y + 15, 2);
  }
  icon(8, 194, OWL, 48);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Shake Sparkle and it", 64, 200, 2);
  tft.drawString("sends YOU a letter!", 64, 218, 2);
}

void owlTap(int x, int y) {
  if (owlReading) {  // close the letter
    owlEnter();
    return;
  }
  if (y < 40 || y > 190) return;
  int i = (y - 40) / 52 * 2 + (x >= 160);
  if (i < 0 || i > 5) return;
  ask("/api/owl?msg=" + urlEncode(LETTERS_TO_SEND[i]));
  header("Owl Post", 0x1D3346);
  for (int x2 = -48; x2 < 330; x2 += 12) {  // our owl flies off to Sparkle
    tft.fillRect(x2 - 12, 90, 60, 52, rgb(0x1D3346));
    icon(x2, 90, OWL, 48);
    delay(25);
  }
  bigText("Look at Sparkle!", 160, TFT_YELLOW, 0x1D3346);
  note(880, 100);
  note(660, 160);
  delay(1500);
  owlEnter();
}

void owlTick() {
  String j = ask("/api/owl");
  long at = jsonNum(j, "replyAt");
  if (at < 0) return;
  if (owlSeen == -2) owlSeen = at;  // don't show an old letter when we open the page
  if (at == owlSeen || at == 0) return;
  owlSeen = at;
  // Sparkle's owl flies in with a letter!
  header("Owl Post", 0x7EC8FF);
  for (int x = 330; x > 136; x -= 10) {
    tft.fillRect(x, 50, 60, 52, rgb(0x7EC8FF));
    icon(x, 50, OWL, 48);
    delay(20);
  }
  note(660, 100);
  note(880, 160);
  tft.fillRoundRect(20, 106, 280, 100, 10, rgb(0xFBF3DF));
  tft.drawRoundRect(20, 106, 280, 100, 10, rgb(0xB08A4A));
  tft.setTextColor(rgb(0x5A3412));
  tft.drawString("A letter from Sparkle:", 32, 112, 2);
  wrapText(jsonStr(j, "reply"), 30, 136, 260, 4, rgb(0x1F4D2B));
  tft.setTextColor(TFT_BLACK);
  tft.drawCentreString("Tap to write back", 160, 214, 2);
  owlReading = true;
}

// ================= Wand Wave: the spell, big and colourful
const char* SPELLS[5] = {"LUMOS!", "ACCIO!", "LEVIOSA!", "EXPELLIARMUS!", "STUPEFY!"};
const char* SPELL_SAYS[5] = {"Light up!", "Come here, star!", "The feather floats!", "Sparks fly!", "FLASH!"};
const uint32_t SPELL_COLORS[5] = {0xFFF59D, 0xFFD54F, 0xB3E5FC, 0xFF8A80, 0xEA80FC};
const uint16_t* const SPELL_ICONS[5] = {BULB, STAR, FEATHER, ZAP, BOOM};
long wandSeen = -2;
int spellsCast = 0;

void wandWaiting() {
  header("Wand Wave", 0x0D1B3A);
  icon(136, 50, WAND, 48);
  bigText("Swish Sparkle", 120, TFT_YELLOW, 0x0D1B3A);
  bigText("like a wand!", 150, TFT_YELLOW, 0x0D1B3A);
}

void wandTick() {
  String j = ask("/api/wand");
  if (j == "") return;
  long spell = jsonNum(j, "spell"), at = jsonNum(j, "at");
  if (wandSeen == -2) {  // just opened: wait for a new spell
    wandSeen = at;
    wandWaiting();
    return;
  }
  if (at == wandSeen || spell < 0 || spell > 4) return;
  wandSeen = at;
  spellsCast++;
  header("Wand Wave", SPELL_COLORS[spell]);
  for (int i = 0; i < 30; i++) {
    int x = random(320), y = random(40, 240);
    tft.drawFastHLine(x - 5, y, 11, TFT_WHITE);
    tft.drawFastVLine(x, y - 5, 11, TFT_WHITE);
  }
  icon(136, 44, SPELL_ICONS[spell], 48);
  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(spell == 3 ? 1 : 2);  // EXPELLIARMUS! is too long to be extra big
  tft.drawCentreString(SPELLS[spell], 160, spell == 3 ? 116 : 104, 4);
  tft.setTextSize(1);
  tft.drawCentreString(SPELL_SAYS[spell], 160, 164, 4);
  char buf[32];
  snprintf(buf, sizeof(buf), "Spells cast: %d", spellsCast);
  tft.drawCentreString(buf, 160, 206, 2);
  if (spell == 0) glow(1, 1, 1);  // Lumos lights this screen's light too
  else if (spell == 4) glow(1, 0, 1);
  else glow(0, 0, 0);
  note(988, 80);
  note(1319, 160);
}

// ================= Potions: tap 3 ingredients, then shake Sparkle to stir. Found recipes go in the book.
const char* INGREDIENTS[8] = {"unicorn", "frog", "rainbow", "spider", "candy", "pepper", "feather", "star"};
const char* POTIONS[12] = {"MEOW! YOU'RE A CAT!", "PINK HAIR!", "YOU CAN FLY!", "BUBBLE BURPS!", "RIBBIT! FROG VOICE",
                           "YOU'RE INVISIBLE!", "YOU GLOW LIKE A STAR", "UNICORN HORN!", "SUPER SNEEZE! ACHOO",
                           "GIANT BUNNY EARS!", "YOU BREATHE FIRE!", "EVERYTHING TASTES SWEET"};
int picks[3], pickCount = 0;
uint8_t book[12][3];  // the recipe that first made each potion
uint16_t bookFound = 0;
int bookPage = -1;    // -1 = not reading the book

void loadBook() {
  prefs.begin("potions", true);
  bookFound = prefs.getUShort("found", 0);
  prefs.getBytes("book", book, sizeof(book));
  prefs.end();
}

void saveBook() {
  prefs.begin("potions", false);
  prefs.putUShort("found", bookFound);
  prefs.putBytes("book", book, sizeof(book));
  prefs.end();
}

// same recipe -> same potion, exactly like the web page
int potionIndex() {
  int s[3] = {picks[0], picks[1], picks[2]};
  for (int a = 0; a < 3; a++)
    for (int b = a + 1; b < 3; b++)
      if (s[b] < s[a]) { int t = s[a]; s[a] = s[b]; s[b] = t; }
  long h = 0;
  for (int i = 0; i < 3; i++) h = h * 11 + s[i] + 1;
  return h % 12;
}

void potionEnter() {
  bookPage = -1;
  header("Potions", 0x13261A);
  for (int i = 0; i < 8; i++) {
    int x = 6 + (i % 4) * 78, y = 38 + (i / 4) * 66;
    bool in = false;
    for (int k = 0; k < pickCount; k++) in |= picks[k] == i;
    tft.fillRoundRect(x, y, 72, 60, 10, rgb(in ? 0x7DFF9D : 0x2E5A3A));
    if (in) tft.drawRoundRect(x, y, 72, 60, 10, TFT_WHITE);
    icon(x + 20, y + 4, INGREDIENT_ICONS[i], 32);
    tft.setTextColor(in ? TFT_BLACK : TFT_WHITE);
    tft.drawCentreString(INGREDIENTS[i], x + 36, y + 40, 2);
  }
  char buf[32];
  snprintf(buf, sizeof(buf), pickCount < 3 ? "Pick %d more!" : "Shake Sparkle to stir!", 3 - pickCount);
  bigText(buf, 172, TFT_YELLOW, 0x13261A, 2);
  tft.fillRoundRect(8, 198, 100, 36, 10, rgb(0x555555));
  tft.setTextColor(TFT_WHITE);
  tft.drawCentreString("Empty", 58, 208, 2);
  tft.fillRoundRect(170, 198, 142, 36, 10, rgb(0x8E24AA));
  icon(176, 200, BOOK, 32);
  char rb[20];
  snprintf(rb, sizeof(rb), "Recipes %d/12", __builtin_popcount(bookFound));
  tft.drawString(rb, 212, 208, 2);
}

void bookShow(int page) {
  bookPage = page;
  header("Recipe Book", 0x4A148C);
  int shown = 0, row = 0;
  for (int p = 0; p < 12; p++) {
    if (!(bookFound & (1 << p))) continue;
    if (shown++ < page * 4) continue;
    if (row == 4) break;
    int y = 40 + row * 46;
    for (int k = 0; k < 3; k++) icon(6 + k * 34, y + 4, INGREDIENT_ICONS[book[p][k]], 32);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("=", 110, y + 12, 2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(POTIONS[p], 124, y + 12, 2);
    row++;
  }
  tft.setTextColor(rgb(0xE1BEE7));
  if (!bookFound) tft.drawCentreString("No potions yet. Go brew one!", 160, 110, 2);
  char buf[40];
  snprintf(buf, sizeof(buf), "Found %d of 12. Tap for more.", __builtin_popcount(bookFound));
  tft.drawCentreString(buf, 160, 222, 2);
}

void potionTap(int x, int y) {
  if (bookPage >= 0) {  // in the book: tap = next page (or back to the cauldron)
    int found = __builtin_popcount(bookFound);
    if ((bookPage + 1) * 4 < found) bookShow(bookPage + 1);
    else potionEnter();
    return;
  }
  if (y >= 198 && x < 110) {  // Empty
    pickCount = 0;
    ask("/api/potion?empty=1");
  } else if (y >= 198 && x > 168) {  // Recipe book
    bookShow(0);
    return;
  } else if (y >= 38 && y < 170 && pickCount < 3) {
    int i = (y - 38) / 66 * 4 + x / 80;
    for (int k = 0; k < pickCount; k++)
      if (picks[k] == i) return;
    picks[pickCount++] = i;
    String q = "/api/potion?ing=";
    for (int k = 0; k < pickCount; k++) q += String(k ? "," : "") + picks[k];
    if (pickCount == 3) q += "&msg=" + urlEncode(POTIONS[potionIndex()]);
    ask(q);
    note(600 + pickCount * 150, 90);
  }
  potionEnter();
}

void potionTick() {
  static long lastState = -1;
  if (bookPage >= 0) return;
  long st = jsonNum(ask("/api/potion"), "state");
  if (st == lastState) return;
  if (st == 2) bigText("Bloop... blip... bubble...", 172, rgb(0x7DFF9D), 0x13261A, 2);
  if (st == 3 && pickCount == 3) {
    int p = potionIndex();
    bool isNew = !(bookFound & (1 << p));
    if (isNew) {
      bookFound |= 1 << p;
      for (int k = 0; k < 3; k++) book[p][k] = picks[k];
      saveBook();
    }
    header("Potions", 0x6A1B9A);
    for (int k = 0; k < 3; k++) icon(100 + k * 42, 44, INGREDIENT_ICONS[picks[k]], 32);
    icon(136, 84, POTION, 48);
    wrapText(POTIONS[p], 10, 140, 300, 4, TFT_YELLOW);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString(isNew ? "NEW recipe for your book!" : "Shake Sparkle to empty it", 160, 214, 2);
    fanfare();
  }
  if (st == 0 && lastState == 3) {
    pickCount = 0;
    potionEnter();
  }
  lastState = st;
}

// ================= Dance: count and hearts
void danceTick() {
  static long lastCount = -2;
  String j = ask("/api/dance");
  long left = jsonNum(j, "left"), count = jsonNum(j, "count");
  if (left > 0) {
    for (int i = 0; i < 6; i++) {
      int x = random(20, 300), y = random(50, 220), r = random(4, 10);
      uint16_t c = rgb(i % 2 ? 0xFF4FB8 : 0xFF9EC4);
      tft.fillCircle(x - r, y, r, c);
      tft.fillCircle(x + r, y, r, c);
      tft.fillTriangle(x - 2 * r, y + 2, x + 2 * r, y + 2, x, y + 2 * r + 4, c);
    }
    glow((millis() / 200) % 2, 0, 1);
    lastCount = -1;  // redraw the counter when the dance ends
  } else if (count != lastCount) {
    header("Dance", 0x4A0E2E);
    icon(136, 44, DANCER, 48);
    char buf[32];
    snprintf(buf, sizeof(buf), "Danced %ld times!", count < 0 ? 0 : count);
    bigText(buf, 110, TFT_WHITE, 0x4A0E2E);
    bigText("Clap to make", 150, TFT_YELLOW, 0x4A0E2E);
    bigText("Sparkle dance!", 180, TFT_YELLOW, 0x4A0E2E);
    lastCount = count;
    glow(0, 0, 0);
  }
}

// ================= Disco: Sparkle's light and this screen party together
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
    icon(x + 13, y + 8, MENU_ICONS[i], 48);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString(TITLES[i], x + 37, y + 66, 2);
  }
  glow(0, 0, 0);
}

void openGame(int gi) {
  game = (Game)gi;
  ask(String("/api/screen?view=") + VIEWS[gi]);
  if (game != G_DISCO) ask("/api/led?mode=off");
  hatShown = "";
  hatShownStart = -2;
  owlSeen = -2;
  wandSeen = -2;
  switch (game) {
    case G_HAT: header("Sorting Hat", 0x2A1A4A); break;
    case G_CAM: header("Spy Camera", 0x222222); break;
    case G_SNITCH: snitchEnter(); break;
    case G_DANCE: header("Dance", 0x4A0E2E); break;
    case G_OWL: owlEnter(); break;
    case G_WAND: header("Wand Wave", 0x0D1B3A); break;
    case G_POTION: potionEnter(); break;
    case G_DISCO:
      ask("/api/led?mode=disco");
      header("Disco", 0x000000);
      break;
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
  loadBook();

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
        if (i >= 0 && i < G_COUNT) openGame(i);
      }
    } else if (backTapped(x, y) && !(game == G_SNITCH && snWon)) {
      if (game == G_DISCO) ask("/api/led?mode=off");
      ask("/api/screen?view=home");
      game = G_MENU;
      drawMenu();
    } else if (game == G_OWL) owlTap(x, y);
    else if (game == G_POTION) potionTap(x, y);
    else if (game == G_SNITCH && snWon) snitchEnter();  // play again
    // (no tap-to-sort on purpose: shaking Sparkle is the magic)
  }
  switch (game) {
    case G_HAT: hatTick(); break;
    case G_CAM: camTick(); break;
    case G_SNITCH: snitchTick(); break;
    case G_DANCE: danceTick(); break;
    case G_OWL: if (!owlReading) owlTick(); break;
    case G_WAND: wandTick(); break;
    case G_POTION: potionTick(); break;
    case G_DISCO: discoTick(); break;
    default: break;
  }
  delay(game == G_SNITCH ? 15 : game == G_HAT ? 60 : game == G_DISCO ? 120 : 200);
}
