#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/gen build/host
python3 tools/gen_grids.py build/gen/x0x_grids_map.h
python3 tools/gen_remix.py build/gen/x0x_remix_table.h
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
[ -f build/gen/x0x_drum_samples.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
sh tools/gen_builtin_break.sh build/gen >/dev/null
"${CC:-cc}" -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -D_POSIX_C_SOURCE=200809L -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/drum_gen_project_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c \
    firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c \
    firmware/src/seq/remix.c firmware/src/app/engine.c \
    -lm -o build/host/drum_gen_project_test
build/host/drum_gen_project_test
