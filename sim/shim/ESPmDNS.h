// Pretend mDNS: Sparkle is always found.
#pragma once
#include <WiFi.h>
struct MDNSShim {
  bool begin(const char*) { return true; }
  IPAddress queryHost(const char*, int) { return IPAddress(192, 168, 1, 42); }
};
extern MDNSShim MDNS;
