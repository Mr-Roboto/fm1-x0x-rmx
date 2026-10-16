#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Verify the minimally corrected release video; optional comparison with its original MP4."""
import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument('--original', type=Path)
a = ap.parse_args()
ff = os.environ.get('X0X_FFMPEG', 'ffmpeg')
video = Path('docs/media/video/x0x-rmx-demo.mp4')
expected = '0bd9721ce02a5f842c1504e3f450dc09fe4f02a796d2866bb5e690296abe43e2'
assert hashlib.sha256(video.read_bytes()).hexdigest() == expected

def run(src, args):
    return subprocess.check_output([ff, '-v', 'error', '-i', str(src), *args, '-']).decode()

def frames(src):
    data = run(src, ['-map', '0:v:0', '-f', 'framemd5'])
    return [line.split(',') for line in data.splitlines() if line and not line.startswith('#')]

rows = frames(video)
assert len(rows) == 2100
assert [int(row[2]) for row in rows] == list(range(2100))
assert 'eaf7b293c58129dcf3a6afe089148df463f1765145e05b6a3017a0e68c4905ac' in run(
    video, ['-map', '0:a:0', '-c:a', 'copy', '-f', 'hash', '-hash', 'sha256'])
if a.original:
    assert hashlib.sha256(a.original.read_bytes()).hexdigest() == '84adf956774d1678af3b0b8e3303841e968f01ca5533e98a210d1ce27765960c'
    old = frames(a.original)
    assert len(old) == len(rows)
    assert [r[-1] for r in rows[:2045]] == [r[-1] for r in old[:2045]]
    assert run(video, ['-map', '0:a:0', '-f', 'hash', '-hash', 'sha256']) == run(
        a.original, ['-map', '0:a:0', '-f', 'hash', '-hash', 'sha256'])
assert hashlib.sha256(Path('docs/media/video/x0x-rmx-demo-poster.png').read_bytes()).hexdigest() == 'e66e897da7736ccfd02d77e20094275324e56e3ce775ca0af2375e24411d2811'
assert hashlib.sha256(Path('docs/img/x0x-rmx-banner-1280x320.png').read_bytes()).hexdigest() == '349b82e67d76fd3968249f9555c47a7fdc75f3ef3856f2b975a01f76ce85874d'
print(json.dumps({'video_sha256': expected, 'frames': len(rows), 'duration_seconds': 84,
    'fps': 25, 'audio_bitstream_unchanged': True, 'original_prefix_compared': bool(a.original),
    'unchanged_prefix_seconds': 81.8, 'poster_and_banner_unchanged': True}, indent=2))
