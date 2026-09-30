#pragma once
#include <cstdint>
namespace ui::theme {
constexpr uint16_t rgb(unsigned r, unsigned g, unsigned b) {
  return ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3);
}
constexpr uint16_t background = rgb(5, 14, 32);
constexpr uint16_t land = rgb(10, 27, 49);
constexpr uint16_t water = rgb(3, 12, 29);
constexpr uint16_t road = rgb(20, 49, 73);
constexpr uint16_t shoreline = rgb(30, 77, 102);
constexpr uint16_t grid = rgb(23, 68, 101);
constexpr uint16_t cyan = rgb(81, 195, 225);
constexpr uint16_t orange = rgb(255, 157, 61);
constexpr uint16_t white = rgb(227, 242, 250);
constexpr uint16_t muted = rgb(124, 165, 188);
constexpr uint16_t danger = rgb(255, 97, 101);
// Missing/stale values are neutral, not a retained temperature alarm.
constexpr uint16_t temperatureColor(float celsius, bool valid) {
  return !valid             ? muted
         : celsius > 80.0f  ? danger
         : celsius >= 60.0f ? orange
                            : cyan;
}
}  // namespace ui::theme
