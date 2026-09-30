#pragma once
#include <Arduino.h>
namespace services::station {
void settingsInit();
const String& host();
uint16_t dataPort();
uint16_t webPort();
String dataUrl(const char* path);
String webUrl(const char* path);
/** One NVS entry commits the entire validated receiver connection. */
bool saveConnection(const char* host, uint16_t dataPort, uint16_t webPort);
}  // namespace services::station
