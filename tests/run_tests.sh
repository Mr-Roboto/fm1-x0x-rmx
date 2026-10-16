#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# X0X host tests: the maths, the sequencer + motif-based TB-3PO, each engine
# against its reference implementation, and the whole firmware app in the simulator
# (tests/scenarios/*.x0x: UI, patterns, persistence, MIDI clock), with screenshots and
# audio in build/scenarios/. Needs the reference sources (see tests/host/run_*.sh).
#   tests/run_tests.sh
set -u
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
FAIL=0
PKG="${X0X_TEST_PACKAGE:-build/x0x.fwsc}"
OUT=build/host
mkdir -p "$OUT" build/scenarios
python3 tools/gen_grids.py build/gen/x0x_grids_map.h
run() {
    name="$1"; shift
    if "$@" > "$OUT/$name.log" 2>&1; then
        echo "  ok   $name"
    else
        echo "  FAIL $name (see $OUT/$name.log)"
        tail -15 "$OUT/$name.log" | sed 's/^/       /'
        FAIL=1
    fi
}
W="-O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror"
run tb3po sh tests/host/run_tb3po.sh
run drum-generator sh tests/host/run_drum_gen.sh
run drum-generator-project sh tests/host/run_drum_gen_project.sh
run drum-generator-ui sh tests/host/run_drum_gen_ui.sh
run remix sh tests/host/run_remix.sh
run remix-state sh tests/host/run_remix_state.sh
run schemes sh tests/host/run_scheme_ui.sh
run fastmath sh -c "$CC -O2 -ffp-contract=off -Wall -Wextra -Werror -o $OUT/fastmath_test tests/host/fastmath_test.c -lm && $OUT/fastmath_test"   # its reference is double on purpose
seq_test() {
    $CC -O2 -ffp-contract=off -Wall -Wextra -o $OUT/seq_test tests/host/seq_test.c \
        -Ibuild/gen firmware/src/seq/sequencer.c firmware/src/seq/remix.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c || return 1
    $OUT/seq_test
}
run sequencer seq_test
run encoder sh -c "$CC -O2 -w -Ifirmware/hal -o $OUT/encoder_test tests/host/encoder_test.c && $OUT/encoder_test"
run bass303-ui sh tests/host/run_bass303_ui.sh
run bass303-waveform sh tests/host/run_bass303_waveform.sh
run bass303_lite sh -c "$CC -std=c99 -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -o $OUT/bass303_lite_test tests/host/bass303_lite_test.c firmware/src/dsp/bass303.c -lm && $OUT/bass303_lite_test"
run uac sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/uac_test tests/host/uac_test.c -lm && $OUT/uac_test"
run trs sh -c "$CC -O2 -w -Ifirmware/src -Ifirmware/hal -o $OUT/trs_test tests/host/trs_test.c && $OUT/trs_test"
run undo sh -c "$CC -O2 -Wall -Wextra -Wno-unused-function -o $OUT/undo_test tests/host/undo_test.c && $OUT/undo_test"
run lz sh -c "$CC $W -o $OUT/lz_test tests/host/lz_test.c && $OUT/lz_test"
run motion sh -c "$CC $W -o $OUT/motion_test tests/host/motion_test.c firmware/src/seq/motion.c && $OUT/motion_test"
run drum-pan sh -c "$CC -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -Ibuild/gen -o $OUT/drum_pan_test tests/host/drum_pan_test.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c -lm && $OUT/drum_pan_test"
run stereo sh -c "$CC -O2 -ffp-contract=off -w -Ifirmware/src/dsp -Ifirmware/src -Ibuild/gen -o $OUT/stereo_test tests/host/stereo_test.c firmware/src/dsp/master.c firmware/src/dsp/fxbus.c -lm && $OUT/stereo_test"
run master sh -c "$CC $W -Wno-double-promotion -o $OUT/master_test tests/host/master_test.c firmware/src/dsp/master.c -lm && $OUT/master_test"
# Reference comparisons require external source checkouts; never disguise a comparison failure.
for t in drum909 drum808 bass303 breaks; do
    ref=""
    case "$t" in
        drum909) ref="${NINEW9_DIR:-../schwung-9W9}/src/dsp/er99_engine.c" ;;
        drum808) ref="${EIGHTW8_DIR:-../schwung-8W8}/src/dsp/sc808_engine.cpp" ;;
        bass303) ref="${SCHWUNG303:-../schwung-303}/src/dsp/open303/rosic_Open303.cpp" ;;
    esac
    if [ -n "$ref" ] && [ ! -f "$ref" ]; then
        echo "  skip $t (external reference unavailable: $ref)"
    else
        run "$t" sh "tests/host/run_$t.sh"
    fi
