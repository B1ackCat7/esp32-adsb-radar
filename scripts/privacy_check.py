#!/usr/bin/env python3
"""Check tracked/publishable files for common accidental private artifacts.

This is a guardrail, not proof that arbitrary sensitive content is absent.
"""
import re
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
paths = subprocess.check_output(['git', '-C', str(root), 'ls-files', '-z',
                                 '--cached', '--others', '--exclude-standard'])
patterns = {
    'developer home path': rb'/(?:Users|home)/[A-Za-z0-9_.-]+/',
    'credential token': rb'(?:gh[pousr]_[A-Za-z0-9]{20,}|AKIA[A-Z0-9]{16})',
    'private key': rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----',
    'device serial path': rb'/dev/cu\.usbmodem[0-9]+',
}
forbidden_parts = {'backups', 'validation', 'captures', 'handoff', 'private-backup'}
forbidden_names = {'PROJECT_MEMORY.md', 'FLASH_RECORD.md', '.env'}
findings = []
checked = 0
for raw_path in paths.split(b'\0'):
    if not raw_path:
        continue
    relative = Path(raw_path.decode())
    file = root / relative
    if not file.is_file():
        continue  # A staged deletion need not be scanned.
    checked += 1
    if (set(relative.parts) & forbidden_parts or relative.name in forbidden_names
            or relative.suffix.lower() in {'.bin', '.elf', '.ppm', '.log', '.pem', '.key'}):
        findings.append((str(relative), 'private/build artifact'))
    data = file.read_bytes()
    for category, pattern in patterns.items():
        if re.search(pattern, data):
            findings.append((str(relative), category))
if findings:
    for path, category in findings:
        print(f'FAIL {path}: {category}')
    sys.exit(1)
print(f'Privacy pattern check passed for {checked} public files; review staged changes too.')
