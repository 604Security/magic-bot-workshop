// Sparkle's test menu: a friendly test menu for the robot, served over Wi-Fi.
//
// Open the address printed on serial (or http://sparkle.local) and pick a test:
//   Lights, Sounds, Eyes (camera), Memory (microSD), Wi-Fi, Screen.
// The OLED (XIAOML Kit, 72x40) shows a unicorn and follows along with the menu.
//
// Needs: esp32 core 2.0.17, Tools > PSRAM = OPI PSRAM, library U8g2,
//        secrets.h with WIFI_SSID / WIFI_PASSWORD (copy from secrets.h.example).
// Flash: make petbot-test

#include <WiFi.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <I2S.h>
#include <SD.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <math.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "secrets.h"
#include "page.h"
#include "unicorn.h"
#include "kind_notes.h"

// Her robot's name (she picked it on pickers/02-robot-name.html). 7 letters max to fit the screen.
#define ROBOT_NAME "Sparkle"
#define ROBOT_HOST "sparkle"  // web address: http://sparkle.local

// The kid's name lives in config.h, which git ignores, so her real name stays off GitHub.
// Copy config.h.example to config.h and put her name in it.
#if __has_include("config.h")
#include "config.h"
#endif
#ifndef KID_NAME
#define KID_NAME "Megaspy"
#endif

// Unicorn Dance Party (she picked dance 8, Heart Twirl, on pickers/03-dance-party.html)
#define CLAP_LEVEL 1200  // how loud a clap must be (quiet room ~20, talking ~100-500)
#define DANCE_MS   8000  // how long the unicorn dances after a clap (she asked for longer!)
#define HEARTS     10    // how many hearts float up (she asked for way more!)

// ---------------------------------------------------------------- pins
const int LED_PIN = 21;  // orange user LED, active LOW. Shared with the SD card CS!
const int SD_CS = 21;
const uint8_t OLED_ADDR = 0x3C;

// XIAO ESP32S3 Sense camera
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39
#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

// ---------------------------------------------------------------- state
bool cameraOk = false, micOk = false, screenOk = false;

enum LedMode { LED_OFF, LED_ON, LED_BLINK, LED_DISCO, LED_HELLO, LED_MODES };
const char* LED_MODE_NAMES[LED_MODES] = {"off", "on", "blink", "disco", "hello"};
volatile LedMode ledMode = LED_OFF;
volatile bool ledLit = false;
volatile bool sdBusy = false;  // SD test owns GPIO21 while true

volatile int soundLevel = 0;  // RMS of the latest 50 ms of audio
volatile int soundPeak = 0;   // loudest since the page last asked

enum ScreenView { V_HOME, V_LIGHTS, V_SOUNDS, V_EYES, V_MEMORY, V_WIFI, V_MESSAGE, V_DANCE, V_OWL, V_SNITCH, V_HAT, V_POTION, V_WAND, V_SCORE, V_COUNT };
const char* VIEW_NAMES[V_COUNT] = {"home", "lights", "sounds", "eyes", "memory", "wifi", "message", "dance", "owl", "snitch", "hat", "potion", "wand", "score"};
volatile ScreenView screenView = V_HOME;
volatile unsigned long danceStart = 0;  // millis() when the dance began, 0 = not dancing
volatile int danceCount = 0;

// Owl Post (Wizard School pick 3): an owl flies a letter onto the screen
#define OWL_FLY_MS 3000  // how long the owl flies before the letter opens
char owlMsg[25] = "";

// Catch the Snitch (Wizard School pick 7): clap when the snitch is in the box
volatile bool snitchClap = false;     // a clap (or the web Catch button) is waiting to be checked
float snitchPhase = 0;                // how far along its zig-zag path the snitch is
int snitchX = 36, snitchY = 20;
int snitchScore = 0, snitchBest = 0;
unsigned long snitchResultAt = 0;     // when the last catch/miss happened
bool snitchGot = false;
unsigned long snitchLastInBox = 0;

// Sorting Hat (Wizard School pick 2): clap and the hat thinks, then picks a house
#define GRYFFINDOR_CHANCE 25  // out of 100: 25 = every house has the same chance (Megaspy said 55 was too much!)
#define HAT_THINK_MS 3500     // how long the hat goes "hmm..."
const char* HOUSES[4] = {"GRYFFINDOR!", "RAVENCLAW!", "HUFFLEPUFF!", "SLYTHERIN!"};
const char* ANIMALS[4] = {"brave lion", "clever eagle", "kind badger", "sly snake"};
volatile unsigned long hatStart = 0;  // 0 = waiting to sort someone
volatile int hatHouse = 0;

// Shake detection with the IMU on the screen board (LSM6DS3TR-C), read directly over I2C
#define IMU_ADDR 0x6A
#define SHAKE_G  0.7f  // how hard a wiggle counts as a shake (1.0 = the pull of gravity)
bool imuOk = false;
float imuG = 1.0f;  // latest pull on Sparkle (1.0 = sitting still)

// Wand Wave (Wizard School pick 5): wave Sparkle like a wand to cast a random spell
#define WAVE_G   1.0f  // how hard a wave must be to cast (bigger = harder)
#define SPELL_MS 2500  // how long a spell's magic lasts on the screen
const char* SPELLS[5] = {"LUMOS!", "ACCIO!", "LEVIOSA!", "EXPELLIARMUS", "STUPEFY!"};
volatile int wandSpell = -1;  // -1 = no spell yet
volatile unsigned long wandAt = 0;

void castSpell() {
  if (wandAt && millis() - wandAt < SPELL_MS) return;  // one spell at a time
  wandSpell = esp_random() % 5;
  unsigned long now = millis();
  wandAt = now ? now : 1;
  if (wandSpell == 0) ledMode = LED_ON;  // Lumos lights Sparkle's real light!
}

bool imuInit() {
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(0x10);  // CTRL1_XL: accelerometer on, 104 Hz, +-2 g
  Wire.write(0x40);
  return Wire.endTransmission() == 0;
}

// true once when Sparkle is shaken: 4 big wiggles close together, then rests for 1.5 s
bool shaken() {
  static unsigned long lastRead = 0, lastHit = 0, lastShake = 0;
  static int hits = 0;
  unsigned long now = millis();
  if (!imuOk || now - lastRead < 20) return false;
  lastRead = now;
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(0x28);  // OUTX_L_XL
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(IMU_ADDR, 6) != 6) return false;
  int16_t v[3];
  for (int i = 0; i < 3; i++) v[i] = Wire.read() | (Wire.read() << 8);
  float g = sqrtf((float)v[0] * v[0] + (float)v[1] * v[1] + (float)v[2] * v[2]) * 0.061f / 1000.0f;
  imuG = g;
  if (fabsf(g - 1.0f) > SHAKE_G) {
    hits = now - lastHit > 400 ? 1 : hits + 1;
    lastHit = now;
  }
  if (hits >= 4 && now - lastShake > 1500) {
    hits = 0;
    lastShake = now;
    return true;
  }
  return false;
}