done
run storage sh -c "$CC -O2 -o $OUT/storage_test tests/storage_test.c && $OUT/storage_test"
# the update path, against the firmware package (Felucca's tests; needs ./build.sh)
if [ -f "$PKG" ]; then
    run ota-entry sh -c "$CC -o $OUT/ota_test tests/ota_test.c && $OUT/ota_test $PKG"
    if [ -z "${AC79_SDK:-}" ]; then
        echo "  skip update-loader (needs AC79_SDK, as the build)"
    else
    run update-loader sh -c "head -c 200000 build/x0x.bin > $OUT/old_app.bin && \
        python3 tools/fm1pkg_make.py $OUT/old_app.bin build/loader/ota.bin $OUT/old.fwsc >/dev/null && \
        $CC -o $OUT/ldr_test tests/ldr_test.c && $OUT/ldr_test $OUT/old.fwsc $PKG"
    fi
    run installer python3 tests/install_test.py
else
    echo "  skip update-path tests (no build/x0x.fwsc: run ./build.sh)"
fi
run upload-images sh -c "python3 tests/host/make_test_loops.py build/test_loops && \
    python3 tools/upload_breaks.py --dry-run build/test_upload build/test_loops/loop*.wav"
command -v node >/dev/null 2>&1 && run web-installer node web/test_web.mjs
command -v node >/dev/null 2>&1 && run break-connect node tests/host/break_connect_test.mjs
command -v node >/dev/null 2>&1 && run builtin-breaks node tests/host/builtin_breaks_test.mjs
command -v node >/dev/null 2>&1 && run breaks-web node web/test_breaks.mjs   # the browser uploader: upload_breaks.py's bytes
run host-build sh host/build_host.sh
if command -v emcc >/dev/null 2>&1 && command -v node >/dev/null 2>&1; then   # the browser build, when it can be built
    printf 'wav emu_ref.wav\nwait 500\ntapkey b0\ntapkey w0\ntapkey w8\ntap PLAY\nwait 2960\nwavstop\n' > build/host/emu_ref.x0x
    run emu sh -c "sh web/emu/build.sh >/dev/null 2>&1 && build/host/x0x_host build/host/emu_ref.x0x build/host >/dev/null && \
        node tests/host/emu_test.mjs build/emu/x0x.wasm build/host/emu_ref.wav"
    run p9-emu node tests/host/p9_emu_test.mjs build/emu/x0x.wasm
    run bass303-extra-emu node tests/host/bass303_extra_emu_test.mjs build/emu/x0x.wasm
    run emu-slots node tests/host/emu_slots_test.mjs build/emu/x0x.wasm build/test_loops/loop1.wav
    run scheme-emu node tests/host/scheme_emu_test.mjs build/emu/x0x.wasm
    mkdir -p build/scenarios/remix_parity
    run remix-emu sh -c "build/host/x0x_host tests/scenarios/remix_parity.x0x build/scenarios/remix_parity && \
        node tests/host/remix_emu_test.mjs build/emu/x0x.wasm build/scenarios/remix_parity/remix_parity.wav"
    run drum-generator-emu node tests/host/drum_gen_emu_test.mjs build/emu/x0x.wasm
fi
for s in tests/scenarios/*.x0x; do
    n=$(basename "$s" .x0x)
    mkdir -p "build/scenarios/$n"
    run "scenario-$n" build/host/x0x_host "$s" "build/scenarios/$n"
done
[ $FAIL -eq 0 ] && echo "all tests passed" || echo "TESTS FAILED"
exit $FAIL
