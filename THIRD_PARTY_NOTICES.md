# Third-party notices

The firmware is derived from [MatixYo/ESP32-Plane-Radar v1.1.4](https://github.com/MatixYo/ESP32-Plane-Radar/tree/v1.1.4). The original MIT copyright and permission notice are preserved verbatim in `LICENSE` and apply to the derivative firmware source.

## Bundled font

`data/ui_font.vlw` is byte-identical to [TFT_eSPI's NotoSansBold15.vlw](https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Smooth%20Fonts/LittleFS/Font_Demo_1/data/NotoSansBold15.vlw). SHA-256: `6ebf531830cce68d53d1aac46397d8dd100adc202ed5bfe065f60fd92047046f`.

Noto font copyright: Copyright 2018 The Noto Project Authors (github.com/googlei18n/noto-fonts). Font software is under SIL Open Font License 1.1. The complete upstream license is included as [licenses/NotoSans-OFL.txt](licenses/NotoSans-OFL.txt); it applies to the font separately from the firmware MIT license.

## Geographic data

Made with [Natural Earth](https://www.naturalearthdata.com/). World land/lake data is public domain under [Natural Earth terms](https://www.naturalearthdata.com/about/terms-of-use/). Exact source hashes and regeneration are recorded in `docs/MAP_DATA.md`.

Airport/runway data is generated from [OurAirports](https://ourairports.com/data/), whose data is public domain. `scripts/build_large_airports.py` records upstream source URLs and compilation filters.

## Libraries

PlatformIO downloads pinned library dependencies rather than vendoring them here: [LovyanGFX](https://github.com/lovyan03/LovyanGFX) (FreeBSD/BSD license and bundled font notices), [WiFiManager](https://github.com/tzapu/WiFiManager) (MIT), [ArduinoJson](https://github.com/bblanchon/ArduinoJson) (MIT). Arduino-ESP32/ESP-IDF and toolchain components retain their own licenses; consult their distributed notices when building or redistributing firmware.

Complete pinned library notices are included in `licenses/`: LovyanGFX (including its third-party notices), GFX FreeFonts, WiFiManager and ArduinoJson. The firmware source MIT notice does not replace any third-party license.