// Potion Mixer (Wizard School pick 6): pick ingredients on the web page, shake Sparkle to stir
#define BREW_MS 4000           // how long the cauldron bubbles
char potionMsg[25] = "";       // what the potion does (the web page picks it from the ingredients)
volatile int potionState = 0;  // 0 = empty, 1 = ingredients in, 2 = brewing, 3 = done
unsigned long brewStart = 0;
int potionIngr[3], potionCount = 0;  // the ingredients in the cauldron (same order as the web page)
unsigned long lastAddAt = 0;         // when the newest one was dropped in

// 10x10 pictures of the ingredients for the tiny screen (# = a lit dot)
const char* const ICONS[8][10] = {
  {".........#", "........##", ".......##.", "......###.", ".....###..", "....####..", "...####...", "..#####...", ".#####....", "######...."},  // unicorn horn
  {".##....##.", "#..#..#..#", "#.##..##.#", ".########.", "##########", "#........#", "#.######.#", "##......##", ".########.", ".........."},  // frog
  {"..######..", ".########.", "##......##", "#..####..#", "#.######.#", "#.#....#.#", "#.#.##.#.#", "#.#.##.#.#", "..........", ".........."},  // rainbow
  {"#..#..#..#", ".#.#..#.#.", "..#.##.#..", "##.####.##", "..######..", "##.####.##", "..#.##.#..", ".#......#.", "#........#", ".........."},  // spider
  {"..####....", ".#....#...", "#..##..#..", "#.#..#.#..", "#..##..#..", ".#....#...", "..####....", "....#.....", "....#.....", "....#....."},  // candy
  {"........#.", ".......#..", "......###.", ".....####.", "....####..", "...####...", "..####....", ".####.....", "###.......", "#........."},  // dragon pepper
  {".......##.", "......#.##", ".....#.###", "....#.###.", "...#.###..", "..#.###...", "..####....", ".#........", "#.........", ".........."},  // owl feather
  {"....##....", "....##....", "...####...", "##########", ".########.", "..######..", "..##..##..", ".##....##.", ".#......#.", ".........."}   // star dust
};

void startBrew() {
  if (potionState != 1) return;
  potionState = 2;
  brewStart = millis();
}

void startSort() {
  if (hatStart && millis() - hatStart < HAT_THINK_MS + 1500) return;  // still busy
  int roll = esp_random() % 100;
  hatHouse = roll < GRYFFINDOR_CHANCE ? 0 : 1 + (roll - GRYFFINDOR_CHANCE) * 3 / (100 - GRYFFINDOR_CHANCE);
  unsigned long now = millis();
  hatStart = now ? now : 1;
}    // last time the snitch was in the box
#define CATCH_GRACE_MS 2000           // Megaspy's rule: in the box 2 seconds ago still counts!
#define BOX 8                         // how far from the middle still counts as "in the box"
volatile unsigned long owlStart = 0;  // 0 = no letter yet
// Sparkle writes back: one of the kind notes in kind_notes.h
char owlReply[48] = "";
volatile unsigned long owlReplyAt = 0;  // when Sparkle last sent a letter back (0 = never)
bool owlReplied = true;                 // already answered the latest letter?

void sendReply() {
  strncpy(owlReply, KIND_NOTES[esp_random() % KIND_COUNT], sizeof(owlReply) - 1);
  unsigned long now = millis();
  owlReplyAt = now ? now : 1;
  owlReplied = true;
}

// Snitch scoreboard: the yellow screen runs the game, Sparkle hears the claps and shows the score
#define SNITCH_WIN 10
volatile int clapCount = 0;
int boardScore = 0, boardBest = 0;
unsigned long boardChangedAt = 0;

bool dancing() { return danceStart && millis() - danceStart < DANCE_MS; }
void startDance() {
  if (dancing()) return;
  unsigned long now = millis();
  danceStart = now ? now : 1;
  danceCount++;
}
char screenMsg[25] = "";
char sdResult[8] = "?";

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

const char BANNER[] = R"ART(
  ____                   _    _                *
 / ___| _ __   __ _ _ __| | _| | ___       /\  |  /\
 \___ \| '_ \ / _` | '__| |/ / |/ _ \     / (o   o) \
  ___) | |_) | (_| | |  |   <| |  __/      \  ^  /
 |____/| .__/ \__,_|_|  |_|\_\_|\___|       '---'
       |_|  ~ test ~
)ART";

// ---------------------------------------------------------------- LED
void setLed(bool on) {
  ledLit = on;
  digitalWrite(LED_PIN, on ? LOW : HIGH);
}

// "HI" in Morse, as alternating on/off durations in 150 ms units: H = ....  I = ..
const uint8_t HELLO[] = {1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 7};
const int HELLO_STEPS = sizeof(HELLO);

void updateLed() {
  static LedMode lastMode = LED_OFF;
  static unsigned long last = 0, wait = 0;
  static int step = 0;
  if (sdBusy) return;
  unsigned long now = millis();
  if (ledMode != lastMode) {
    lastMode = ledMode;
    step = 0;
    last = now;
    wait = 0;
  }
  switch (ledMode) {
    case LED_OFF: setLed(false); break;
    case LED_ON: setLed(true); break;
    case LED_BLINK: setLed((now / 500) % 2 == 0); break;
    case LED_DISCO:
      if (now - last >= wait) {
        setLed(!ledLit);
        last = now;
        wait = random(40, 160);
      }
      break;
    case LED_HELLO:
      if (now - last >= HELLO[step] * 150UL) {
        last = now;
        step = (step + 1) % HELLO_STEPS;
      }
      setLed(step % 2 == 0);
      break;
    default: break;
  }
}

// ---------------------------------------------------------------- mic
// Start order matters on core 2.0.17: if I2S (the mic) is started after the camera,
// esp_camera_fb_get() times out from then on. setup() starts the mic first.
bool micStart() {
  I2S.setAllPins(-1, 42, 41, -1, -1);
  return I2S.begin(PDM_MONO_MODE, 16000, 16);
}

