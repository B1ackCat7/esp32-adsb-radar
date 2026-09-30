# Offline world map data

Made with Natural Earth. The bundled 1:50m land and lake polygons are [public domain](https://www.naturalearthdata.com/about/terms-of-use/). All locations use this same dataset. Generalized coastlines/major lakes are included; streets, rivers and many small features are not represented.

Sources retrieved September 30, 2026:

- https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/ne_50m_land.geojson
- https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/ne_50m_lakes.geojson

Source SHA-256:

- `ne_50m_land.geojson`: `e874b27a51d146452be360cafb3cc50c86001074a67d534113e6534682f9826b`
- `ne_50m_lakes.geojson`: `d350b75978b26fe839b797c2c529b2fb8f47fb3983c03f4964e36d5df9378a52`

Run `python3 scripts/build_world_map.py ne_50m_land.geojson ne_50m_lakes.geojson` to reproduce `include/data/world_map.h`. Coordinates are quantized to 1e-4 degrees; adjacent duplicates and collapsed rings are removed. The generator rejects unsplit longitude-seam crossings. 79,283 points and 1,887 complete rings occupy 687,100 bytes in flash. Inputs are public source datasets, never a receiver's settings or household coordinates.

The saved center and current range control projection. `ui/world_geometry.h` selects nearby rings using bounding boxes and integer latitude culling; selected edge references enter a bounded 768-edge RAM cache. Complete polygons stay in flash. Even-odd scanline filling preserves off-screen crossings and island holes. Land fills over an ocean background; lakes fill over land. Longitude copies at -360/0/+360 support views crossing the date line.

The existing 1,024-span buffer is shared between builds; scanline intersections are capped at 128. A capacity failure omits geography and is reported in settings, while aircraft/rings remain. Failed builds are cached until center/range changes. Centers at or beyond 85 degrees latitude show rings because this projection is unsuitable there. No map service is contacted and no second framebuffer is allocated.

`tests/world_map_test.cpp` compares sampled map pixels with an independent geographic point-in-polygon reference across multiple continents and ranges, including interior land/ocean, date-line views and bounded failure/recovery. The geographic reference is the bundled source vectors.
