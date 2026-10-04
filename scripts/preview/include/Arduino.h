#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Preview-only clock and silent serial sink; no hardware or network access.
inline unsigned long millis() { return 100000; }
inline unsigned long micros() { return 100000000; }
struct PreviewSerial {
  template <typename... Args> void printf(const char*, Args...) {}
  void println(const char*) {}
  void print(const char*) {}
  void write(const uint8_t*, size_t) {}
  void flush() {}
};
inline PreviewSerial Serial;