void micTask(void*) {
  static int16_t buf[800];  // 50 ms at 16 kHz
  int warmup = 10;          // the first ~0.5 s after start is noisy
  for (;;) {
    size_t bytes = 0;
    if (esp_i2s::i2s_read(esp_i2s::I2S_NUM_0, buf, sizeof(buf), &bytes, pdMS_TO_TICKS(200)) != ESP_OK || bytes == 0) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    int n = bytes / sizeof(int16_t);
    // Subtract the mean: the PDM mic has a DC offset that would otherwise dominate the RMS
    double mean = 0;
    for (int i = 0; i < n; i++) mean += buf[i];
    mean /= n;
    double sumSq = 0;
    for (int i = 0; i < n; i++) sumSq += (buf[i] - mean) * (buf[i] - mean);
    int rms = (int)sqrt(sumSq / n);
    if (warmup > 0) {
      warmup--;
      continue;
    }
    soundLevel = rms;
    if (rms > soundPeak) soundPeak = rms;
    // A clap on the home or dance screen starts the Unicorn Dance Party
    if (rms > CLAP_LEVEL && (screenView == V_HOME || screenView == V_DANCE)) startDance();
    if (rms > CLAP_LEVEL && screenView == V_HAT) startSort();
    static unsigned long lastBoardClap = 0;
    if (rms > CLAP_LEVEL && screenView == V_SCORE && millis() - lastBoardClap > 400) {
      lastBoardClap = millis();
      clapCount++;
    }
    // A clap in the snitch game tries to catch it (one try per half second)
    static unsigned long lastClap = 0;
    if (rms > CLAP_LEVEL && screenView == V_SNITCH && millis() - lastClap > 500) {
      lastClap = millis();
      snitchClap = true;
    }
  }
}

// ---------------------------------------------------------------- camera
bool initCamera() {
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = Y2_GPIO_NUM;
  c.pin_d1 = Y3_GPIO_NUM;
  c.pin_d2 = Y4_GPIO_NUM;
  c.pin_d3 = Y5_GPIO_NUM;
  c.pin_d4 = Y6_GPIO_NUM;
  c.pin_d5 = Y7_GPIO_NUM;
  c.pin_d6 = Y8_GPIO_NUM;
  c.pin_d7 = Y9_GPIO_NUM;
  c.pin_xclk = XCLK_GPIO_NUM;
  c.pin_pclk = PCLK_GPIO_NUM;
  c.pin_vsync = VSYNC_GPIO_NUM;
  c.pin_href = HREF_GPIO_NUM;
  c.pin_sscb_sda = SIOD_GPIO_NUM;
  c.pin_sscb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn = PWDN_GPIO_NUM;
  c.pin_reset = RESET_GPIO_NUM;
  c.xclk_freq_hz = 20000000;
  c.frame_size = FRAMESIZE_VGA;
  c.pixel_format = PIXFORMAT_JPEG;
  c.grab_mode = CAMERA_GRAB_LATEST;
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.jpeg_quality = 12;
  c.fb_count = 2;
  if (esp_camera_init(&c) != ESP_OK) return false;

  sensor_t* s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {  // OV3660 (XIAOML Kit) starts flipped and over-saturated
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  return true;
}

// ---------------------------------------------------------------- SD card
// Mounts the card, lists it, writes a note and reads it back, then hands GPIO21 back to the LED.
String runSdTest() {
  sdBusy = true;
  delay(5);
  String j;
  if (!SD.begin(SD_CS)) {
    j = "{\"ok\":false,\"message\":\"I can't find a memory card. Is it pushed all the way in?\"}";
    strcpy(sdResult, "none");
  } else {
    uint8_t type = SD.cardType();
    const char* typeName = type == CARD_MMC ? "MMC" : type == CARD_SD ? "SD" : type == CARD_SDHC ? "SDHC" : "unknown";
    unsigned long mb = SD.cardSize() / (1024 * 1024);

    String files = "[";
    int count = 0;
    File root = SD.open("/");
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
      if (count < 12) {
        if (count) files += ",";
        files += "\"" + jsonEscape(f.name()) + (f.isDirectory() ? "/" : "") + "\"";
      }
      count++;
    }
    root.close();
    files += "]";

    String note = String(ROBOT_NAME) + " was here! (awake for " + String(millis() / 1000) + " seconds)";
    String back;
    File w = SD.open("/petbot_note.txt", FILE_WRITE);
    bool wrote = w && w.print(note) == note.length();
    if (w) w.close();
    File r = SD.open("/petbot_note.txt");
    if (r) {
      back = r.readString();
      r.close();
    }
    bool ok = wrote && back == note;
    strcpy(sdResult, ok ? "OK!" : "oops");

    j = String("{\"ok\":") + (ok ? "true" : "false") + ",\"type\":\"" + typeName + "\",\"sizeMB\":" + mb +
        ",\"count\":" + count + ",\"files\":" + files + ",\"note\":\"" + jsonEscape(back) + "\"}";
  }
  SD.end();
  pinMode(LED_PIN, OUTPUT);
  sdBusy = false;
  return j;
}

// ---------------------------------------------------------------- screen
void drawCentered(const char* s, int y) {
  int w = u8g2.getStrWidth(s);
  u8g2.drawStr(37 + (35 - w) / 2, y, s);
}

void drawSparkle(int x, int y, int r) {
  u8g2.drawHLine(x - r, y, 2 * r + 1);
  u8g2.drawVLine(x, y - r, 2 * r + 1);
  if (r > 1) {
    u8g2.drawPixel(x - 1, y - 1);
    u8g2.drawPixel(x + 1, y - 1);
    u8g2.drawPixel(x - 1, y + 1);
    u8g2.drawPixel(x + 1, y + 1);
  }
}

void drawHeart(int x, int y, int r) {
  u8g2.drawDisc(x - r, y, r);
  u8g2.drawDisc(x + r, y, r);
  u8g2.drawTriangle(x - 2 * r - 1, y + 1, x + 2 * r + 1, y + 1, x, y + 2 * r + 3);
}

// Loudness 0..1 on a log scale (quiet room ~20, clap ~several thousand)
float loudness(int level) {
  float v = (log10f(max(level, 10)) - 1.0f) / 2.8f;
  return v < 0 ? 0 : v > 1 ? 1 : v;
}

// The unicorn bitmap, drawn pixel by pixel so it can face either way
void drawUnicorn(int x0, int y0, bool flip) {
  const int stride = (UNICORN_W + 7) / 8;
  for (int y = 0; y < UNICORN_H; y++)
    for (int x = 0; x < UNICORN_W; x++)
      if (UNICORN_BITS[y * stride + x / 8] & (1 << (x % 8)))
        u8g2.drawPixel(x0 + (flip ? UNICORN_W - 1 - x : x), y0 + y);
}

// Heart Twirl: the unicorn sways and turns around while lots of hearts float up and wobble
void drawHeartTwirl(unsigned long t) {
  int x = 4 + (int)lroundf(sinf(t / 1000.0f * 1.5f * 2 * PI) * 3);
  drawUnicorn(x, 0, (t / 750) % 2 == 1);
  for (int i = 0; i < HEARTS; i++) {
    float period = 1200 + (i * 370) % 1000;  // each heart floats at its own speed
    float p = fmodf(t / period + i / (float)HEARTS, 1.0f);
    int hx = 41 + (i * 11) % 28 + (int)lroundf(sinf(p * 4 * PI + i) * 2);
    drawHeart(hx, 44 - (int)(p * 50), i % 3 == 0 ? 2 : 1);
  }
}

