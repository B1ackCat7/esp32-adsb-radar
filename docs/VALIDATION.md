# Release validation

v1.0.0 is the first stable release, built from public source for ESP32-C3 Super Mini / GC9A01. Public checks cover native logic, source defaults, compilation, merged-image generation, source/history privacy review and release-artifact scanning. No device flash dump or live receiver capture is distributed.

Run `python3 scripts/run_checks.py` for aircraft-buffer, polling/snapshot scheduling, feed/source/control logic, palette/temperature, settings validation/rendering, world geometry and HTTP body framing suites. Framing checks cover every split in a generic JSON payload, chunk extensions/trailers, malformed and truncated bodies, content-length boundaries and close-delimited responses. World-map checks compare 8,220 sampled pixels at 20 generic centers and three ranges with an independent point-in-polygon reference, and check bounded failure/recovery. Settings tests include empty first-install fields, generic example values, valid/invalid addresses/ports/coordinates and escaping.

## Public-specific changes

- Station and first-run center remain blank; internal coordinate defaults are neutral `0, 0`.
- Aircraft fetching is disabled until a center and station are configured; settings status identifies an unset center.
- Every location uses the same world map; region-specific additions and private operational documentation/history are excluded.
- Serial Wi-Fi success messages omit the connected SSID. Project/build-cache paths are normalized before compilation.
- Upstream MIT notice, font OFL text and public geographic-data attribution are included.

## Validation limits and known limitations

Fresh-board onboarding, physical panel colors, physical buttons, cold unplug/replug, maximum receiver-payload memory usage and extended stability remain unconfirmed for this public release. Native tests and logical geometry checks do not establish these hardware properties. Settings use trusted-LAN HTTP without authentication; station discovery/HTTPS/arbitrary paths and generic board support are not implemented.

Browser/OTA firmware updates are unsupported: the partition table has one 3 MB application slot. Use the documented USB update procedure.

Inherited upstream GitHub workflows are unchanged. Local checks are the release validation record; a workflow file alone is not evidence that CI ran. Release checksums identify the published source-built artifacts.

The preceding v0.3.0-beta.2 passed all seven native suites, source/privacy scans, supermini compilation, merged-image generation and additional filtered ArduinoJson/framing checks. Its static RAM was 68,260 bytes and application flash was 2,362,610 of 3,145,728 bytes (75.1%). These figures do not measure worst-case runtime heap.

## v1.0.0 release scope

The first stable release promotes the existing public firmware with a new version identifier and updated installation, support and validation documentation. Receiver, display, map, settings and control behavior are unchanged. The stability designation does not claim additional physical hardware or long-duration testing. All seven native suites, source/privacy checks, supermini compilation and merged-image checks passed again for v1.0.0. Static RAM is 68,260 bytes and application flash is 2,362,594 of 3,145,728 bytes (75.1%). App/merged images passed the known-private-identifier scan; merged NVS padding is blank and the merged application payload matches the app image.
