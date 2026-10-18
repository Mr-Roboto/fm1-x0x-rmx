#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${BACKUP_TEST_OUT:-build/host/backup}
mkdir -p "$OUT"
${CC:-cc} ${BACKUP_TEST_FLAGS:-} -O2 -Wall -Wextra -Wno-unused-function -o "$OUT/transport" tests/backup_test.c
"$OUT/transport" "$OUT/transport.x0b"
${CC:-cc} ${BACKUP_TEST_FLAGS:-} -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -D_POSIX_C_SOURCE=200809L -DX0X_HOST -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o "$OUT/project" tests/host/backup_project_test.c \
    firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/bass303.c \
    firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c \
    firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c \
    firmware/src/seq/motion.c firmware/src/seq/remix.c -lm
"$OUT/project"
python3 tests/x0x_backup_test.py "$OUT/transport.x0b"
python3 tests/x0x_backup_record.py "$OUT/transport.x0b" "$OUT/transcript.json"
node tests/host/backup_page_test.mjs "$OUT/transcript.json"