void drawOwl(int x, int y, bool flap) {
  u8g2.drawDisc(x, y, 6);
  u8g2.setDrawColor(0);
  u8g2.drawDisc(x - 2, y - 2, 2);
  u8g2.drawDisc(x + 2, y - 2, 2);
  u8g2.drawPixel(x, y + 1);  // beak
  u8g2.setDrawColor(1);
  u8g2.drawPixel(x - 2, y - 2);
  u8g2.drawPixel(x + 2, y - 2);
  u8g2.drawLine(x - 6, y - 7, x - 3, y - 5);  // ear tufts
  u8g2.drawLine(x + 6, y - 7, x + 3, y - 5);
  u8g2.drawLine(x - 6, y, x - 12, y + (flap ? -5 : 4));  // wings
  u8g2.drawLine(x + 6, y, x + 12, y + (flap ? -5 : 4));
}

void drawEnvelope(int x, int y, int w, int h) {
  u8g2.drawFrame(x, y, w, h);
  u8g2.drawLine(x, y, x + w / 2, y + h / 2);
  u8g2.drawLine(x + w - 1, y, x + w / 2, y + h / 2);
}

// Up to 2 lines of 12 letters across the whole screen, split at a space if we can
void drawLetter(const char* msg) {
  u8g2.setFont(u8g2_font_6x10_tr);
  char a[13] = "", b[13] = "";
  int len = strlen(msg), cut = len;
  if (len > 12) {
    cut = 12;
    for (int i = 12; i > 0; i--)
      if (msg[i] == ' ') { cut = i; break; }
  }
  strncpy(a, msg, min(cut, 12));
  a[min(cut, 12)] = 0;
  const char* rest = msg + cut + (msg[cut] == ' ' ? 1 : 0);
  strncpy(b, rest, 12);
  b[12] = 0;
  int y = b[0] ? 18 : 24;
  u8g2.drawStr((72 - u8g2.getStrWidth(a)) / 2, y, a);
  if (b[0]) u8g2.drawStr((72 - u8g2.getStrWidth(b)) / 2, y + 13, b);
}

void drawOwlPost(unsigned long now) {
  bool flap = (now / 120) % 2;
  if (owlReplyAt && now - owlReplyAt < 2500) {  // Sparkle's owl flies off with a letter for you
    unsigned long t = now - owlReplyAt;
    int x = 20 + (int)(t * 70 / 2500), y = 18 + (int)(sinf(t / 120.0f) * 3);
    drawOwl(x, y, flap);
    drawEnvelope(x - 4, y + 9, 9, 6);
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(0, 8, "for you!");
    return;
  }
  if (!owlStart) {  // waiting for a letter: the owl sits and blinks
    drawOwl(16, 22, false);
    u8g2.setFont(u8g2_font_6x10_tr);
    drawCentered("owl", 16);
    drawCentered("post", 28);
    return;
  }
  unsigned long t = now - owlStart;
  if (t < OWL_FLY_MS) {  // flying in with the letter
    int x = -14 + (int)(t * 50 / OWL_FLY_MS), y = 14 + (int)(sinf(t / 120.0f) * 3);
    drawOwl(x, y, flap);
    drawEnvelope(x - 4, y + 9, 9, 6);
    drawCentered("hoot!", 36);
  } else if (t < OWL_FLY_MS + 1500) {  // landed: "post for you!"
    drawOwl(14, 20, false);
    drawEnvelope(40, 6, 28, 18);
    u8g2.setFont(u8g2_font_5x8_tr);
    drawCentered("for you!", 34);
  } else {
    drawLetter(owlMsg);
    if (!owlReplied && t > OWL_FLY_MS + 6000) sendReply();  // Sparkle writes back after reading your letter
  }
}

bool snitchInBox() { return abs(snitchX - 36) <= BOX && abs(snitchY - 20) <= BOX; }

void snitchTry() {
  snitchResultAt = millis();
  snitchGot = snitchInBox() || (snitchLastInBox && snitchResultAt - snitchLastInBox <= CATCH_GRACE_MS);
  if (snitchGot) {
    snitchLastInBox = 0;  // one catch per visit to the box
    snitchScore++;
    if (snitchScore > snitchBest) snitchBest = snitchScore;
  }
}

void drawSnitch(unsigned long now) {
  static unsigned long last = now;
  unsigned long since = now - snitchResultAt;
  if (!(snitchGot && snitchResultAt && since < 1200)) {  // it stops while you celebrate a catch
    float speed = 1.0f + min(snitchScore, 12) * 0.15f;  // faster after every catch!
    if (snitchInBox()) speed *= 0.3f;                    // ...but it hovers in the box
    snitchPhase += (now - last) * speed;
  }
  last = now;
  snitchX = 36 + (int)lroundf(sinf(snitchPhase / 430.0f) * 30);
  snitchY = 20 + (int)lroundf(sinf(snitchPhase / 270.0f) * 15);
  if (snitchInBox()) snitchLastInBox = now;

  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.setCursor(0, 7);
  u8g2.print(snitchScore);
  if (snitchGot && snitchResultAt && since < 1200) {  // caught it!
    int r = 2 + since / 150;
    drawSparkle(36, 20, min(r, 8));
    drawSparkle(12, 10, r / 2);
    drawSparkle(60, 30, r / 2);
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(15, 38, "GOT IT!");
    return;
  }
  u8g2.drawFrame(36 - BOX - 1, 20 - BOX - 1, 2 * BOX + 3, 2 * BOX + 3);
  bool flap = (now / 80) % 2;
  u8g2.drawDisc(snitchX, snitchY, 2);
  u8g2.drawLine(snitchX - 3, snitchY, snitchX - 8, snitchY + (flap ? -3 : 2));
  u8g2.drawLine(snitchX + 3, snitchY, snitchX + 8, snitchY + (flap ? -3 : 2));
  if (snitchResultAt && !snitchGot && since < 600) u8g2.drawStr(26, 39, "miss!");
  else if (snitchInBox()) u8g2.drawStr(28, 39, "NOW!");
}

void drawHat(int dx, bool talk) {
  for (int y = 6; y < 32; y++) {
    float w = (y - 6) * 0.55f + 1;
    int x = 18 - (int)w + (y < 14 ? (int)((14 - y) * 0.6f) : 0) + dx;
    u8g2.drawHLine(x, y, (int)(w * 2));
  }
  u8g2.drawBox(4 + dx, 31, 30, 3);  // brim
  u8g2.setDrawColor(0);
  u8g2.drawDisc(14 + dx, 22, 1);  // eyes
  u8g2.drawDisc(22 + dx, 22, 1);
  if (talk) u8g2.drawBox(14 + dx, 26, 9, 3);
  else u8g2.drawHLine(13 + dx, 27, 11);
  u8g2.setDrawColor(1);
}

