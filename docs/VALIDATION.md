# Public beta validation

v0.3.0-beta.1 is a source-built public beta for ESP32-C3 Super Mini / GC9A01. Public checks cover native logic, source defaults, compilation, merged-image generation, source/history privacy review and release-artifact scanning. No device flash dump or live receiver capture is distributed.

Run `python3 scripts/run_checks.py` for aircraft-buffer, polling/snapshot scheduling, feed/source/control logic, palette/temperature, settings validation/rendering and world geometry suites. World-map checks compare 8,220 sampled pixels at 20 generic centers and three ranges with an independent point-in-polygon reference, and check bounded failure/recovery. Settings tests include empty first-install fields, generic example values, valid/invalid addresses/ports/coordinates and escaping.

## Public-specific changes

- Station and first-run center remain blank; internal coordinate defaults are neutral `0, 0`.
- Aircraft fetching is disabled until a center and station are configured; settings status identifies an unset center.
- Every location uses the same world map; region-specific additions and private operational documentation/history are excluded.
- Serial Wi-Fi success messages omit the connected SSID. Project/build-cache paths are normalized before compilation.
- Upstream MIT notice, font OFL text and public geographic-data attribution are included.

## Remaining acceptance

Fresh-board onboarding, physical panel colors, physical buttons, cold unplug/replug, maximum receiver-payload memory usage and extended stability remain unconfirmed for this public beta. Native tests and logical geometry checks do not establish these hardware properties. Settings use trusted-LAN HTTP without authentication; station discovery/HTTPS/arbitrary paths and generic board support are not implemented.

Inherited upstream GitHub workflows are unchanged. Local checks are the release validation record; a workflow file alone is not evidence that CI ran. Release checksums identify the published source-built artifacts.

Local release checks passed: all six native suites, source/privacy scans, supermini compilation and merged-image generation. Public build static RAM is 68,252 bytes; application flash is 2,360,870 of 3,145,728 bytes (75.1%). These build figures do not measure worst-case runtime heap.
