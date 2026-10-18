#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=build/host/break-filter
mkdir -p "$OUT/reference"
CC="${CC:-cc}"
for f in breaks.c breaks.h; do
    git show "47ca00828bb0689ca404a4ed35d61b0168259e7a:firmware/src/dsp/$f" > "$OUT/reference/$f"
done
# The old header's relative pattern include resolves against this fixture tree.
mkdir -p "$OUT/seq"
cp firmware/src/seq/pattern.h "$OUT/seq/pattern.h"
W="-O2 -ffp-contract=off -std=c99 -Wall -Wextra -Werror"
$CC $W -Ifirmware/src/dsp -Ibuild/gen tests/host/break_filter_test.c -lm -o "$OUT/dsp_test"
$CC $W -DBREAK_FILTER_REFERENCE -I"$OUT/reference" -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/break_filter_test.c -lm -o "$OUT/reference_test"
"$OUT/dsp_test"
"$OUT/reference_test" "$OUT/reference.bin"
for pot in 60 61 62 63 64 65 66 67 68; do
    "$OUT/dsp_test" "$OUT/bypass.bin" "$pot"
    cmp "$OUT/reference.bin" "$OUT/bypass.bin"
done
echo "PASS: all OFF positions bit-exact against pre-filter playback/audio/state (pitch, tempo, reverse, half-speed, stutter, retrigger, pin, gate, stop/restart, blocks 1/17/256)"
$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -D_POSIX_C_SOURCE=200809L -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o "$OUT/ui_test" tests/host/break_filter_ui_test.c \
    firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c \
    firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c \
    firmware/src/seq/motion.c firmware/src/seq/remix.c -lm
"$OUT/ui_test"
if [ "${BREAK_FILTER_SANITIZE:-0}" = 1 ]; then
    S="-fsanitize=address,undefined -fno-sanitize-recover=all -g"
    $CC $W $S -Ifirmware/src/dsp -Ibuild/gen tests/host/break_filter_test.c -lm -o "$OUT/sanitized_test"
    "$OUT/sanitized_test"
    "$OUT/sanitized_test" "$OUT/sanitized.bin" 64
    cmp "$OUT/reference.bin" "$OUT/sanitized.bin"
    $CC $W $S tests/host/fastmath_test.c -lm -o "$OUT/fastmath_sanitized_test"
    "$OUT/fastmath_sanitized_test"
fi
