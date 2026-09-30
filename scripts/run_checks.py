#!/usr/bin/env python3
"""Compile/run every native C++ suite without creating tracked artifacts."""
import os
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='esp-radar-tests-') as directory:
    for source in sorted((root / 'tests').glob('*_test.cpp')):
        executable = str(Path(directory) / source.stem)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2',
                        '-I', str(root / 'include'), str(source), '-o', executable], check=True)
        subprocess.run([executable], check=True)
        print(f'PASS {source.name}', flush=True)
