// Magic Bot Workshop CYD emulator: just enough Arduino to run the CYD sketches on a laptop.
// Time is pretend: millis() is a counter that delay() moves forward, so runs are fast and repeatable.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <string>
#include <algorithm>

#define PROGMEM
#define pgm_read_byte(a) (*(const uint8_t*)(a))
#define pgm_read_word(a) (*(const uint16_t*)(a))
#define pgm_read_dword(a) (*(const uint32_t*)(a))
#define OUTPUT 1
#define INPUT 0
#define HIGH 1
#define LOW 0
#ifndef PI
#define PI 3.14159265358979f
#endif

extern unsigned long g_now;
inline unsigned long millis() { return g_now; }
inline void delay(unsigned long ms) { g_now += ms; }
inline void pinMode(int, int) {}
inline void digitalWrite(int pin, int v);
inline void ledcSetup(int, int, int) {}
inline void ledcAttachPin(int, int) {}
inline void ledcWriteTone(int, int) {}
inline long random(long n) { return n > 0 ? std::rand() % n : 0; }
inline long random(long a, long b) { return b > a ? a + std::rand() % (b - a) : a; }
inline long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }
template <class T, class L, class H> inline T constrain(T x, L lo, H hi) { return x < lo ? lo : x > hi ? hi : x; }
using std::min;
using std::max;
using std::abs;

// The RGB light: the emulator remembers it so a picture can show it
extern int g_led[3];
inline void digitalWrite(int pin, int v) {
  if (pin == 4) g_led[0] = !v;
  if (pin == 16) g_led[1] = !v;
  if (pin == 17) g_led[2] = !v;
}

// ---- Arduino's String, the bits the sketches use
class String {
 public:
  std::string s;
  String() {}
  String(const char* c) : s(c ? c : "") {}
  String(const std::string& x) : s(x) {}
  String(char c) : s(1, c) {}
  String(int v) : s(std::to_string(v)) {}
  String(long v) : s(std::to_string(v)) {}
  String(unsigned int v) : s(std::to_string(v)) {}
  String(unsigned long v) : s(std::to_string(v)) {}
  unsigned length() const { return s.size(); }
  const char* c_str() const { return s.c_str(); }
  int indexOf(const String& x, unsigned from = 0) const {
    size_t i = s.find(x.s, from);
    return i == std::string::npos ? -1 : (int)i;
  }
  int indexOf(char c, unsigned from = 0) const {
    size_t i = s.find(c, from);
    return i == std::string::npos ? -1 : (int)i;
  }
  String substring(unsigned a) const { return a >= s.size() ? String() : String(s.substr(a)); }
  String substring(unsigned a, unsigned b) const {
    if (a >= s.size() || b <= a) return String();
    return String(s.substr(a, b - a));
  }
  long toInt() const { return std::strtol(s.c_str(), nullptr, 10); }
  String& operator+=(const String& x) { s += x.s; return *this; }
  String& operator+=(const char* x) { s += x; return *this; }
  String& operator+=(char c) { s += c; return *this; }
  bool operator==(const String& x) const { return s == x.s; }
  bool operator!=(const String& x) const { return s != x.s; }
  bool operator==(const char* x) const { return s == x; }
  bool operator!=(const char* x) const { return s != x; }
};
inline String operator+(const String& a, const String& b) { return String(a.s + b.s); }
inline String operator+(const String& a, const char* b) { return String(a.s + b); }
inline String operator+(const char* a, const String& b) { return String(std::string(a) + b.s); }
inline String operator+(const String& a, char b) { return String(a.s + b); }

struct SerialShim {
  void begin(int) {}
  void println(const String&) {}
  void println(const char*) {}
  template <class... A> void printf(const char*, A...) {}
};
extern SerialShim Serial;
