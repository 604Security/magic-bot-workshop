#pragma once
#define VSPI 3
#define HSPI 2
struct SPIClass {
  SPIClass(int = 0) {}
  void begin(int = 0, int = 0, int = 0, int = 0) {}
};
