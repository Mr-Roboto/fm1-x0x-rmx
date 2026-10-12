#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/host
for test in remix_state remix_ui remix_motion; do
    engine_src=firmware/src/app/engine.c
    # This test includes engine.c to exercise its private ISR callbacks/runtime.
    if [ "$test" = remix_motion ]; then engine_src=; fi
    ${CC:-cc} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
        -D_POSIX_C_SOURCE=200809L -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
        -o build/host/${test}_test tests/host/${test}_test.c \
        firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c \
        firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
        firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c \
        firmware/src/seq/motion.c firmware/src/seq/remix.c $engine_src -lm
    build/host/${test}_test
done
