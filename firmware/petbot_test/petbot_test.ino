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

// Her robot's name (she picked it on pickers/02-robot-name.html). 7 letters max to fit the screen.
#define ROBOT_NAME "Sparkle"
#define ROBOT_HOST "sparkle"  // web address: http://sparkle.local

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

enum ScreenView { V_HOME, V_LIGHTS, V_SOUNDS, V_EYES, V_MEMORY, V_WIFI, V_MESSAGE, V_COUNT };
const char* VIEW_NAMES[V_COUNT] = {"home", "lights", "sounds", "eyes", "memory", "wifi", "message"};
volatile ScreenView screenView = V_HOME;
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

void updateScreen() {
  static unsigned long last = 0;
  unsigned long now = millis();
  if (!screenOk || now - last < 50) return;
  last = now;

  u8g2.clearBuffer();
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
             "\",\"uptime\":" + (millis() / 1000) + "}";
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
  cfg.max_uri_handlers = 12;
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
  updateLed();
  updateScreen();
  if (Serial.available()) {
    while (Serial.available()) Serial.read();
    printMenu();
  }
  delay(5);
}
