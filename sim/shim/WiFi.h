// Pretend Wi-Fi: always connected.
#pragma once
#include <Arduino.h>
#define WIFI_STA 1
#define WL_CONNECTED 3
struct IPAddress {
  uint8_t b[4] = {0, 0, 0, 0};
  IPAddress() {}
  IPAddress(int a, int c, int d, int e) { b[0] = a; b[1] = c; b[2] = d; b[3] = e; }
  bool operator==(const IPAddress& o) const { return !memcmp(b, o.b, 4); }
  String toString() const {
    char t[20];
    snprintf(t, sizeof(t), "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
    return String(t);
  }
};
struct WiFiShim {
  void mode(int) {}
  void begin(const char*, const char*) {}
  int status() { return WL_CONNECTED; }
};
extern WiFiShim WiFi;
struct WiFiClient {
  int left = 0;
  int available() { return left; }
  int readBytes(uint8_t* buf, int n) {
    memset(buf, 0, n);
    left -= n;
    return n;
  }
};