void drawSortingHat(unsigned long now) {
  u8g2.setFont(u8g2_font_6x10_tr);
  if (!hatStart) {  // waiting
    drawHat(0, false);
    drawCentered("shake", 16);
    drawCentered("me!", 28);
    return;
  }
  unsigned long t = now - hatStart;
  if (t < HAT_THINK_MS) {  // thinking...
    const char* words[4] = {"hmm...", "tricky", "brave!", "hmm..."};
    drawHat((t / 200) % 2 ? 1 : -1, (t / 150) % 2);
    drawCentered(words[(t / 900) % 4], 22);
    return;
  }
  // the answer!
  u8g2.clearBuffer();
  int k = (t / 250) % 4;
  drawSparkle(6, 6, k == 0 ? 3 : 1);
  drawSparkle(66, 8, k == 2 ? 3 : 1);
  drawSparkle(64, 34, k == 1 ? 2 : 1);
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr((72 - u8g2.getStrWidth(HOUSES[hatHouse])) / 2, 20, HOUSES[hatHouse]);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr((72 - u8g2.getStrWidth(ANIMALS[hatHouse])) / 2, 33, ANIMALS[hatHouse]);
}

void drawIcon(int i, int x0, int y0) {
  for (int y = 0; y < 10; y++)
    for (int x = 0; x < 10; x++)
      if (ICONS[i][y][x] == '#') u8g2.drawPixel(x0 + x, y0 + y);
}

void drawWandSpell(unsigned long now) {
  // the wand, pointing up and right, tip at (26, 16)
  u8g2.drawLine(3, 39, 25, 17);
  u8g2.drawLine(4, 39, 26, 17);
  u8g2.drawBox(2, 35, 5, 5);  // handle
  u8g2.setFont(u8g2_font_6x10_tr);
  unsigned long t = now - wandAt;
  if (wandSpell < 0 || t > SPELL_MS) {  // waiting: the tip twinkles
    drawSparkle(27, 15, (now / 300) % 2 ? 2 : 1);
    drawCentered("wave", 18);
    drawCentered("me!", 30);
    return;
  }
  const char* name = SPELLS[wandSpell];
  u8g2.drawStr((72 - u8g2.getStrWidth(name)) / 2, 9, name);
  switch (wandSpell) {
    case 0: {  // LUMOS: a big glowing light
      int r = 4 + (t / 120) % 3;
      u8g2.drawDisc(27, 16, r);
      for (int a = 0; a < 8; a++) {
        float ang = a * 0.785f;
        u8g2.drawLine(27 + cosf(ang) * (r + 3), 16 + sinf(ang) * (r + 3), 27 + cosf(ang) * (r + 7), 16 + sinf(ang) * (r + 7));
      }
      break;
    }
    case 1: {  // ACCIO: a star zooms to the wand
      float p = min(1.0f, t / 1500.0f);
      drawIcon(7, 60 - (int)(p * 38), 26 - (int)(p * 15));
      if (p >= 1.0f) drawSparkle(27, 15, 3);
      break;
    }
    case 2:  // LEVIOSA: a feather floats up and down
      drawIcon(6, 46, 20 + (int)(sinf(t / 300.0f) * 8));
      drawSparkle(27, 15, 1);
      break;
    case 3:  // EXPELLIARMUS: a beam of sparks
      for (int i = 0; i < 6; i++) {
        float p = fmodf(t / 600.0f + i / 6.0f, 1.0f);
        drawSparkle(28 + (int)(p * 44), 15 + (int)(p * 14), p < 0.5f ? 2 : 1);
      }
      break;
    case 4:  // STUPEFY: the whole screen flashes
      drawSparkle(27, 15, 3);
      if (t < 1200 && (t / 150) % 2) {
        u8g2.setDrawColor(2);
        u8g2.drawBox(0, 0, 72, 40);
        u8g2.setDrawColor(1);
      }
      break;
  }
}

// Snitch scoreboard: big score, 10 stars to fill, a party at 10
void drawStar(int cx, int cy, bool filled) {
  if (filled) {
    u8g2.drawTriangle(cx - 3, cy - 1, cx + 3, cy - 1, cx, cy + 3);
    u8g2.drawTriangle(cx, cy - 3, cx - 2, cy + 3, cx + 2, cy + 3);
  } else {
    u8g2.drawPixel(cx, cy - 2);
    u8g2.drawPixel(cx - 2, cy);
    u8g2.drawPixel(cx + 2, cy);
    u8g2.drawPixel(cx, cy + 2);
  }
}

void drawScoreboard(unsigned long now) {
  unsigned long since = now - boardChangedAt;
  if (boardScore >= SNITCH_WIN) {  // WINNER!
    int k = (now / 200) % 4;
    drawSparkle(8, 8, k == 0 ? 4 : 1);
    drawSparkle(64, 10, k == 1 ? 4 : 1);
    drawSparkle(10, 32, k == 2 ? 4 : 1);
    drawSparkle(62, 32, k == 3 ? 4 : 1);
    u8g2.setFont(u8g2_font_helvB12_tr);
    u8g2.drawStr((72 - u8g2.getStrWidth("WIN!")) / 2, 26, "WIN!");
    return;
  }
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", boardScore);
  u8g2.setFont(u8g2_font_helvB18_tn);
  int w = u8g2.getStrWidth(buf);
  u8g2.drawStr(18 - w / 2, 24, buf);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(40, 10, "of 10");
  u8g2.drawStr(40, 22, boardChangedAt && since < 1200 ? "GOT IT" : "clap!");
  for (int i = 0; i < SNITCH_WIN; i++) drawStar(4 + i * 7, 34, i < boardScore);
  if (boardChangedAt && since < 600) drawSparkle(18, 12, 2 + since / 150);
}

void drawCauldron(int wobble, int bubbleSpeed, unsigned long now) {
  u8g2.drawBox(6 + wobble, 22, 28, 2);  // rim
  for (int y = 24; y < 38; y++) {
    int w = (int)sqrtf(max(0, 14 * 14 - (y - 24) * (y - 24)));
    u8g2.drawHLine(20 - w + wobble, y, w * 2);
  }
  u8g2.drawBox(10 + wobble, 38, 3, 2);  // little feet
  u8g2.drawBox(27 + wobble, 38, 3, 2);
  for (int i = 0; i < 5; i++) {  // bubbles rising
    float p = fmodf(now / (float)bubbleSpeed + i / 5.0f, 1.0f);
    if (p < 0.7f) u8g2.drawCircle(12 + i * 4 + wobble, 20 - (int)(p * 18), 1 + (i % 2));
  }
}

