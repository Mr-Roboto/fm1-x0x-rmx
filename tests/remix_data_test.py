#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Verify import provenance, packing, deterministic output and validation failures.

Optional --historical-source performs a direct comparison with the original KSP;
normal tests/builds use only repository data and the pinned historical digest.
"""
import argparse
import hashlib
import re
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from gen_remix import SOURCE, generate, pack, read_rows

HISTORICAL_SHA256 = "88034625d620ce4a1a87f954edf81850adf261155a7891fabfea68b01b6d60cb"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--historical-source", type=Path)
    args = parser.parse_args()
    rows = read_rows(SOURCE)
    values = bytes(value for row in rows for value in row)
    assert len(values) == 4096
    assert hashlib.sha256(values).hexdigest() == HISTORICAL_SHA256
    if args.historical_source:
        text = args.historical_source.read_text()
        matches = re.findall(r"declare\s+%remixes\[16\*16\*16\]\s*:=\s*\((.*?)\)", text, re.S)
        assert len(matches) == 1, "Expected one complete historical table"
        body = matches[0].replace("...", "")
        assert re.fullmatch(r"[\s\d,]+", body)
        historical = [int(token) for token in body.split(",") if token.strip()]
        assert len(historical) == 4096
        assert all(1 <= value <= 16 for value in historical)
        assert all(historical[i] == 1 for i in range(0, 4096, 16))
        assert historical[:16] == list(range(1, 17))
        assert values == bytes(value - 1 for value in historical)
        print("remix data: all 4096 values exactly match historical KSP")
    packed = pack(rows)
    assert len(packed) == 2048
    unpacked = bytes((packed[i // 2] >> (4 * (i % 2))) & 15 for i in range(4096))
    assert unpacked == values
    with tempfile.TemporaryDirectory() as tmp:
        first, second = Path(tmp) / "first.h", Path(tmp) / "second.h"
        generate(SOURCE, first)
        generate(SOURCE, second)
        assert first.read_bytes() == second.read_bytes()
        emitted = bytes(int(token, 16) for token in re.findall(r"0x([0-9a-f]{2})", first.read_text()))
        assert emitted == packed
        # Every required canonical invariant must reject malformed input.
        for kind in ("rows", "columns", "range", "first", "identity"):
            invalid = [row[:] for row in rows]
            if kind == "rows":
                invalid.pop()
            elif kind == "columns":
                invalid[1].pop()
            elif kind == "range":
                invalid[1][1] = 16
            elif kind == "first":
                invalid[1][0] = 1
            else:
                invalid[0][1] = 2
            bad = Path(tmp) / "invalid.txt"
            bad.write_text("\n".join(" ".join(map(str, row)) for row in invalid))
            try:
                read_rows(bad)
            except ValueError:
                pass
            else:
                raise AssertionError(f"Accepted invalid {kind}")
    print("remix data: historical digest, 2048-byte round trip, determinism and rejection checks passed")


if __name__ == "__main__":
    main()
