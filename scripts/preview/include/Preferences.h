#pragma once
#include "Arduino.h"
struct Preferences {
  bool begin(const char*, bool) { return true; }
  bool getBool(const char*, bool fallback) { return fallback; }
  size_t putBool(const char*, bool) { return 1; }
  bool remove(const char*) { return true; }
  void end() {}
};