void drawPotion(unsigned long now) {
  u8g2.setFont(u8g2_font_6x10_tr);
  if (potionState == 3) {  // the silly result!
    int k = (now / 250) % 4;
    drawSparkle(4, 4, k == 0 ? 3 : 1);
    drawSparkle(67, 36, k == 2 ? 3 : 1);
    drawLetter(potionMsg);
    return;
  }
  if (potionState == 2) {  // brewing: it wobbles and bubbles fast
    unsigned long t = now - brewStart;
    drawCauldron((t / 100) % 2 ? 1 : -1, 500, now);
    drawCentered((t / 400) % 2 ? "bloop" : "blip!", 22);
    if (t > BREW_MS) potionState = 3;
    return;
  }
  drawCauldron(0, 1800, now);
  unsigned long since = now - lastAddAt;
  bool dropping = potionCount && since < 700;
  for (int k = 0; k < potionCount; k++) {
    if (dropping && k == potionCount - 1) {  // the newest one falls into the cauldron... splash!
      if (since < 500) drawIcon(potionIngr[k], 15, -10 + (int)(since * 30 / 500));
      else { drawSparkle(11, 18, 2); drawSparkle(29, 18, 2); }
    } else {
      drawIcon(potionIngr[k], 38 + k * 11, 1);
    }
  }
  u8g2.setFont(u8g2_font_5x8_tr);
  if (potionState == 1) {
    drawCentered("shake", 26);
    drawCentered("to stir!", 36);
  } else {
    char more[8];
    snprintf(more, sizeof(more), "add %d", 3 - potionCount);
    drawCentered(more, 26);
    drawCentered("more!", 36);
  }
}

void updateScreen() {
  static unsigned long last = 0;
  unsigned long now = millis();
  if (!screenOk || now - last < 50) return;
  last = now;

  u8g2.clearBuffer();
  if (dancing()) {
    drawHeartTwirl(now - danceStart);
    u8g2.sendBuffer();
    return;
  }
  if (screenView == V_SCORE) {
    drawScoreboard(now);
    u8g2.sendBuffer();
    return;
  }
  if (screenView == V_WAND) {
    drawWandSpell(now);
    u8g2.sendBuffer();
    return;
  }
  if (screenView == V_POTION) {
    drawPotion(now);
    u8g2.sendBuffer();
    return;
  }
  if (screenView == V_HAT) {
    drawSortingHat(now);
    u8g2.sendBuffer();
    return;
  }
  if (screenView == V_SNITCH) {
    drawSnitch(now);
    u8g2.sendBuffer();
    return;
  }
  if (screenView == V_OWL) {
    drawOwlPost(now);
    u8g2.sendBuffer();
    return;
  }
  bool blink = (now % 4000) < 160;
  u8g2.drawXBMP(0, 0, UNICORN_W, UNICORN_H, blink ? UNICORN_BLINK_BITS : UNICORN_BITS);

  // twinkling sparkles around the horn
  int t = (now / 300) % 4;
  drawSparkle(26, 3, t == 0 ? 2 : 1);
  if (t == 2) drawSparkle(35, 8, 2);

  switch (screenView) {
    case V_HOME:
      u8g2.setFont(u8g2_font_helvB10_tr);
      drawCentered("Hi!", 12);
      u8g2.setFont(u8g2_font_5x8_tr);
      drawCentered(ROBOT_NAME, 22);
      drawHeart(54, 28, (now / 400) % 2 ? 2 : 1);
      break;
    case V_LIGHTS:
      u8g2.setFont(u8g2_font_5x8_tr);
      drawCentered("light", 8);
      if (ledLit) {
        u8g2.drawDisc(54, 25, 6);
        for (int a = 0; a < 8; a++) {
          float r = a * 0.785f;
          u8g2.drawLine(54 + cosf(r) * 9, 25 + sinf(r) * 9, 54 + cosf(r) * 12, 25 + sinf(r) * 12);
        }
      } else {
        u8g2.drawCircle(54, 25, 6);
      }
      break;
    case V_SOUNDS: {
      static float shown = 0;
      float v = loudness(soundLevel);
      shown = v > shown ? v : shown * 0.85f + v * 0.15f;
      u8g2.setFont(u8g2_font_6x10_tr);
      const char* word = shown < 0.22f ? "zzz" : shown < 0.5f ? "hi!" : shown < 0.75f ? "yay!" : "WOW!";
      drawCentered(word, 12);
      u8g2.drawRFrame(39, 22, 32, 10, 3);
      int w = (int)(shown * 28);
      if (w > 0) u8g2.drawRBox(41, 24, w, 6, 2);
      break;
    }
    case V_EYES:
      u8g2.setFont(u8g2_font_6x10_tr);
      drawCentered("say", 14);
      drawCentered((now / 1000) % 2 ? "cheese" : "smile!", 28);
      break;
    case V_MEMORY:
      u8g2.setFont(u8g2_font_5x8_tr);
      drawCentered("card", 8);
      u8g2.setFont(u8g2_font_helvB10_tr);
      drawCentered(sdResult, 28);
      break;
    case V_WIFI: {
      u8g2.setFont(u8g2_font_5x8_tr);
      drawCentered("wifi", 8);
      int rssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -100;
      int bars = rssi > -55 ? 4 : rssi > -67 ? 3 : rssi > -75 ? 2 : rssi > -85 ? 1 : 0;
      for (int b = 0; b < 4; b++) {
        int h = 5 + b * 5, x = 42 + b * 7;
        if (b < bars) u8g2.drawBox(x, 36 - h, 5, h);
        else u8g2.drawFrame(x, 36 - h, 5, h);
      }
      break;
    }
    case V_MESSAGE: {
      // up to 2 lines of 6 characters
      u8g2.setFont(u8g2_font_6x10_tr);
      char line[7];
      int len = strlen(screenMsg);
      for (int row = 0; row < 2 && row * 6 < len; row++) {
        strncpy(line, screenMsg + row * 6, 6);
        line[6] = 0;
        drawCentered(line, 14 + row * 12);
      }
      break;
    }
    case V_DANCE:
      u8g2.setFont(u8g2_font_6x10_tr);
      drawCentered("clap", 14);
      drawCentered("me!", 26);
      break;
    default: break;
  }
  u8g2.sendBuffer();
}

// ---------------------------------------------------------------- web
String jsonEscape(const String& s) {
  String out;
  for (char c : s) {
    if (c == '"' || c == '\\') out += '\\';
    if ((uint8_t)c >= 32) out += c;
  }
  return out;
}

