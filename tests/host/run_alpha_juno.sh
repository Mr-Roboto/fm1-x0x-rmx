#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
mkdir -p build/host
${CC:-cc} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Ifirmware/src/dsp \
    -o build/host/alpha_juno_test tests/host/alpha_juno_test.c firmware/src/dsp/alpha_juno.c -lm
build/host/alpha_juno_test
