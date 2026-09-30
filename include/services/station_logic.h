#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
namespace services::station {
constexpr uint32_t kOfflineAfterMs = 30000;
constexpr uint32_t kFeedDelayedMs = 20000;
struct Freshness {
  bool valid = false;
  bool reachable = false;
  double timestamp = 0;
  uint32_t advancedAt = 0;
  bool accept(double stamp, uint32_t now) {
    if (!std::isfinite(stamp) || stamp <= 0) {
      reachable = false;
      return false;
    }
    // Restarting readsb may reset its clock. Treat a changed epoch as new data.
    if (!valid || stamp != timestamp) {
      timestamp = stamp;
      advancedAt = now;
    }
    valid = true;
    reachable = true;
    return online(now);
  }
  bool online(uint32_t now) const {
    // A failed poll never resets the grace window or immediately changes
    // status.
    return valid && uint32_t(now - advancedAt) < kOfflineAfterMs;
  }
};
inline bool localType(const char* type) {
  // Only locally decoded ADS-B / Mode S; no rebroadcast TIS-B or MLAT
  // positions.
  if (!type) return false;
  return std::strcmp(type, "adsb_icao") == 0 ||
         std::strcmp(type, "adsb_icao_nt") == 0 ||
         std::strcmp(type, "adsb_other") == 0 ||
         std::strcmp(type, "mode_s") == 0;
}
inline double distanceKm(double lat1, double lon1, double lat2, double lon2) {
  constexpr double r = 0.017453292519943295;
  const double a = std::sin((lat2 - lat1) * r / 2),
               b = std::sin((lon2 - lon1) * r / 2);
  double h = a * a + std::cos(lat1 * r) * std::cos(lat2 * r) * b * b;
  return 12742.0 * std::asin(std::sqrt(h > 1 ? 1 : h));
}
// Release-time classification survives HTTP work between button presses.
struct Clicks {
  bool pending = false;
  uint32_t releasedAt = 0;
  int release(uint32_t at) {
    if (pending && uint32_t(at - releasedAt) <= 350) {
      pending = false;
      return 2;
    }
    int action = pending ? 1 : 0;
    pending = true;
    releasedAt = at;
    return action;
  }
  int poll(uint32_t now) {
    if (pending && uint32_t(now - releasedAt) > 350) {
      pending = false;
      return 1;
    }
    return 0;
  }
};
}  // namespace services::station
