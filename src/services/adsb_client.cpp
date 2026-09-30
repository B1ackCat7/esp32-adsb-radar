#include "services/adsb_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include <algorithm>
#include <cstring>

#include "config.h"
#include "services/aircraft_buffer.h"
#include "services/radar_location.h"
#include "services/station_settings.h"
#include "services/station_status.h"

namespace services::adsb {

namespace {

services::station::Status s_status, s_display_status;
uint32_t s_configuration_generation = 0;
double s_counter_stamp = 0;
double s_counter = 0;
constexpr int kConnectTimeoutMs = 1200;  // Short LAN connect timeout
constexpr int kConnectAttempts =
    1;  // a stalled TLS connect blocks the UI; retry next poll instead
constexpr unsigned long kRequestTimeoutMs = 1500;

Aircraft s_aircraft[kMaxAircraft];
AircraftBuffer<Aircraft, kMaxAircraft> s_buffer;
double s_center_lat = 0, s_center_lon = 0;
float s_radius = 0;
size_t s_aircraft_count = 0;
PollFn s_poll_fn = nullptr;

void pollNetwork() {
  if (s_poll_fn != nullptr) {
    s_poll_fn();
  }
}

bool readJsonFloat(const JsonObject& obj, const char* key, float* out) {
  if (obj[key].is<float>() || obj[key].is<double>() || obj[key].is<int>()) {
    *out = obj[key].as<float>();
    return true;
  }
  return false;
}

float pickNoseHeading(const JsonObject& plane) {
  float v = 0.0f;
  if (readJsonFloat(plane, "true_heading", &v)) {
    return v;
  }
  if (readJsonFloat(plane, "mag_heading", &v)) {
    return v;
  }
  if (readJsonFloat(plane, "track", &v)) {
    return v;
  }
  if (readJsonFloat(plane, "dir", &v)) {
    return v;
  }
  return 0.0f;
}

float pickTrackHeading(const JsonObject& plane) {
  float v = 0.0f;
  if (readJsonFloat(plane, "track", &v)) {
    return v;
  }
  if (readJsonFloat(plane, "true_heading", &v)) {
    return v;
  }
  if (readJsonFloat(plane, "mag_heading", &v)) {
    return v;
  }
  if (readJsonFloat(plane, "dir", &v)) {
    return v;
  }
  return 0.0f;
}

float pickGroundSpeed(const JsonObject& plane) {
  float v = 0.0f;
  if (readJsonFloat(plane, "gs", &v)) {
    return v;
  }
  if (readJsonFloat(plane, "tas", &v)) {
    return v;
  }
  if (readJsonFloat(plane, "ias", &v)) {
    return v;
  }
  return 0.0f;
}

bool isOnGround(const JsonObject& plane) {
  if (!plane["alt_baro"].is<const char*>()) {
    return false;
  }
  return strcmp(plane["alt_baro"].as<const char*>(), "ground") == 0;
}

void copyJsonStringTrimmed(const JsonObject& obj, const char* key, char* out,
                           size_t out_len) {
  out[0] = '\0';
  if (out_len == 0 || !obj[key].is<const char*>()) {
    return;
  }
  const char* s = obj[key].as<const char*>();
  size_t n = strnlen(s, out_len - 1);
  while (n > 0 && s[n - 1] == ' ') {
    --n;
  }
  memcpy(out, s, n);
  out[n] = '\0';
}

void formatAltitudeTag(const JsonObject& plane, char* out, size_t out_len) {
  out[0] = '\0';
  if (out_len == 0) {
    return;
  }

  if (plane["alt_baro"].is<const char*>()) {
    const char* s = plane["alt_baro"].as<const char*>();
    if (strcmp(s, "ground") == 0) {
      strncpy(out, "GND", out_len - 1);
      out[out_len - 1] = '\0';
      return;
    }
  }

  float alt = 0.0f;
  if (readJsonFloat(plane, "alt_baro", &alt) ||
      readJsonFloat(plane, "alt_geom", &alt)) {
    snprintf(out, out_len, "%d ft", static_cast<int>(lroundf(alt)));
  }
}

bool httpGetJson(const String& url, const char* tag, JsonDocument& doc,
                 const JsonDocument& filter) {
  const uint32_t generation = s_configuration_generation;
  WiFiClient client;

  HTTPClient http;
  if (!http.begin(client, url)) {
    Serial.printf("%s: http.begin failed\n", tag);
    return false;
  }

  http.useHTTP10(true);
  http.addHeader("Accept-Encoding", "identity");
  http.setTimeout(kRequestTimeoutMs);
  http.setConnectTimeout(kConnectTimeoutMs);

  int code = 0;
  for (int attempt = 0; attempt < kConnectAttempts; ++attempt) {
    pollNetwork();
    if (generation != s_configuration_generation) { http.end(); return false; }
    code = http.GET();
    if (code > 0) {
      break;
    }
  }
  if (code != HTTP_CODE_OK) {
    Serial.printf("%s: HTTP %d\n", tag, code);
    http.end();
    return false;
  }

  if (http.getSize() > 200000) {
    http.end();
    return false;
  }
  // WiFiClient::setTimeout takes SECONDS on Arduino-ESP32 2.0.14.
  // Use a bounded custom reader instead of Stream's per-byte timeout.
  struct BoundedReader {
    WiFiClient& client;
    uint32_t started = millis(), lastByteAt = started;
    uint32_t bytes = 0;
    uint8_t buffer[1024];
    size_t pos = 0, size = 0;
    const char* stop = "eof";
    explicit BoundedReader(WiFiClient& c) : client(c) {}
    int read() {
      if (pos < size) return buffer[pos++];
      while (uint32_t(millis() - started) < 6000) {
        pollNetwork();
        if (bytes >= 200000) {
          stop = "size limit";
          return -1;
        }
        int available = client.available();
        if (available > 0) {
          size_t count = std::min(size_t(available), sizeof(buffer));
          count = std::min(count, size_t(200000 - bytes));
          int got = client.read(buffer, count);
          if (got > 0) {
            bytes += got;
            lastByteAt = millis();
            size = got;
            pos = 1;
            return buffer[0];
          }
        } else if (!client.connected())
          return -1;
        if (uint32_t(millis() - lastByteAt) >= 1500) {
          stop = "idle timeout";
          return -1;
        }
        delay(1);
      }
      stop = "total timeout";
      return -1;
    }
    size_t readBytes(char* out, size_t count) {
      size_t n = 0;
      while (n < count) {
        const int c = read();
        if (c < 0) break;
        out[n++] = c;
      }
      return n;
    }
  } reader(client);
  const DeserializationError err =
      deserializeJson(doc, reader, DeserializationOption::Filter(filter));
  http.end();
  // Portal saves can happen cooperatively during this read. Never publish
  // a response from the previous receiver/center after a configuration change.
  if (generation != s_configuration_generation) { doc.clear(); return false; }
  if (err) {
    Serial.printf("%s: JSON %s; %s bytes=%u elapsed_ms=%lu rssi=%d\n", tag,
                  err.c_str(), reader.stop, reader.bytes,
                  (unsigned long)(millis() - reader.started), WiFi.RSSI());
    return false;
  }
  return true;
}

void fillTagFields(Aircraft* ac, const JsonObject& plane) {
  copyJsonStringTrimmed(plane, "flight", ac->callsign, sizeof(ac->callsign));
  if (ac->callsign[0] == '\0') {
    copyJsonStringTrimmed(plane, "hex", ac->callsign, sizeof(ac->callsign));
  }

  copyJsonStringTrimmed(plane, "t", ac->type, sizeof(ac->type));
  formatAltitudeTag(plane, ac->alt, sizeof(ac->alt));
}

}  // namespace

void setPollFn(PollFn fn) { s_poll_fn = fn; }

void configurationChanged() {
  ++s_configuration_generation;
  s_buffer.clear();
  s_aircraft_count = 0;
  s_status = {};
  s_display_status = {};
  s_counter_stamp = 0;
  s_counter = 0;
}

size_t aircraftCount() { return s_aircraft_count; }

const Aircraft* aircraftList() { return s_aircraft; }

bool fetchUpdate(double center_lat, double center_lon, float fetch_radius_km) {
  if (s_center_lat != center_lat || s_center_lon != center_lon ||
      s_radius != fetch_radius_km) {
    s_buffer.clear();
    s_aircraft_count = 0;
    s_center_lat = center_lat;
    s_center_lon = center_lon;
    s_radius = fetch_radius_km;
  }
  if (services::station::host().isEmpty() || !services::location::configured()) {
    s_status.feed.reachable = false;
    s_aircraft_count = 0;
    return false;
  }
  JsonDocument filter;
  filter["now"] = true;
  JsonObject f = filter["aircraft"].add<JsonObject>();
  for (const char* key :
       {"lat", "lon", "true_heading", "mag_heading", "track", "dir", "gs",
        "tas", "ias", "alt_baro", "alt_geom", "flight", "hex", "t", "category",
        "type", "seen", "seen_pos"})
    f[key] = true;
  JsonDocument doc;
  const String url =
      services::station::dataUrl("/data/aircraft.json");
  if (!httpGetJson(url, "local aircraft", doc, filter) ||
      !doc["now"].is<double>() || !doc["aircraft"].is<JsonArray>()) {
    s_status.feed.reachable = false;
    services::station::expire();
    return false;
  }
  const double stamp = doc["now"].as<double>();
  const bool changed = !s_status.feed.valid || stamp != s_status.feed.timestamp;
  if (s_status.feed.valid && stamp < s_status.feed.timestamp) s_buffer.clear();
  if (!s_status.feed.accept(stamp, millis())) {
    services::station::expire();
    return false;
  }
  if (!changed) {
    services::station::expire();
    return true;
  }
  const uint32_t observedAt = millis();
  size_t n = 0;
  unsigned tracked = 0;
  float distances[kMaxAircraft] = {};
  for (JsonObject plane : doc["aircraft"].as<JsonArray>()) {
    const char* hex = plane["hex"] | "";
    // Explicit source/ground transitions supersede the old airborne position.
    if (!services::station::localType(plane["type"] | "") ||
        (isOnGround(plane) && !config::kAdsbShowGroundAircraft))
      s_buffer.remove(hex);
    if (!plane["hex"].is<const char*>() ||
        strlen(plane["hex"].as<const char*>()) == 0 ||
        strlen(plane["hex"].as<const char*>()) > 8 ||
        !services::station::localType(plane["type"] | "") ||
        !plane["seen"].is<float>() ||
        !std::isfinite(plane["seen"].as<float>()) ||
        plane["seen"].as<float>() < 0 || plane["seen"].as<float>() > 10)
      continue;
    ++tracked;
    if (!plane["lat"].is<float>() || !plane["lon"].is<float>() ||
        !plane["seen_pos"].is<float>() ||
        !std::isfinite(plane["seen_pos"].as<float>()) ||
        plane["seen_pos"].as<float>() < 0 ||
        plane["seen_pos"].as<float>() > 10 ||
        (isOnGround(plane) && !config::kAdsbShowGroundAircraft))
      continue;
    float lat = plane["lat"], lon = plane["lon"];
    if (!std::isfinite(lat) || !std::isfinite(lon) || fabs(lat) > 90 ||
        fabs(lon) > 180)
      continue;
    const float distance =
        services::station::distanceKm(center_lat, center_lon, lat, lon);
    if (distance > fetch_radius_km) {
      s_buffer.remove(hex);
      continue;
    }
    size_t slot = n;
    if (n >= kMaxAircraft) {
      slot = 0;
      for (size_t j = 1; j < n; ++j)
        if (distances[j] > distances[slot]) slot = j;
      if (distance >= distances[slot]) continue;
    } else
      ++n;
    distances[slot] = distance;
    auto& ac = s_aircraft[slot];
    copyJsonStringTrimmed(plane, "hex", ac.hex, sizeof(ac.hex));
    ac.observedAt = observedAt;
    ac.positionAge = plane["seen_pos"].as<float>();
    ac.lat = lat;
    ac.lon = lon;
    ac.nose_deg = pickNoseHeading(plane);
    ac.track_deg = pickTrackHeading(plane);
    ac.gs_knots = pickGroundSpeed(plane);
    fillTagFields(&ac, plane);
  }
  n = s_buffer.merge(s_aircraft, n, observedAt);
  s_aircraft_count = n;
  s_status.tracked = tracked;
  s_status.plotted = n;
  Serial.printf("local: %u tracked, %u plotted, heap %u\n", tracked,
                (unsigned)n, ESP.getFreeHeap());
  return true;
}

}  // namespace services::adsb

