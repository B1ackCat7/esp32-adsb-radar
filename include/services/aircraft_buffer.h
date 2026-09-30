#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace services::adsb {
// Keep a missing observation briefly, never renewing its original position age.
constexpr uint32_t kMissingAircraftHoldMs = 30000;
constexpr double kMaxPositionAgeSeconds = 30;
template <typename T, size_t Capacity>
class AircraftBuffer {
 public:
  void clear() { count_ = 0; }
  void remove(const char* hex) {
    for (size_t i = 0; i < count_;)
      if (strcmp(entries_[i].hex, hex) == 0)
        entries_[i] = entries_[--count_];
      else
        ++i;
  }
  size_t merge(T* fresh, size_t count, uint32_t now) {
    // Fresh observations always take priority over retained targets at
    // capacity.
    for (size_t i = 0; i < count_; ++i) {
      const auto& old = entries_[i];
      bool replaced = false;
      for (size_t j = 0; j < count; ++j)
        if (strcmp(fresh[j].hex, old.hex) == 0) replaced = true;
      if (!replaced && count < Capacity && alive(old, now))
        fresh[count++] = old;
    }
    count_ = count;
    for (size_t i = 0; i < count; ++i) entries_[i] = fresh[i];
    return count;
  }
  static bool alive(const T& entry, uint32_t now) {
    const uint32_t elapsed = now - entry.observedAt;
    return elapsed < kMissingAircraftHoldMs &&
           entry.positionAge + elapsed / 1000.0 < kMaxPositionAgeSeconds;
  }

 private:
  T entries_[Capacity]{};
  size_t count_ = 0;
};
}  // namespace services::adsb