// Decodes %XX and '+' in place.
void urlDecode(char* s) {
  char* o = s;
  for (; *s; s++) {
    if (*s == '+') *o++ = ' ';
    else if (*s == '%' && isxdigit(s[1]) && isxdigit(s[2])) {
      char hex[3] = {s[1], s[2], 0};
      *o++ = (char)strtol(hex, nullptr, 16);
      s += 2;
    } else *o++ = *s;
  }
  *o = 0;
}

bool queryParam(httpd_req_t* req, const char* key, char* out, size_t len) {
  char query[96];
  out[0] = 0;
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) return false;
  if (httpd_query_key_value(query, key, out, len) != ESP_OK) return false;
  urlDecode(out);
  return true;
}

esp_err_t sendJson(httpd_req_t* req, const String& body) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, body.c_str(), body.length());
}

esp_err_t indexHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, PAGE_HTML, HTTPD_RESP_USE_STRLEN);
}

esp_err_t infoHandler(httpd_req_t* req) {
  String j = String("{\"ssid\":\"") + jsonEscape(WiFi.SSID()) + "\",\"ip\":\"" + WiFi.localIP().toString() +
             "\",\"rssi\":" + WiFi.RSSI() + ",\"channel\":" + WiFi.channel() +
             ",\"camera\":" + (cameraOk ? "true" : "false") + ",\"mic\":" + (micOk ? "true" : "false") +
             ",\"screen\":" + (screenOk ? "true" : "false") + ",\"led\":\"" + LED_MODE_NAMES[ledMode] +
             "\",\"uptime\":" + (millis() / 1000) + ",\"kid\":\"" + jsonEscape(KID_NAME) + "\"}";
  return sendJson(req, j);
}

esp_err_t ledHandler(httpd_req_t* req) {
  char mode[12];
  if (queryParam(req, "mode", mode, sizeof(mode))) {
    for (int i = 0; i < LED_MODES; i++)
      if (!strcmp(mode, LED_MODE_NAMES[i])) ledMode = (LedMode)i;
  }
  return sendJson(req, String("{\"mode\":\"") + LED_MODE_NAMES[ledMode] + "\"}");
}

esp_err_t soundHandler(httpd_req_t* req) {
  int peak = soundPeak;
  soundPeak = soundLevel;
  return sendJson(req, String("{\"level\":") + soundLevel + ",\"peak\":" + peak + "}");
}

esp_err_t sdHandler(httpd_req_t* req) {
  return sendJson(req, runSdTest());
}

esp_err_t screenHandler(httpd_req_t* req) {
  char view[12], msg[40];
  if (queryParam(req, "msg", msg, sizeof(msg))) {
    int n = 0;
    for (char* p = msg; *p && n < 12; p++)
      if (*p >= 32 && *p < 127) screenMsg[n++] = *p;
    screenMsg[n] = 0;
    screenView = V_MESSAGE;
  } else if (queryParam(req, "view", view, sizeof(view))) {
    for (int i = 0; i < V_COUNT; i++)
      if (!strcmp(view, VIEW_NAMES[i])) screenView = (ScreenView)i;
  }
  return sendJson(req, String("{\"view\":\"") + VIEW_NAMES[screenView] + "\",\"screen\":" + (screenOk ? "true" : "false") + "}");
}

// /api/dance?go=1 starts a dance; either way it says how long is left
esp_err_t danceHandler(httpd_req_t* req) {
  char go[4];
  if (queryParam(req, "go", go, sizeof(go))) startDance();
  unsigned long left = dancing() ? DANCE_MS - (millis() - danceStart) : 0;
  return sendJson(req, String("{\"left\":") + left + ",\"total\":" + DANCE_MS + ",\"count\":" + danceCount + "}");
}

// /api/owl?msg=HELLO sends a letter by owl
esp_err_t owlHandler(httpd_req_t* req) {
  char msg[80];
  if (queryParam(req, "msg", msg, sizeof(msg))) {
    int n = 0;
    for (char* p = msg; *p && n < 24; p++)
      if (*p >= 32 && *p < 127) owlMsg[n++] = *p;
    owlMsg[n] = 0;
    owlStart = millis() ? millis() : 1;
    owlReplied = false;
    screenView = V_OWL;
  }
  return sendJson(req, String("{\"fly\":") + OWL_FLY_MS + ",\"reply\":\"" + jsonEscape(owlReply) +
                           "\",\"replyAt\":" + owlReplyAt + "}");
}

// /api/snitch?catch=1 tries a catch, ?reset=1 starts a new game; always returns the score
esp_err_t snitchHandler(httpd_req_t* req) {
  char v[4];
  if (queryParam(req, "catch", v, sizeof(v))) snitchClap = true;
  if (queryParam(req, "reset", v, sizeof(v))) {
    snitchScore = 0;
    snitchResultAt = 0;
  }
  return sendJson(req, String("{\"score\":") + snitchScore + ",\"best\":" + snitchBest + "}");
}

// /api/hat?go=1 starts sorting; returns "thinking", "done" (with the house) or "waiting"
esp_err_t hatHandler(httpd_req_t* req) {
  char v[4];
  if (queryParam(req, "go", v, sizeof(v))) startSort();
  const char* state = !hatStart ? "waiting" : millis() - hatStart < HAT_THINK_MS ? "thinking" : "done";
  return sendJson(req, String("{\"state\":\"") + state + "\",\"house\":" + hatHouse + ",\"start\":" + hatStart + "}");
}

// /api/potion?msg=... puts the ingredients in, ?stir=1 brews, ?empty=1 starts again; returns the state
esp_err_t potionHandler(httpd_req_t* req) {
  char msg[80], v[4];
  if (queryParam(req, "ing", msg, sizeof(msg))) {  // e.g. ing=0,3,5
    int old = potionCount;
    potionCount = 0;
    for (char* p = msg; *p && potionCount < 3; p++)
      if (*p >= '0' && *p <= '7') potionIngr[potionCount++] = *p - '0';
    if (potionCount > old) lastAddAt = millis();
    potionState = 0;
    screenView = V_POTION;
  }
  if (queryParam(req, "msg", msg, sizeof(msg))) {
    int n = 0;
    for (char* p = msg; *p && n < 24; p++)
      if (*p >= 32 && *p < 127) potionMsg[n++] = *p;
    potionMsg[n] = 0;
    if (potionCount == 3) potionState = 1;
    screenView = V_POTION;
  }
  if (queryParam(req, "stir", v, sizeof(v))) startBrew();
  if (queryParam(req, "empty", v, sizeof(v))) {
    potionState = 0;
    potionCount = 0;
  }
  return sendJson(req, String("{\"state\":") + potionState + "}");
}

// /api/wand?cast=1 casts a spell; returns the latest spell (or -1) and when it was cast
esp_err_t wandHandler(httpd_req_t* req) {
  char v[4];
  if (queryParam(req, "cast", v, sizeof(v))) castSpell();
  return sendJson(req, String("{\"spell\":") + wandSpell + ",\"at\":" + wandAt + "}");
}

