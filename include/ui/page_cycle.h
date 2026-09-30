#pragma once
#include <cstdint>
namespace ui {
// Three radar scales followed by Station. Selection is RAM-only: automatic
// rotation must never write NVS or alter the shared aircraft observation buffer.
struct Pages {
  static constexpr uint8_t kRadarCount = 3, kStation = 3, kCount = 4;
  uint8_t current = 0, lastRadar = 0;
  void select(uint8_t page) {
    if (page >= kCount) return;
    current = page;
    if (page < kRadarCount) lastRadar = page;
  }
  void next() { select((current + 1) % kCount); }
  void nextRadar() { select((lastRadar + 1) % kRadarCount); }
};
// Unsigned elapsed time also handles millis() wrapping after ~49 days.
struct PageCycle {
  static constexpr uint32_t kIntervalMs = 25000;
  uint32_t startedAt = 0;
  void restart(uint32_t now) { startedAt = now; }
  bool poll(uint32_t now) {
    if (uint32_t(now - startedAt) < kIntervalMs) return false;
    restart(now);
    return true;
  }
};
}  // namespace ui
