# Development

This fork's application branch is `adsb-station`, based on public upstream v1.1.4. The fork's `main` branch retains the original upstream snapshot copied by GitHub; `adsb-station` includes the reviewed upstream history through `139a3ac7d173d0f1a03ac66d121e2dc8aba35718`. Clone uses the default `adsb-station` branch. See [the current synchronization decisions](UPSTREAM_SYNC.md). Review subsequent upstream changes before merging them: the local receiver adapter, RGB565 framebuffer, settings page and polling semantics differ from current upstream.

Use Python 3 and PlatformIO 6.2.0. `platformio.ini` pins espressif32 6.5.0 / Arduino-ESP32 2.0.14, LovyanGFX 1.2.30, WiFiManager 2.0.17 and ArduinoJson 7.4.3. Native tests need a C++17 compiler. `scripts/run_checks.py` runs every `*_test.cpp` suite in a temporary directory. `scripts/privacy_check.py` checks public files for common accidental private artifacts.

## Architecture

- `services/station_settings.cpp`: station IPv4 and ports persisted together as `station/connection`; legacy `station/host` remains readable. No migration write occurs at boot.
- `services/radar_location.cpp`: saved center; unset coordinates remain unconfigured and prevent aircraft requests. Invalid input is rejected before saving.
- `services/settings_portal.cpp`: `/`, `/param`, `/settings`, POST `/paramsave`, read-only `/api/radar/status`. WiFiManager manages Wi-Fi separately. Save uses a per-boot token and revision; old in-flight receiver results are invalidated after connection/center changes. Storage across station/location/display namespaces is not an atomic transaction.
- `services/adsb_client.cpp`: numeric HTTP endpoints, bounded JSON body reader, one request at a time. Aircraft/statistics/temperature use a fixed ten-second cycle with phases 0/3/6 seconds. Snapshot publication is also ten seconds. Optional metrics can be deferred on failure.
- `services/aircraft_buffer.h`: one shared, widest-range 64-target buffer; views filter locally without clearing it or forcing requests. Missing positions expire by 30 seconds, mute at 15 seconds. Feed warning/offline thresholds are 20/30 seconds.
- `services/http_body_framing.h`: upstream HTTP/1.1 body decoder, adapted to the bounded cooperative LAN reader. Content-length and chunked bodies are drained and checked before publication; compressed/unsupported encodings are rejected.
- `ui/map_overlay.cpp` and `ui/world_geometry.h`: world map vectors remain in flash; a shared 6 KB span cache and 6 KB edge-reference cache rebuild on center/range changes. Capacity/polar failure retains radar/rings. No additional framebuffer or map network request.
- `ui/page_cycle.h`: four 25-second pages, manual priority, RAM-only selection.

One shared 240×240 RGB565 framebuffer serves Radar and Station. Logical palette values are RGB; panel order/inversion are independent profile settings. Pi temperature is cyan below 60 C, amber at 60–80 C inclusive, red above 80 C; unavailable values use muted dashes.

## Diagnostics

USB commands: `1`/`2`/`3` select ranges; `n`/`m` invoke single/double-click actions; `t` selects Station; `r` returns to Radar. `d` reports status, performance and heap. `b` compares cold/warm frame hashes and prints map rebuild time, edges/spans and coverage. `s` emits `P6\n240 240\n255\n` followed by exactly 172800 RGB bytes. These checks do not establish physical button/color acceptance. Keep captures and serial logs private.

For transport faults, compare complete body receipt and advancing timestamps, not only receiver website availability. Body reading uses a 1 KB buffer, six-second total deadline, 1.5-second idle deadline and 200 KB cap. Connect/header waits remain synchronous and bounded. Scheduling intervals are targets and can be delayed by rendering/network work. Zero traffic alone is healthy.

`scripts/sanitize_build_paths.py` maps the project and PlatformIO core paths to generic prefixes in compiled strings/debug info. Release images must be rebuilt from the audited public source. Do not publish ELFs, full-device flash dumps or local build logs as release assets.