namespace services::station {
const Status& status() { return services::adsb::s_status; }
const Status& displayStatus() { return services::adsb::s_display_status; }
void commitDisplay() { services::adsb::s_display_status = status(); }
bool online() { return status().feed.online(millis()); }
void expire() {
  if (!status().feed.valid ||
      uint32_t(millis() - status().feed.advancedAt) >= kOfflineAfterMs) {
    if (services::adsb::s_aircraft_count)
      Serial.printf("expire: feed offline, clearing %u targets\n",
                    (unsigned)services::adsb::s_aircraft_count);
    services::adsb::s_buffer.clear();
    services::adsb::s_aircraft_count = 0;
    services::adsb::s_status.plotted = 0;
  } else {
    using namespace services::adsb;
    size_t n = 0;
    for (size_t i = 0; i < s_aircraft_count; ++i)
      if (AircraftBuffer<Aircraft, kMaxAircraft>::alive(s_aircraft[i],
                                                        millis()))
        s_aircraft[n++] = s_aircraft[i];
    if (n != s_aircraft_count)
      Serial.printf("expire: position age, removed %u targets\n",
                    (unsigned)(s_aircraft_count - n));
    s_aircraft_count = n;
    s_status.plotted = n;
  }
}
void fetchMetrics(bool temperature) {
  using namespace services::adsb;
  JsonDocument filter, doc;
  if (host().isEmpty()) return;
  if (!temperature) {
    filter["now"] = true;
    filter["total"]["local"]["accepted"] = true;
    if (httpGetJson(dataUrl("/data/stats.json"), "local stats", doc,
                    filter) &&
        doc["now"].is<double>() &&
        doc["total"]["local"]["accepted"].is<JsonArray>()) {
      double count = 0;
      JsonArray counts = doc["total"]["local"]["accepted"].as<JsonArray>();
      bool validCounts = counts.size() > 0;
      for (JsonVariant v : counts) {
        if (!v.is<double>() || !std::isfinite(v.as<double>()) ||
            v.as<double>() < 0)
          validCounts = false;
        count += v.as<double>();
      }
      double stamp = doc["now"];
      if (!validCounts || !std::isfinite(stamp) || stamp <= 0) {
        s_status.rateValid = false;
        s_counter_stamp = 0;
      } else {
        if (stamp > s_counter_stamp && s_counter_stamp > 0 &&
            count >= s_counter && stamp - s_counter_stamp <= 30) {
          s_status.messagesPerSecond =
              (count - s_counter) / (stamp - s_counter_stamp);
          s_status.rateValid = true;
          s_status.rateAt = millis();
        } else if (stamp < s_counter_stamp || count < s_counter)
          s_status.rateValid = false;
        if (stamp != s_counter_stamp) {
          s_counter_stamp = stamp;
          s_counter = count;
        }
      }
    }  // Transient transport failures retain the last bounded valid sample.
    return;
  }
  doc.clear();
  filter.clear();
  filter["cpu"] = true;
  filter["age"] = true;
  if (httpGetJson(webUrl("/api/get_temperatures.json"), "temperature", doc,
                  filter)) {
    // adsb.im reports CPU temperature as a string and sample age in seconds.
    const char* raw = doc["cpu"] | "";
    char* end = nullptr;
    float temp =
        doc["cpu"].is<float>() ? doc["cpu"].as<float>() : strtof(raw, &end);
    bool numeric = doc["cpu"].is<float>() || (*raw && end && *end == '\0');
    if (numeric && std::isfinite(temp) && temp >= -40 && temp <= 125 &&
        doc["age"].is<float>() && doc["age"].as<float>() >= 0 &&
        doc["age"].as<float>() < 30) {
      s_status.temperatureC = temp;
      s_status.temperatureAt = millis();
      s_status.temperatureValid = true;
      return;
    }
  }
  // Keep the last temperature until its existing 30-second expiry.
}
}  // namespace services::station
