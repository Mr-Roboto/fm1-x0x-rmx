#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/gen build/host
python3 tests/remix_data_test.py "$@"
python3 tools/gen_remix.py build/gen/x0x_remix_table.h
"${CC:-cc}" -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror \
    -Ifirmware/src -Ibuild/gen tests/host/remix_test.c firmware/src/seq/remix.c \
    -o build/host/remix_test
build/host/remix_test
