#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host/waveform-ui
${CC:-cc} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -D_POSIX_C_SOURCE=200809L -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o build/host/waveform-ui/test tests/host/bass303_ui_test.c \
    firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c \
    firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c \
    firmware/src/seq/motion.c firmware/src/seq/remix.c -lm
build/host/waveform-ui/test
