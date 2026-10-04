# README page previews

These images are software-rendered examples, not device captures or photographs.
They use the public firmware's unchanged Radar, Station, world-map and runway
drawing code, its bundled VLW/bitmap fonts, and pinned LovyanGFX 1.2.30 rasterization.
The host adapter replaces the display panel with an RGB565 sprite and supplies
synthetic services. It never reads Wi-Fi credentials, NVS, receiver data, private
captures or an attached device.

The center is a public Sydney example (`-33.87, 151.21`). All eight aircraft,
callsigns, positions, headings and station metrics are invented. The Station
example shows 14 tracked targets (including six without usable positions),
872 messages/second and 47 C. Kilometers and runway overlays are enabled.
The map/airport datasets retain their attribution in the repository notices.

The four native images are 240×240. Pixels outside the round viewport are
transparent. The overview arranges the same pages with captions; its 2× nearest
neighbor enlargement preserves the original pixels. PNGs contain only image
chunks, with no paths, EXIF or saved configuration.

After a PlatformIO build has populated the pinned library cache, regenerate:

```sh
python3 scripts/render_readme_previews.py
```

Requires Python 3 and C/C++17 compilers; no Pillow, SDL or display server is
needed. `--library` accepts another local copy of LovyanGFX 1.2.30. The script
copies that library to a temporary build, adapts only its platform/entry headers,
and compiles the original drawing/font implementations. It does not modify the
installed library or firmware build. Temporary sources/executables/raw frames
are removed after rendering. Keep screenshots current when UI code changes.

These previews do not establish physical panel colors, button behavior, device
memory usage or hardware acceptance.
