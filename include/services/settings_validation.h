#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace services::settings {
constexpr uint16_t kDefaultDataPort = 8080;
constexpr uint16_t kDefaultWebPort = 80;

inline std::string trim(const char* raw) {
  const std::string text = raw ? raw : "";
  const auto begin = text.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) return "";
  return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

// Keep requests numeric: this core's DNS lookup can block for 15 seconds.
// Empty is an explicit unconfigured station, never a private default.
inline bool parseHost(const char* raw, std::string& out) {
  const auto text = trim(raw);
  if (text.empty()) { out.clear(); return true; }
  unsigned octets[4] = {};
  size_t pos = 0;
  for (unsigned i = 0; i < 4; ++i) {
    const size_t start = pos;
    while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
      if (pos - start >= 3) return false;
      octets[i] = octets[i] * 10 + text[pos++] - '0';
    }
    if (pos == start || octets[i] > 255) return false;
    if (i < 3 && (pos == text.size() || text[pos++] != '.')) return false;
  }
  if (pos != text.size() || octets[0] == 0 || octets[0] == 127 ||
      octets[0] >= 224) return false;
  char host[16];
  snprintf(host, sizeof(host), "%u.%u.%u.%u", octets[0], octets[1],
           octets[2], octets[3]);
  out = host;
  return true;
}

inline bool parsePort(const char* raw, uint16_t& out) {
  const auto text = trim(raw);
  if (text.empty() || text.size() > 5) return false;
  unsigned port = 0;
  for (char c : text) {
    if (c < '0' || c > '9') return false;
    port = port * 10 + c - '0';
  }
  if (!port || port > 65535) return false;
  out = static_cast<uint16_t>(port);
  return true;
}

inline bool parseCoordinates(const char* latText, const char* lonText,
                             double& lat, double& lon) {
  const auto a = trim(latText), b = trim(lonText);
  char *aEnd = nullptr, *bEnd = nullptr;
  lat = strtod(a.c_str(), &aEnd);
  lon = strtod(b.c_str(), &bEnd);
  return !a.empty() && !b.empty() && *aEnd == '\0' && *bEnd == '\0' &&
         std::isfinite(lat) && std::isfinite(lon) &&
         lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180;
}

inline std::string escapeHtml(const std::string& raw) {
  std::string out;
  for (char c : raw) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += c;
    }
  }
  return out;
}
}  // namespace services::settings