// /api/score?score=N&best=M shows the yellow screen's snitch score; always returns how many claps Sparkle heard
esp_err_t scoreHandler(httpd_req_t* req) {
  char v[8];
  if (queryParam(req, "score", v, sizeof(v))) {
    int s = atoi(v);
    if (s != boardScore) boardChangedAt = millis();
    boardScore = s;
    screenView = V_SCORE;
  }
  if (queryParam(req, "best", v, sizeof(v))) boardBest = atoi(v);
  return sendJson(req, String("{\"claps\":") + clapCount + ",\"score\":" + boardScore + "}");
}

esp_err_t captureHandler(httpd_req_t* req) {
  camera_fb_t* fb = cameraOk ? esp_camera_fb_get() : nullptr;
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=petbot.jpg");
  esp_err_t res = httpd_resp_send(req, (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

#define PART_BOUNDARY "petbotframe"
esp_err_t streamHandler(httpd_req_t* req) {
  if (!cameraOk) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=" PART_BOUNDARY);
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  char part[64];
  for (;;) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) return ESP_FAIL;
    size_t hlen = snprintf(part, sizeof(part), "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    esp_err_t res = httpd_resp_send_chunk(req, "\r\n--" PART_BOUNDARY "\r\n", strlen("\r\n--" PART_BOUNDARY "\r\n"));
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, part, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    if (res != ESP_OK) return res;  // browser left the Eyes page
  }
}

void startWeb() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.max_uri_handlers = 16;
  cfg.stack_size = 8192;
  httpd_handle_t web = nullptr;
  if (httpd_start(&web, &cfg) == ESP_OK) {
    httpd_uri_t uris[] = {
      {"/", HTTP_GET, indexHandler, nullptr},
      {"/api/info", HTTP_GET, infoHandler, nullptr},
      {"/api/led", HTTP_GET, ledHandler, nullptr},
      {"/api/sound", HTTP_GET, soundHandler, nullptr},
      {"/api/sd", HTTP_GET, sdHandler, nullptr},
      {"/api/screen", HTTP_GET, screenHandler, nullptr},
      {"/api/dance", HTTP_GET, danceHandler, nullptr},
      {"/api/owl", HTTP_GET, owlHandler, nullptr},
      {"/api/snitch", HTTP_GET, snitchHandler, nullptr},
      {"/api/hat", HTTP_GET, hatHandler, nullptr},
      {"/api/potion", HTTP_GET, potionHandler, nullptr},
      {"/api/wand", HTTP_GET, wandHandler, nullptr},
      {"/api/score", HTTP_GET, scoreHandler, nullptr},
      {"/capture", HTTP_GET, captureHandler, nullptr},
    };
    for (auto& u : uris) httpd_register_uri_handler(web, &u);
  }

  // The camera stream gets its own server so it doesn't block the menu
  cfg.server_port = 81;
  cfg.ctrl_port = 32769;
  httpd_handle_t stream = nullptr;
  if (httpd_start(&stream, &cfg) == ESP_OK) {
    httpd_uri_t s = {"/stream", HTTP_GET, streamHandler, nullptr};
    httpd_register_uri_handler(stream, &s);
  }
}

// ---------------------------------------------------------------- serial menu
void printMenu() {
  Serial.println(BANNER);
  Serial.printf("  camera %s   mic %s   screen %s\n\n", cameraOk ? "OK" : "--", micOk ? "OK" : "--", screenOk ? "OK" : "--");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("  Wi-Fi not connected (\"%s\"). Check secrets.h, and that it's a 2.4 GHz network.\n", WIFI_SSID);
    return;
  }
  String base = "http://" + WiFi.localIP().toString();
  Serial.println("  " ROBOT_NAME " is awake! Click a link to start a test:\n");
  Serial.printf("    Menu     %s/\n", base.c_str());
  Serial.printf("    Lights   %s/#lights\n", base.c_str());
  Serial.printf("    Sounds   %s/#sounds\n", base.c_str());
  Serial.printf("    Eyes     %s/#eyes\n", base.c_str());
  Serial.printf("    Memory   %s/#memory\n", base.c_str());
  Serial.printf("    Wi-Fi    %s/#wifi\n", base.c_str());
  Serial.printf("    Screen   %s/#screen\n", base.c_str());
  Serial.println("\n    (http://" ROBOT_HOST ".local/ works too on most computers)");
  Serial.println("  Press Enter here to show this menu again.\n");
}

// ---------------------------------------------------------------- main
void setup() {
  Serial.begin(115200);
  delay(2000);  // give the USB serial time to come up
  Serial.println(ROBOT_NAME " is starting...");

  pinMode(LED_PIN, OUTPUT);
  setLed(false);

  Wire.begin();
  Wire.beginTransmission(OLED_ADDR);
  screenOk = Wire.endTransmission() == 0;
  if (screenOk) {
    u8g2.begin();
    u8g2.setBusClock(400000);
    imuOk = imuInit();
    updateScreen();
  }

  micOk = micStart();  // must come before the camera, see micStart()
  cameraOk = initCamera();
  if (micOk) xTaskCreatePinnedToCore(micTask, "mic", 4096, nullptr, 1, nullptr, 1);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(ROBOT_HOST);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
    updateScreen();
  }
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.setSleep(false);  // snappier web page and camera stream
    MDNS.begin(ROBOT_HOST);
    MDNS.addService("http", "tcp", 80);
    startWeb();
  }
  printMenu();
}

void loop() {
  if (snitchClap) {
    snitchClap = false;
    snitchTry();
  }
  // Shake on the Sorting Hat: sorts when the hat is waiting, and goes back to the hat when a house is showing
  bool shake = shaken();
  // Wand Wave: one big swish casts a spell; Lumos turns the light off again when its magic ends
  if (screenView == V_WAND && fabsf(imuG - 1.0f) > WAVE_G) castSpell();
  if (wandSpell == 0 && ledMode == LED_ON && millis() - wandAt > SPELL_MS + 500) ledMode = LED_OFF;
  if (shake && screenView == V_HAT) {
    if (!hatStart) startSort();
    else if (millis() - hatStart > HAT_THINK_MS) hatStart = 0;
  }
  // Shake on the Potion Mixer: stirs the cauldron, or empties it when the potion is done
  if (shake && screenView == V_OWL) sendReply();  // shake Sparkle: it sends you a kind letter
  if (shake && screenView == V_POTION) {
    if (potionState == 1) startBrew();
    else if (potionState == 3) {
      potionState = 0;
      potionCount = 0;
    }
  }
  updateLed();
  updateScreen();
  if (Serial.available()) {
    while (Serial.available()) Serial.read();
    printMenu();
  }
  delay(5);
}
