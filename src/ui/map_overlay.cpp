#include "ui/map_overlay.h"

#include <algorithm>
#include <cmath>

#include "services/radar_location.h"
#include "ui/display_theme.h"
#include "ui/radar_range.h"
#include "ui/radar_theme.h"
#include "ui/world_geometry.h"

namespace ui::map {
namespace {
constexpr int radius = radar::kGridOuterRadius;
static_assert(radius == world::kRadius, "Map and radar projections must agree");
world::Span spans[world::kMaxSpans];
world::Geometry geometry;
float cachedLat = 0, cachedLon = 0, cachedScale = 0;
bool cacheValid = false;
uint32_t buildMicros = 0;

// Clip crossing shoreline segments to the circular radar, including segments
// whose endpoints are both outside the viewport.
void line(lgfx::LGFXBase& g, float ax, float ay, float bx, float by,
          uint16_t color) {
  const float dx = bx - ax, dy = by - ay, a = dx * dx + dy * dy;
  if (a < 0.01f) return;
  const float b = 2 * (ax * dx + ay * dy);
  const float c = ax * ax + ay * ay - radius * radius;
  const float discriminant = b * b - 4 * a * c;
  if (discriminant < 0) return;
  const float lo = std::max(0.f, (-b - sqrtf(discriminant)) / (2 * a));
  const float hi = std::min(1.f, (-b + sqrtf(discriminant)) / (2 * a));
  if (lo > hi) return;
  g.drawLine(120 + lroundf(ax + lo * dx), 120 + lroundf(ay + lo * dy),
             120 + lroundf(ax + hi * dx), 120 + lroundf(ay + hi * dy), color);
}
}  // namespace

void invalidateCache() { cacheValid = false; }
const char* coverage() {
  if (!services::location::configured()) return "Set radar coordinates";
  const float lat = services::location::lat(), lon = services::location::lon();
  if (fabsf(lat) >= 85.f) return "Polar map unavailable";
  if (!cacheValid || cachedLat != lat || cachedLon != lon)
    return "World coastlines and lakes (rebuild pending)";
  return geometry.available ? "World coastlines and lakes" :
      "Map exceeds device cache; range rings remain available";
}
uint32_t rebuildMicros() { return buildMicros; }
size_t selectedEdges() { return geometry.edgeCount; }
size_t cachedSpans() { return geometry.available ? geometry.spanCount : 0; }

void draw(lgfx::LGFXBase& g) {
  if (!services::location::configured()) return;
  const float lat = services::location::lat(), lon = services::location::lon();
  const float scale = radius / radar::rangeCurrent().outer_km * 111.f;
  if (!cacheValid || lat != cachedLat || lon != cachedLon ||
      scale != cachedScale) {
    cachedLat = lat;
    cachedLon = lon;
    cachedScale = scale;
    const uint32_t started = micros();
    geometry.build(lat, lon, radar::rangeCurrent().outer_km, spans);
    buildMicros = micros() - started;
    // Cache failures too, avoiding repeated work until center/range changes.
    cacheValid = true;
  }
  if (!geometry.available) return;
  g.fillCircle(120, 120, radius, theme::water);
  for (size_t i = 0; i < geometry.spanCount; ++i) {
    const auto& span = spans[i];
    g.drawFastHLine(span.x, span.y, std::abs(span.width),
                    span.width < 0 ? theme::water : theme::land);
  }
  for (size_t i = 0; i < geometry.edgeCount; ++i) {
    float ax, ay, bx, by;
    geometry.project(geometry.edges[i], ax, ay, bx, by);
    line(g, ax, ay, bx, by, theme::shoreline);
  }
}
}  // namespace ui::map
