#!/usr/bin/env python3
"""Render real firmware UI code with a headless LovyanGFX sprite adapter.

Requires C/C++17 compilers and the pinned LovyanGFX cache from a PlatformIO
build. No device, network, saved settings, Pillow or display server is used.
"""
import argparse
import concurrent.futures
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
ADAPTER = ROOT / 'scripts' / 'preview'


def png_chunk(kind, data):
    return (struct.pack('>I', len(data)) + kind + data +
            struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff))


def convert_frame(source, target, round_view=False, scale=1):
    with source.open('rb') as file:
        assert file.readline() == b'P6\n'
        width, height = map(int, file.readline().split())
        assert file.readline() == b'255\n'
        pixels = file.read()
    assert len(pixels) == width * height * 3
    channels = 4 if round_view else 3
    rows = bytearray()
    for y in range(height):
        row = bytearray(b'\0')
        for x in range(width):
            at = (y * width + x) * 3
            pixel = pixels[at:at + 3]
            if round_view:
                pixel += bytes([255 if (x - 120)**2 + (y - 120)**2 <= 120**2 else 0])
            row.extend(pixel * scale)
        rows.extend(row * scale)
    header = struct.pack('>IIBBBBB', width * scale, height * scale, 8,
                         6 if channels == 4 else 2, 0, 0, 0)
    target.write_bytes(b'\x89PNG\r\n\x1a\n' + png_chunk(b'IHDR', header) +
                       png_chunk(b'IDAT', zlib.compress(rows, 9)) + png_chunk(b'IEND', b''))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path,
                        default=ROOT / '.pio/libdeps/supermini/LovyanGFX')
    parser.add_argument('--output', type=Path, default=ROOT / 'docs/images')
    args = parser.parse_args()
    version_file = args.library / 'src/lgfx/v1/gitTagVersion.h'
    if not version_file.is_file():
        parser.error('Build with PlatformIO first, or pass --library with LovyanGFX 1.2.30.')
    version = version_file.read_text()
    assert all(f'#define LGFX_VERSION_{key} {value}' in version
               for key, value in [('MAJOR', 1), ('MINOR', 2), ('PATCH', 30)])
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='plane-radar-preview-') as directory:
        work = Path(directory)
        library = work / 'graphics'
        shutil.copytree(args.library / 'src', library)
        # Modify only the temporary platform/entry headers. The actual drawing,
        # font, sprite and color conversion implementations remain unchanged.
        shutil.copyfile(ADAPTER / 'platform.hpp', library / 'lgfx/v1/platforms/common.hpp')
        (library / 'LovyanGFX.hpp').write_text(
            '#pragma once\n#include "lgfx/v1/lgfx_filesystem_support.hpp"\n'
            '#include "lgfx/v1/LGFXBase.hpp"\n#include "lgfx/v1/LGFX_Sprite.hpp"\n')
        font = (ROOT / 'data/ui_font.vlw').read_bytes()
        (work / 'font.cpp').write_text(
            '#include <cstdint>\nextern "C" {\n'
            'extern const uint8_t font[] asm("_binary_data_ui_font_vlw_start") = {' +
            ','.join(str(b) for b in font) + '};\n}\n'
            'asm(".global _binary_data_ui_font_vlw_end\\n'
            f'.set _binary_data_ui_font_vlw_end, _binary_data_ui_font_vlw_start + {len(font)}");\n')
        include = ['-I', str(ADAPTER / 'include'), '-I', str(ROOT / 'include'),
                   '-I', str(library)]
        cpp = [ADAPTER / 'graphics.cpp', ADAPTER / 'main.cpp', work / 'font.cpp']
        cpp += [ROOT / 'src' / name for name in [
            'hardware/display_font.cpp', 'ui/radar_display.cpp',
            'ui/station_display.cpp', 'ui/map_overlay.cpp', 'ui/runway_overlay.cpp',
            'ui/radar_range.cpp', 'data/large_airports_data.cpp']]
        c = list((library / 'lgfx/utility').glob('*.c'))
        for relative in ['lgfx/Fonts/efont', 'lgfx/Fonts/IPA', 'lgfx/Fonts/lvgl', 'lgfx/v1/lv_font']:
            c += list((library / relative).glob('*.c'))
        sources = cpp + sorted(c)
        objects = [work / f'{i}.o' for i in range(len(sources))]

        def compile_source(pair):
            source, obj = pair
            is_cpp = source.suffix == '.cpp'
            compiler = os.environ.get('CXX' if is_cpp else 'CC', 'c++' if is_cpp else 'cc')
            flags = ['-std=c++17', '-include', str(ADAPTER / 'include/Arduino.h')] if is_cpp else ['-std=c11']
            subprocess.run([compiler, *flags, '-O2', *include,
                            '-c', str(source), '-o', str(obj)], check=True)

        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as workers:
            list(workers.map(compile_source, zip(sources, objects)))
        executable = work / 'render'
        subprocess.run([os.environ.get('CXX', 'c++'), *map(str, objects),
                        '-o', str(executable)], check=True)
        subprocess.run([str(executable), str(work)], check=True)
        for name in ['radar-25km', 'radar-50km', 'radar-100km', 'station']:
            convert_frame(work / f'{name}.ppm', args.output / f'{name}.png', round_view=True)
        convert_frame(work / 'overview.ppm', args.output / 'pages-overview.png', scale=2)
    print('Rendered four firmware pages and overview using synthetic Sydney data.')


if __name__ == '__main__':
    main()
