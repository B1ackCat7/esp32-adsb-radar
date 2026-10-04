#pragma once
#include "Arduino.h"
constexpr int WL_CONNECTED = 3;
struct PreviewWiFi { int status() const { return WL_CONNECTED; } };
inline PreviewWiFi WiFi;
