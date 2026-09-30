# ESP32 ADS-B Radar

A local ADS-B radar and station monitor for an **ESP32-C3 Super Mini with a 240×240 GC9A01 round display**. Connect it to your own ADSB.im/readsb receiver, choose your radar center in the browser, and watch nearby aircraft over an offline world base map.

Derived from **[MatixYo/ESP32-Plane-Radar v1.1.4](https://github.com/MatixYo/ESP32-Plane-Radar/tree/v1.1.4)**. Original radar graphics, Wi-Fi setup, wiring and runway support are credited to the upstream project. This fork adds local receiver integration, a Station page, configurable connection settings, four-page rotation, bounded aircraft retention and world-map generation. The upstream MIT license is preserved in [LICENSE](LICENSE).

**First public beta: v0.3.0-beta.1.** This binary targets the hardware and wiring below. Fresh-board onboarding, physical panel/button acceptance and extended stability testing remain open; see [validation](docs/VALIDATION.md).

## Display and controls

- Pages rotate **25 km Radar → 50 km Radar → 100 km Radar → Station**, every **25 seconds**. Startup is 25 km. Aircraft text is shown only at 25 km.
- Single-click BOOT advances all four pages. Double-click advances radar range, skipping Station. Manual selection restarts the page timer.
- Hold BOOT for three seconds to clear Wi-Fi, center and display preferences. The station address/ports remain saved.
- Range labels refer to the third ring. The outer ring is 4/3 of the displayed distance. Miles and runway overlays are configurable.
- Aircraft, local message rate and optional Pi CPU temperature refresh on a fixed ten-second cycle. Station publishes a consistent ten-second snapshot.
- Missing aircraft retain their last known position for at most 30 seconds. At 15 seconds they are muted and lose their vector. Feed delay is shown at 20 seconds without timestamp advancement; offline/expiry occurs at 30 seconds. No position prediction is used.

## Install

Download the app image, merged image and checksums from [Releases](https://github.com/B1ackCat7/esp32-adsb-radar/releases).

| Installation | Image | Flash offset |
| --- | --- | --- |
| Fresh supported board | `esp32-adsb-radar-v0.3.0-beta.1-merged.bin` | `0x0` |
| Compatible existing partition layout | `esp32-adsb-radar-v0.3.0-beta.1-app.bin` | `0x10000` |

Verify the SHA-256 checksum and board type. Back up your complete flash before updating an existing device. An app-only update preserves settings when the existing partition layout matches [plane_radar.csv](partitions/plane_radar.csv). A merged write can overwrite settings; use it for fresh installation or deliberate recovery. Do not distribute a device flash dump: it can contain Wi-Fi credentials and saved settings.

With Python and esptool installed, replace `<PORT>` with your board's port:

```sh
python -m esptool --chip esp32c3 --port <PORT> read-flash 0 0x400000 private-backup.bin
# Fresh install:
python -m esptool --chip esp32c3 --port <PORT> write-flash 0x0 esp32-adsb-radar-v0.3.0-beta.1-merged.bin
# Compatible update instead:
python -m esptool --chip esp32c3 --port <PORT> write-flash 0x10000 esp32-adsb-radar-v0.3.0-beta.1-app.bin
```

## Configure your station

1. Join the radar's `PlaneRadar-Setup` Wi-Fi and open `http://192.168.4.1`. Select **Configure Wi-Fi** and join your home network.
2. Open **[http://plane-radar.local/](http://plane-radar.local/)** or `http://<radar-IP>/`. Find the radar's DHCP address in your router if `.local` does not resolve. `/param` and `/settings` also open Radar settings.
3. Enter your receiver's **Station IPv4 address**, for example `192.168.1.100`. This is a generic example, not a preconfigured receiver. Enter the address without a URL, hostname, port or path. A DHCP reservation keeps it stable.
4. Enter your own latitude/longitude in decimal degrees and choose units/runways. New installations leave the station and coordinates blank; there is no preselected household. Neutral internal coordinates are `0, 0` and are not used to fetch aircraft until a center is saved.
5. Save. Settings apply without reboot. The new map builds when Radar next draws; receiver updates follow the normal polling cycle. Read-only status shows connection freshness, metrics and map coverage. It does not probe unsaved addresses.

Advanced ports default to **8080** for aircraft/statistics and **80** for the ADSB.im web/temperature endpoint. Configure the ports your station actually uses. Clearing the station address disconnects it. Invalid forms are rejected before writing; expired/stale forms require a reload. Wi-Fi settings are separate.

| HTTP endpoint on your station | Used for |
| --- | --- |
| `http://<station>:<data-port>/data/aircraft.json` | Local aircraft and advancing `now` timestamp |
| `http://<station>:<data-port>/data/stats.json` | Local message-counter delta |
| `http://<station>:<web-port>/api/get_temperatures.json` | Optional ADSB.im Pi CPU temperature/sample age |

Tracked traffic includes local targets without a usable position; plotted symbols are airborne targets in range. MLAT/TIS-B/ADS-R and ground traffic are excluded. An advancing empty feed is ONLINE with zero tracked targets; unavailable metrics show dashes. A generic readsb receiver can supply aircraft/statistics without the ADSB.im temperature endpoint.

Station addresses are IPv4 only in this release; HTTPS, arbitrary paths and station hostname discovery are not supported. The radar's own `.local` name is supported. For multiple radars, use their individual IPs until configurable names are added. Settings use HTTP on a trusted home LAN and have no login; do not expose the portal to the Internet.

## Automatic offline map

Saving different coordinates regenerates the base map from bundled **Natural Earth world land/lake polygons**. No map download, cloud API, map account or external location request is required. Coverage is generalized coastlines and major lakes, with polygon holes and date-line views supported. Streets, rivers and many small features are omitted. At or beyond 85° latitude, or if a bounded geometry cache is exceeded, the radar shows range rings and reports map availability in settings.

All locations use the same world dataset. See [map provenance and regeneration](docs/MAP_DATA.md).

## Hardware

| GC9A01 signal | ESP32-C3 Super Mini |
| --- | --- |
| VCC | 3.3 V |
| GND | GND |
| RST | GPIO 0 |
| CS | GPIO 1 |
| DC | GPIO 10 |
| SDA / MOSI | GPIO 3 |
| SCL / SCLK | GPIO 4 |
| BOOT button | GPIO 9, active LOW |

Display channel order/inversion are panel-profile settings in `include/config.h`; verify them on your own panel. Do not assume this binary fits every ESP32 variant or differently wired display.

## Build and test

```sh
python3 -m venv .venv
.venv/bin/pip install platformio==6.2.0
python3 scripts/run_checks.py
.venv/bin/pio run -e supermini
.venv/bin/pio run -e supermini -t merge
```

Build outputs are under `.pio/build/supermini/`. Release binaries are freshly compiled from source, with project/build-cache paths normalized; they are not device backups. [Development notes](docs/DEVELOPMENT.md) describe architecture, diagnostics and the branch layout. [Privacy notes](docs/PRIVACY.md) describe stored settings and publication boundaries.

## Credits and licenses

- Firmware foundation: [MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar), MIT. Baseline commit `9d857787ecf067ad68924b2600c63f19ef0fcba7` (v1.1.4).
- Map: [Natural Earth](https://www.naturalearthdata.com/), public domain.
- Airports/runways: [OurAirports](https://ourairports.com/data/), public domain.
- Bundled Noto Sans font: SIL Open Font License 1.1; [license and provenance](THIRD_PARTY_NOTICES.md).
- Runtime libraries: LovyanGFX, WiFiManager and ArduinoJson; versions are pinned in `platformio.ini`.
