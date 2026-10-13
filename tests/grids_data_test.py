#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Verify all reduced nodes; emit original source tables for host tests only."""
import importlib.util
from pathlib import Path
import re
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("gen_grids", ROOT / "tools/gen_grids.py")
gen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gen)


def main():
    arrays = gen.read_arrays()
    layout = ((10, 8, 0, 9, 11), (15, 7, 13, 12, 6), (18, 14, 4, 5, 3),
              (23, 16, 21, 1, 2), (24, 19, 17, 20, 22))
    assert gen.LAYOUT == layout
    assert sorted(n for row in layout for n in row) == list(range(25))
    reduced = gen.reduce_nodes(arrays)
    assert len(reduced) == 1200
    for x in range(5):
        for y in range(5):
            for ch in range(3):
                for step in range(16):
                    assert reduced[((x * 5 + y) * 3 + ch) * 16 + step] == \
                        arrays[f"node_{layout[x][y]}"][ch * 32 + step * 2]
    with tempfile.TemporaryDirectory() as directory:
        output = Path(directory) / "map.h"
        gen.generate(output)
        first = output.read_bytes()
        gen.generate(output)
        assert output.read_bytes() == first
        # The generated header has exactly one initializer and no original LUT.
        body = re.sub(r"/\*.*?\*/", "", first.decode(), flags=re.S).split("= {", 1)[1]
        assert bytes(map(int, re.findall(r"\d+", body))) == reduced
        assert "euclidean" not in first.decode().lower()
        corrupt = Path(directory) / "bad.cc"
        corrupt.write_bytes(gen.SOURCE.read_bytes() + b"\n")
        try:
            gen.read_arrays(corrupt)
        except ValueError:
            pass
        else:
            raise AssertionError("modified source accepted")
    destination = Path(sys.argv[1])
    destination.parent.mkdir(parents=True, exist_ok=True)
    lines = ["/* SPDX-License-Identifier: GPL-3.0-or-later */",
             "/* Copyright 2012 Emilie Gillet. Original Grids data, HOST TESTS ONLY. */",
             "static const uint8_t dg_ref_node[25][96] = {"]
    lines += ["    {" + ",".join(map(str, arrays[f"node_{i}"])) + "}," for i in range(25)]
    lines += ["};", "static const uint32_t dg_ref_euclid[1024] = {"]
    assert len(arrays["lut_res_euclidean"]) == 1024
    lines += [",".join(str(v) + "u" for v in arrays["lut_res_euclidean"][start:start + 32]) + ","
              for start in range(0, 1024, 32)]
    lines += ["};"]
    destination.write_text("\n".join(lines) + "\n")
    print("Grids data: 25 nodes / 1,200 retained values, layout, checksum and reproducibility passed")


if __name__ == "__main__":
    main()
