#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# X0X in the browser: the host simulator compiled to WebAssembly (Emscripten), plus its page.
#   [X0X_VERSION=0.4-beta] web/emu/build.sh  ->  build/emu/{index.html, worklet.js, x0x.wasm}
# Same sources and flags as host/build_host.sh (-ffp-contract=off, like the device).
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/gen build/emu
python3 tools/gen_remix.py build/gen/x0x_remix_table.h
python3 tools/gen_grids.py build/gen/x0x_grids_map.h
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
[ -f build/gen/x0x_drum_samples.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
sh tools/gen_builtin_break.sh build/gen >/dev/null
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c
   firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
   firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/seq/remix.c firmware/src/app/engine.c"
# shellcheck disable=SC2086
emcc -O2 -ffp-contract=off -std=gnu99 -Wall -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable \
    -DX0X_HOST -DX0X_WEB "-DX0X_VERSION=\"$(printf %s "${X0X_VERSION:-DEV}" | tr a-z A-Z)\"" -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    --no-entry -sSTANDALONE_WASM -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=33554432 -sFILESYSTEM=0 \
    -o build/emu/x0x.wasm web/emu/x0x_web.c $U
cp web/emu/index.html web/emu/worklet.js web/emu/favicon.svg build/emu/
# Local provenance only: does not change firmware identity or WebAssembly behavior.
python3 - <<'PROVENANCE'
import hashlib
import json
import subprocess
from pathlib import Path

wasm = Path("build/emu/x0x.wasm")
info = {
    "project": "X0X-RMX",
    "revision": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
    "branch": subprocess.check_output(["git", "branch", "--show-current"], text=True).strip(),
    "working_tree_changes": subprocess.check_output(["git", "status", "--porcelain"], text=True).splitlines(),
    "wasm_sha256": hashlib.sha256(wasm.read_bytes()).hexdigest(),
    "compiler": subprocess.check_output(["emcc", "--version"], text=True).splitlines()[0],
}
Path("build/emu/build-info.json").write_text(json.dumps(info, indent=2) + "\n")
PROVENANCE
echo "emu: build/emu ($(wc -c < build/emu/x0x.wasm) B wasm)"
