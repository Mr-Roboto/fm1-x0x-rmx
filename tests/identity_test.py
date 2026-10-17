#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""X0X-RMX identity rules and optional real build inspection; no MIDI or publishing.

python3 tests/identity_test.py [build/x0x-1.0.1.fwsc build/x0x.bin]
"""
import hashlib
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path("tools").resolve()))
import build
import fm1_install as installer
import fm1pkg_make as package
import make_pages


for version, expected in (
    ("1.0.1", "FM-1_987010001"),
    ("1.2", "FM-1_987010200"),
    ("0.10.3", "FM-1_987001003"),
    ("99.99.99", "FM-1_987999999"),
    ("1.0.1-rc1", "FM-1_987010001"),
):
    product = build.release_identity(version)
    assert product == expected
    assert len(product) <= 20 and re.fullmatch(r"FM-1_[0-9]+", product)
    # Exercise the real package markers, including unused padding bytes.
    raw = package.ufw(bytes(2048), b"FELUCCA-LOADER-1", product)
    assert installer.product_of(raw) == make_pages.product_of(raw) == product
    assert make_pages.is_release_identity(product)

for invalid in ("1", "1.2.3.4", "100.0.1", "-1.0.1", "1.0.100", "1.0.1-", "١.٠.١",
                "1.0.0", "1.0", "01.00.00"):
    try:
        build.release_identity(invalid)
    except ValueError:
        pass
    else:
        raise AssertionError(f"accepted invalid/reserved version {invalid}")

assert build.PRODUCT == "FM-1_987"
assert make_pages.is_release_identity("FM-1_9010000")  # immutable historical package
for invalid in ("FM-1_900", "FM-1_987", "FM-1_98701001", "FM-1_9870100010",
                "FM-1_987010001-rc1", "FM-1_98701A001", "XY-1_987010001"):
    assert not make_pages.is_release_identity(invalid), invalid
print("Identity: namespace, version boundaries, suffixes, markers, Pages gate and v1.0.0 protection passed")

historical_link = '<a href="https://mr-roboto.github.io/fm1-x0x-rmx/firmware/x0x-1.0.0.fwsc">Stable</a>'
assert make_pages.site_doc_links(historical_link) == historical_link
assert make_pages.site_doc_links(historical_link, "x0x-1.0.1.fwsc") == historical_link
assert 'href="../firmware/x0x-1.0.0.fwsc"' in make_pages.site_doc_links(historical_link, "x0x-1.0.0.fwsc")

stable = Path("build/x0x-1.0.0.fwsc")
if stable.exists():
    raw = stable.read_bytes()
    assert installer.product_of(raw) == "FM-1_9010000"
    assert hashlib.sha256(raw).hexdigest() == "eccacfa995efec6b9c819583e7bce43b3aa7a126033c8e668ae89c6a988b754c"
    print("Historical v1.0.0 identity and SHA-256 unchanged")

if len(sys.argv) > 1:
    raw = Path(sys.argv[1]).read_bytes()
    app = Path(sys.argv[2]).read_bytes()
    product = installer.product_of(raw)
    assert re.fullmatch(r"FM-1_987[0-9]{6}", product)
    assert (product + "\0").encode() in app, "app USB identity differs from package"
    assert installer.LOADER_MARK in raw
    assert len(app) <= package.APP_SLOT
    # Decode the container entry and SFC-encrypted app; verify the actual packaged app.
    logical = installer.logical_image(raw)
    entry = bytearray(logical[0x40:0x90])
    package.enc(entry, 0, len(entry))
    assert struct.unpack_from("<H", entry)[0] == 0  # flash.bin
    off, size = struct.unpack_from("<II", entry, 8)
    flash = logical[off:off + size]
    area = bytearray(flash[0x4000:])
    package.sfc(area, 0, len(area), 0, package.KEY)
    assert area[0x120:0x120 + len(app)] == app
    print(f"Built package/app agree exactly: {product}, {len(raw)} B package, {len(app)} B app")
