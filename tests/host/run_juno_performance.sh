#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
${CC:-cc} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -D_POSIX_C_SOURCE=200809L -DX0X_HOST -DX0X_JUNO -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o build/host/juno_performance_test tests/host/juno_performance_test.c \
    firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c \
    firmware/src/dsp/alpha_juno.c firmware/src/dsp/juno_part.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c \
    firmware/src/seq/motion.c firmware/src/seq/remix.c -lm
build/host/juno_performance_test "$@"
