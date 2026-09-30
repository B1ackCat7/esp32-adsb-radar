#pragma once

#include <cstddef>
#include <cstdint>

namespace services::adsb {

struct Aircraft {
  char hex[9];
  uint32_t observedAt;
  float positionAge;
  float lat;
  float lon;
  float nose_deg;
  float track_deg;
  float gs_knots;
  char callsign[9];
  char type[5];
  char alt[12];
};

constexpr size_t kMaxAircraft = 64;
inline bool heldPosition(const Aircraft& ac, uint32_t now) {
  return ac.positionAge + uint32_t(now - ac.observedAt) / 1000.0f >= 15;
}

size_t aircraftCount();
const Aircraft* aircraftList();

/** Hook invoked during long HTTP I/O (e.g. wifiLoop). Optional. */
using PollFn = void (*)();
void setPollFn(PollFn fn);
/** Clear receiver data and discard in-flight responses after a settings save. */
void configurationChanged();

/** Fetch aircraft within fetch_radius_km of center_lat/lon from the configured
 * local readsb receiver. */
bool fetchUpdate(double center_lat, double center_lon, float fetch_radius_km);

}  // namespace services::adsb
