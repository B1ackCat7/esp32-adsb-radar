#include "services/radar_location.h"

#include <Preferences.h>
#include <cstdlib>
#include <cstring>

#include "config.h"
#include "services/settings_validation.h"

namespace services::location {

namespace {

constexpr char kPrefsNamespace[] = "radar";
constexpr char kKeyLat[] = "lat";
constexpr char kKeyLon[] = "lon";

double s_lat = config::kDefaultRadarLat;
double s_lon = config::kDefaultRadarLon;
bool s_configured = false;

bool validLatLon(double lat, double lon) {
  return lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
}

bool persist(double lat, double lon) {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, false)) return false;
  const bool ok = prefs.putDouble(kKeyLat, lat) == sizeof(double) &&
                  prefs.putDouble(kKeyLon, lon) == sizeof(double);
  prefs.end();
  if (!ok) return false;
  s_lat = lat;
  s_lon = lon;
  s_configured = true;
  return true;
}

}  // namespace

void init() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, true);
  if (prefs.isKey(kKeyLat) && prefs.isKey(kKeyLon)) {
    const double lat = prefs.getDouble(kKeyLat, config::kDefaultRadarLat);
    const double lon = prefs.getDouble(kKeyLon, config::kDefaultRadarLon);
    if (validLatLon(lat, lon)) {
      s_lat = lat;
      s_lon = lon;
      s_configured = true;
    }
  }
  prefs.end();
}

double lat() { return s_lat; }

double lon() { return s_lon; }
bool configured() { return s_configured; }

bool saveFromStrings(const char* lat_str, const char* lon_str) {
  double lat = 0.0;
  double lon = 0.0;
  if (!settings::parseCoordinates(lat_str, lon_str, lat, lon)) return false;
  if (s_configured && lat == s_lat && lon == s_lon) return true;
  return persist(lat, lon);
}

void clear() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.remove(kKeyLat);
  prefs.remove(kKeyLon);
  prefs.end();
  s_lat = config::kDefaultRadarLat;
  s_lon = config::kDefaultRadarLon;
  s_configured = false;
}

}  // namespace services::location
