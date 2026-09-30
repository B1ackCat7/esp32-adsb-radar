#include "ui/world_geometry.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace ui::map;

// Independent geographic point-in-polygon reference, not the cached scanlines.
bool waterAt(double lat, double lon) {
  bool inside[2] = {};
  for (const auto& r : worldData::rings) {
    if (lat < r.minLat * 0.0001 || lat > r.maxLat * 0.0001) continue;
    for (int shift : {-360, 0, 360}) {
      if (lon < r.minLon * 0.0001 + shift || lon > r.maxLon * 0.0001 + shift)
        continue;
      for (uint32_t i = r.first; i + 1 < r.first + r.count; ++i) {
        const auto& a = worldData::points[i];
        const auto& b = worldData::points[i + 1];
        const double ay = a.lat * 0.0001, by = b.lat * 0.0001;
        if ((ay > lat) == (by > lat)) continue;
        const double x = (a.lon + (lat - ay) / (by - ay) * (b.lon - a.lon)) * 0.0001 + shift;
        if (x < lon) inside[r.water] = !inside[r.water];
      }
    }
  }
  return !inside[0] || inside[1];
}

int main() {
  world::Geometry geometry;
  world::Span spans[world::kMaxSpans];
  const float centers[][2] = {{47.6062,-122.3321}, {-33.8688,151.2093},
    {51.5074,-0.1278}, {40.7128,-74.0060}, {35.6762,139.6503},
    {59.9139,10.7522}, {60.1699,24.9384}, {-17.7134,178.0650},
    {0,179.9}, {0,-179.9}, {0,0}, {39,-100}, {-23.5505,-46.6333},
    {48.8566,2.3522}, {52.3676,4.9041}, {65,-18}, {31.5,35.5},
    {1.3521,103.8198}, {84,20}, {-84,20}};
  size_t maxEdges = 0, maxSpans = 0, checked = 0;
  for (const auto& c : centers) {
    for (float outer : {100.f/3.f, 200.f/3.f, 400.f/3.f}) {
      assert(geometry.build(c[0], c[1], outer, spans));
      maxEdges = std::max(maxEdges, geometry.edgeCount);
      maxSpans = std::max(maxSpans, geometry.spanCount);
      std::vector<bool> raster(240*240, true);
      for (size_t i = 0; i < geometry.spanCount; ++i) {
        const auto s = spans[i];
        assert(s.y >= 13 && s.y <= 227 && s.x >= 13 && s.x <= 227);
        for (int x = s.x; x < s.x + std::abs(s.width); ++x) {
          assert(x <= 227);
          assert((x-120)*(x-120)+(s.y-120)*(s.y-120) <= 107*107);
          raster[s.y*240+x] = s.width < 0;
        }
      }
      for (int y = -90; y <= 90; y += 15) {
        for (int x = -90; x <= 90; x += 15) {
          if (x*x+y*y > 100*100) continue;
          const auto expected = waterAt(c[0]-y/geometry.scale, c[1]+x/geometry.sx);
          if (raster[(y+120)*240+x+120] != expected) {
            // Sub-pixel shoreline rounding may affect its boundary pixel.
            bool boundary = false;
            for (int dx : {-1,0,1}) for (int dy : {-1,0,1})
              boundary |= raster[(y+dy+120)*240+x+dx+120] == expected;
            if (!boundary) std::cerr << "Mismatch " << c[0] << ',' << c[1] << " range " << outer << " pixel " << x << ',' << y << '\n';
            assert(boundary);
          }
          ++checked;
        }
      }
    }
  }
  assert(!waterAt(39,-100)); // interior land, without a visible shoreline
  assert(waterAt(0,0)); // open ocean, without a visible shoreline
  assert(waterAt(-1,33)); // Lake Victoria
  assert(!geometry.build(85,0,100,spans));
  assert(!geometry.available);
  assert(!geometry.build(0,181,100,spans));
  assert(!geometry.build(NAN,0,100,spans));
  assert(!geometry.build(39,-100,100,spans,1)); // bounded fill exhaustion
  assert(!geometry.available);
  assert(geometry.build(39,-100,100,spans)); // normal rebuild after failure
  std::cout << checked << " geographic samples passed; max edges=" << maxEdges
            << " max spans=" << maxSpans << " cache bytes="
            << sizeof(geometry)+sizeof(spans) << '\n';
}
