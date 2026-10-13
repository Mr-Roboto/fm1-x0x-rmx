#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Local cubic prototype must be present; legacy source is pinned before integration.
set -eu
cd "$(dirname "$0")/../.."
OUT=build/host/waveform-integration
BASE=e6454b95c542479093fc824b869f2a022cb9fc1c
CC="${CC:-cc}"
mkdir -p "$OUT/baseline"
git show "$BASE:firmware/src/dsp/bass303.c" > "$OUT/baseline/bass303.c"
git show "$BASE:firmware/src/dsp/bass303.h" > "$OUT/baseline/bass303.h"
python3 - <<'PY'
from pathlib import Path
s = Path('firmware/src/dsp/bass303.c').read_text()
start = s.index('static float morph_eval(')
end = s.index('/* Low-level oscillator controls', start)
s = s[:start] + '''static float morph_eval(const bass303_morph_mix_t *v, float phase, float d)
{
    waveform_mix_t ref;
    ref.shark = v->shark; ref.trap = v->trap; ref.notch = v->notch;
    ref.dc = v->dc; ref.jump = v->jump;
    for (int i = 0; i < 5; i++) ref.slope[i] = v->slope[i];
    return waveform_eval(&ref, phase, d, WAVEFORM_CUBIC);
}
''' + s[end:]
Path('build/host/waveform-integration/oracle.c').write_text('#include "waveform_prototype.h"\n' + s)
PY
W="-std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror -Ifirmware/src/dsp -Itests/host"
BASE_NAMES=""
ORACLE_NAMES=""
for f in init set note_on note_off all_off render set_lite nparams param get; do
    BASE_NAMES="$BASE_NAMES -Dbass303_$f=baseline_bass303_$f"
    ORACLE_NAMES="$ORACLE_NAMES -Dbass303_$f=oracle_bass303_$f"
done
ORACLE_NAMES="$ORACLE_NAMES -Dbass303_set_waveform=oracle_bass303_set_waveform -Dbass303_set_morph=oracle_bass303_set_morph"
for OS in 1 2 4; do
    for CTRL in 1 4; do
        $CC $W -DBASS303_OS=$OS -DBASS303_CTRL=$CTRL $BASE_NAMES -c "$OUT/baseline/bass303.c" -o "$OUT/baseline.o"
        # Oracle retains unused production cubic helpers; only its oscillator is replaced.
        $CC $W -Wno-unused-function -DBASS303_OS=$OS -DBASS303_CTRL=$CTRL $ORACLE_NAMES -c "$OUT/oracle.c" -o "$OUT/oracle.o"
        $CC $W -DBASS303_OS=$OS -DBASS303_CTRL=$CTRL tests/host/bass303_waveform_test.c tests/host/waveform_prototype.c "$OUT/baseline.o" "$OUT/oracle.o" -lm -o "$OUT/test-$OS-$CTRL"
        "$OUT/test-$OS-$CTRL"
    done
done
