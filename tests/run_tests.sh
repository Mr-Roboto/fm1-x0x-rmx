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
run drum-mulaw python3 tests/host/run_drum_mulaw.py
run break-filter sh tests/host/run_break_filter.sh
run env-reset sh tests/host/run_env_reset.sh
run hat-velocity sh tests/host/run_hat_velocity.sh
run bass303-hpf-mirror sh tests/host/run_bass303_hpf_mirror.sh
run live sh tests/host/run_live.sh
run live-fills sh tests/host/run_live_fills.sh
run alpha-juno sh tests/host/run_alpha_juno.sh
if command -v csound >/dev/null 2>&1; then
    run juno-saw-reference python3 tests/host/juno_saw_reference.py
fi
run juno-hardware sh tests/host/run_juno_hardware.sh
run juno-performance sh tests/host/run_juno_performance.sh
run backup sh tests/host/run_backup.sh
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
# the 303, 909 and 808 bit for bit as their golden renders (tests/host/dsp_hash.golden): a speed-up changes no sample
run dsp-hash sh -c "$CC $W -Ifirmware/src/dsp -Ifirmware/src -Ibuild/gen -o $OUT/dsp_hash_test tests/host/dsp_hash_test.c firmware/src/dsp/bass303.c firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c -lm && $OUT/dsp_hash_test tests/host/dsp_hash.golden"
run drum808-bank sh -c "$CC $W -Ifirmware/src/dsp -Ifirmware/src -Ibuild/gen -o $OUT/drum808_bank_test tests/host/drum808_bank_test.c && $OUT/drum808_bank_test"
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
run identity python3 tests/identity_test.py
run flash-ok sh -c "$CC -std=c99 -Wall -Wextra -Werror -o $OUT/flash_ok_test tests/host/flash_ok_test.c && $OUT/flash_ok_test"   # firmware/hal/fm1_flash_ok.h: the flash parts the storage may use
if command -v node >/dev/null 2>&1; then
    run installer-hs node tests/host/installer_hs_test.mjs
fi
# the M-UPGRADE handshake (firmware/src/ota.c): a release's identity cut to NAME_ and three digits for the plain
# query (M-UPGRADE 3.11), the whole one for the query with body 01; a dev build's; the update loader's, whole
for m in long short loader; do
    case $m in long) f="" ;; short) f="-DTEST_ID='\"FM-1_900\"'" ;; loader) f="-DFELUCCA_LOADER=1" ;; esac
    run "ota-hs-$m" sh -c "$CC -std=c11 -O1 -Wall -Wextra -Werror -Wno-unused-function $f -o $OUT/ota_hs_$m tests/ota_hs_test.c && $OUT/ota_hs_$m"
done
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
    # Web now replaces BREAK with JUNO: native BREAK audio/slot parity is not applicable.
    run juno-web-build sh web/emu/build.sh
    run juno-emu node tests/host/juno_emu_test.mjs build/emu/x0x.wasm
    run juno-performance-emu node tests/host/juno_performance_emu_test.mjs build/emu/x0x.wasm
    run juno-sequencer-emu node tests/host/juno_sequencer_emu_test.mjs build/emu/x0x.wasm
    run juno-slides-emu node tests/host/juno_slides_emu_test.mjs build/emu/x0x.wasm
    run bass303-extra-emu node tests/host/bass303_extra_emu_test.mjs build/emu/x0x.wasm
    run bass303-hpf-mirror-emu node tests/host/bass303_hpf_mirror_emu_test.mjs build/emu/x0x.wasm
    run live-emu node tests/host/live_emu_test.mjs build/emu/x0x.wasm
    run live-fills-emu node tests/host/live_fills_emu_test.mjs build/emu/x0x.wasm
    run scheme-emu node tests/host/scheme_emu_test.mjs build/emu/x0x.wasm
    run drum-generator-emu node tests/host/drum_gen_emu_test.mjs build/emu/x0x.wasm
fi
for s in tests/scenarios/*.x0x; do
    n=$(basename "$s" .x0x)
    mkdir -p "build/scenarios/$n"
    run "scenario-$n" build/host/x0x_host "$s" "build/scenarios/$n"
done
[ $FAIL -eq 0 ] && echo "all tests passed" || echo "TESTS FAILED"
exit $FAIL
