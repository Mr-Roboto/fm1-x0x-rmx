#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/gen build/host
python3 tools/gen_grids.py build/gen/x0x_grids_map.h
python3 tests/grids_data_test.py build/host/x0x_grids_reference.h
"${CC:-cc}" -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror \
    ${DRUM_GEN_TEST_FLAGS:-} -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen -Ibuild/host \
    tests/host/drum_gen_test.c -o build/host/drum_gen_test
build/host/drum_gen_test
