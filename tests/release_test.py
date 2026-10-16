#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Inspect local stable 1.0.0 artifacts; never publishes or talks to a device."""
import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path('tools').resolve()))
import fm1_install as installer
import fm1pkg_make as package

pkg = Path('build/x0x-1.0.0.fwsc')
raw = pkg.read_bytes()
app = Path('build/x0x.bin').read_bytes()
wasm = Path('build/emu/x0x.wasm').read_bytes()
assert installer.product_of(raw) == 'FM-1_9010000'
assert installer.LOADER_MARK in raw
assert b'X0X 1.0.0\0' in app
# Emscripten can synthesize the ABOUT text with stores; p9_emu_test verifies it at runtime.
assert b'BETA' not in app and b'BETA' not in wasm
assert len(app) <= package.APP_SLOT
info = json.loads(Path('build/emu/build-info.json').read_text())
assert info['version'] == '1.0.0'
assert info['wasm_sha256'] == hashlib.sha256(wasm).hexdigest()
print(f'Stable 1.0.0: product {installer.product_of(raw)}, display X0X 1.0.0, loader present')
print(f'Package {len(raw)} B; app {len(app)} B; flash reserve {package.APP_SLOT - len(app)} B')
print(f'SHA-256 {hashlib.sha256(raw).hexdigest()}')
print('Firmware and emulator carry the stable version without an implicit suffix')
