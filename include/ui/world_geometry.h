#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "data/world_map.h"

namespace ui::map::world {
constexpr int kRadius = 107;
constexpr size_t kMaxEdges = 768;
constexpr size_t kMaxSpans = 1024;
struct Span { int16_t x, y, width; }; // negative width = water, positive = land
struct EdgeRef { uint32_t point; int16_t shift; uint16_t water; };

// Only selected shoreline edges live in RAM. Complete polygons stay in flash;
// far-left crossings retain fill parity even when no coast is visible.
class Geometry {
 public:
  EdgeRef edges[kMaxEdges];
  size_t edgeCount = 0, spanCount = 0;
  float centerLat = 0, centerLon = 0, scale = 0, sx = 0;
  bool available = false;

  void project(const EdgeRef& edge, float& ax, float& ay,
               float& bx, float& by) const {
    const auto& a = worldData::points[edge.point];
    const auto& b = worldData::points[edge.point + 1];
    ax = (a.lon * 0.0001f + edge.shift - centerLon) * sx;
    bx = (b.lon * 0.0001f + edge.shift - centerLon) * sx;
    ay = (centerLat - a.lat * 0.0001f) * scale;
    by = (centerLat - b.lat * 0.0001f) * scale;
  }

  bool build(float lat, float lon, float outerKm, Span* spans,
             size_t capacity = kMaxSpans) {
    edgeCount = spanCount = 0;
    available = false;
    if (!std::isfinite(lat) || !std::isfinite(lon) ||
        !std::isfinite(outerKm) || outerKm <= 0 ||
        std::abs(lat) >= 85 || lon < -180 || lon > 180) return false;
    centerLat = lat; centerLon = lon;
    scale = kRadius / outerKm * 111.f;
    sx = scale * std::cos(lat * 0.01745329252f);
    const float minLat = lat - kRadius / scale;
    const float maxLat = lat + kRadius / scale;
    const float minLon = lon - kRadius / sx;
    const float maxLon = lon + kRadius / sx;
    // Conservatively cull in source integer units before software-float projection.
    const int32_t minLatUnits = int32_t(std::floor(minLat * 10000.f)) - 1;
    const int32_t maxLatUnits = int32_t(std::ceil(maxLat * 10000.f)) + 1;
    uint8_t baseline[2][kRadius * 2 + 1] = {};
    for (const auto& ring : worldData::rings) {
      if (ring.minLat * 0.0001f > maxLat || ring.maxLat * 0.0001f < minLat)
        continue;
      for (int shift : {-360, 0, 360}) {
        if (ring.minLon * 0.0001f + shift > maxLon ||
            ring.maxLon * 0.0001f + shift < minLon) continue;
        for (uint32_t i = ring.first; i + 1 < ring.first + ring.count; ++i) {
          const auto& a = worldData::points[i];
          const auto& b = worldData::points[i + 1];
          if (std::min(a.lat, b.lat) > maxLatUnits ||
              std::max(a.lat, b.lat) < minLatUnits) continue;
          const EdgeRef edge = {i, int16_t(shift), uint16_t(ring.water)};
          float ax, ay, bx, by;
          project(edge, ax, ay, bx, by);
          if (std::min(ay, by) > kRadius || std::max(ay, by) < -kRadius)
            continue;
          if (std::max(ax, bx) < -kRadius) {
            const int from = std::max(-kRadius, int(std::floor(std::min(ay, by))) + 1);
            const int to = std::min(kRadius, int(std::floor(std::max(ay, by))));
            for (int y = from; y <= to; ++y) baseline[ring.water][y + kRadius] ^= 1;
          } else if (std::min(ax, bx) <= kRadius) {
            if (edgeCount == kMaxEdges) return false;
            edges[edgeCount++] = edge;
          }
        }
      }
    }
    for (unsigned water = 0; water < 2; ++water) {
      for (int y = -kRadius; y <= kRadius; ++y) {
        float hits[128];
        size_t count = 0;
        for (size_t i = 0; i < edgeCount; ++i) {
          const auto& edge = edges[i];
          if (edge.water != water) continue;
          const auto& a = worldData::points[edge.point];
          const auto& b = worldData::points[edge.point + 1];
          const float ay = (centerLat - a.lat * 0.0001f) * scale;
          const float by = (centerLat - b.lat * 0.0001f) * scale;
          if ((ay < y) == (by < y)) continue;
          const float ax = (a.lon * 0.0001f + edge.shift - centerLon) * sx;
          const float bx = (b.lon * 0.0001f + edge.shift - centerLon) * sx;
          if (count == 128) return false;
          hits[count++] = ax + (y - ay) / (by - ay) * (bx - ax);
        }
        std::sort(hits, hits + count);
        const int extent = int(std::sqrt(float(kRadius * kRadius - y * y)));
        bool inside = baseline[water][y + kRadius] != 0;
        float begin = -1e9f;
        auto addSpan = [&](float end) {
          // Clamp before converting float to int; off-screen bounds can be huge.
          const float leftF = std::max(float(-extent), begin);
          const float rightF = std::min(float(extent), end);
          if (rightF < leftF) return true;
          const int left = int(std::ceil(leftF)), right = int(std::floor(rightF));
          if (right < left) return true;
          if (spanCount == capacity) return false;
          const int width = right - left + 1;
          spans[spanCount++] = {int16_t(120 + left), int16_t(120 + y),
                               int16_t(water ? -width : width)};
          return true;
        };
        for (size_t i = 0; i < count; ++i) {
          if (inside) { if (!addSpan(hits[i])) return false; }
          else begin = hits[i];
          inside = !inside;
        }
        if (inside && !addSpan(1e9f)) return false;
      }
    }
    available = true;
    return true;
  }
};
}  // namespace ui::map::world
