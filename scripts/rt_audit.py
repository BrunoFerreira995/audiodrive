#!/usr/bin/env python3
import re
import sys
from pathlib import Path

RT_PATHS = [
    Path('src/native_audio.cpp'),
    Path('src/audio_unit.cpp'),
    Path('src/dsp.cpp'),
    Path('src/mixer.cpp'),
    Path('src/buffer.cpp'),
    Path('src/simd.cpp'),
]

FORBIDDEN = [
    (r'\bmalloc\s*\(', 'malloc'),
    (r'\bcalloc\s*\(', 'calloc'),
    (r'\brealloc\s*\(', 'realloc'),
    (r'\bfree\s*\(', 'free'),
    (r'\bnew\s+', 'new'),
    (r'\bdelete\s+', 'delete'),
    (r'\.lock\s*\(', 'lock()'),
    (r'std::mutex', 'std::mutex'),
    (r'std::lock_guard', 'std::lock_guard'),
    (r'std::unique_lock', 'std::unique_lock'),
    (r'std::shared_lock', 'std::shared_lock'),
    (r'std::condition_variable', 'condition_variable'),
    (r'std::this_thread::sleep_for', 'sleep'),
]

errors = []
for path in RT_PATHS:
    if not path.exists():
        continue
    content = path.read_text()
    for pattern, name in FORBIDDEN:
        if re.search(pattern, content):
            errors.append(f'{path}: forbidden real-time pattern: {name}')

if errors:
    for e in errors:
        print('RT-AUDIT FAIL:', e)
    sys.exit(1)
else:
    print('RT-AUDIT PASS')
    sys.exit(0)
