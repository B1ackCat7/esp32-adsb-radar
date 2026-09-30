#include "services/station_settings.h"

#include <Preferences.h>
#include "services/settings_validation.h"
namespace services::station {
namespace {
String s_host = "";
uint16_t s_data_port = settings::kDefaultDataPort;
uint16_t s_web_port = settings::kDefaultWebPort;
}
void settingsInit() {
  Preferences p;
  if (p.begin("station", true)) {
    // Migrate the old IPv4-only key in memory; do not rewrite on boot.
    const String saved = p.getString("connection", "");
    std::string normalized;
    if (saved.isEmpty()) {
      const String legacy = p.getString("host", "");
      if (settings::parseHost(legacy.c_str(), normalized))
        s_host = normalized.c_str();
    } else {
      const int first = saved.indexOf('|'), second = saved.indexOf('|', first + 1);
      uint16_t data = 0, web = 0;
      if (first >= 0 && second > first &&
          settings::parseHost(saved.substring(0, first).c_str(), normalized) &&
          settings::parsePort(saved.substring(first + 1, second).c_str(), data) &&
          settings::parsePort(saved.substring(second + 1).c_str(), web)) {
        s_host = normalized.c_str(); s_data_port = data; s_web_port = web;
      }
    }
    p.end();
  }
}
const String& host() { return s_host; }
uint16_t dataPort() { return s_data_port; }
uint16_t webPort() { return s_web_port; }
String dataUrl(const char* path) {
  return "http://" + s_host + ":" + String(s_data_port) + path;
}
String webUrl(const char* path) {
  return "http://" + s_host + ":" + String(s_web_port) + path;
}
bool saveConnection(const char* value, uint16_t data, uint16_t web) {
  std::string normalized;
  if (!settings::parseHost(value, normalized) || !data || !web) return false;
  if (s_host == normalized.c_str() && s_data_port == data && s_web_port == web)
    return true;
  Preferences p;
  if (!p.begin("station", false)) return false;
  const String saved = String(normalized.c_str()) + "|" + data + "|" + web;
  const bool ok = p.putString("connection", saved) == saved.length();
  p.end();
  if (ok) { s_host = normalized.c_str(); s_data_port = data; s_web_port = web; }
  return ok;
}
}  // namespace services::station
