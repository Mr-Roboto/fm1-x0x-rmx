# X0X-RMX v1.1.0 development: G.711 cymbal storage

This is the first development component after v1.0.1 Stable, based on
`a3cca394f942d66b7b75a47af3e944ed03728765`, on `dev/v1.1.0-mulaw`.
It is not a public release. No release tag, published package, installer or
Pages content is changed.

## Implementation and audio compatibility

The sample generator and the necessary 909 sampler changes are ported from
Charles Vestal's `charlesvestal/fm1-x0x` commit
`f56a395bb3448a4ea6001a32fecc123514dd8841` (GPL-3.0-only). No other upstream
features are included. `tools/gen_drum_samples.py` is byte-for-byte identical
to that upstream revision.

The existing `assets/909/hh.wav`, `ride.wav` and `crash.wav` are unchanged.
Their lengths remain 20,250, 46,999 and 43,300 frames at 44,100 Hz. The hi-hat's
24-bit source still uses `clip_int16(floor(v24 / 256 + 0.5))` before encoding.
Closed and open hats continue sharing the same sample.

Each sample now occupies one G.711 mu-law byte. A flash-resident, constant
256-entry `int16_t` table decodes both adjacent samples before the existing
linear interpolation. Read positions, fractional rates, pitch controls,
envelopes, choking, distortion, stereo pan and sends are unchanged. There is
no runtime encoder, allocation, decompressed RAM buffer or resampling stage.
The intended audio difference is the same quantization already evaluated in
the PCM/mu-law A/B set: approximately 37.25–37.35 dB signal-to-error ratio.
The generated built-in 909 BREAK loops naturally inherit this cymbal
quantization; their generation algorithm, length and storage size do not change.
User projects, uploaded user samples, persistent layouts and the firmware
identity scheme are unchanged.

Host, emulator and built-in BREAK generation refresh the sample header to
prevent a stale PCM header from being used after switching branches.

## Memory measurements

Both builds use the established JieLi toolchain and
`./build.sh --release 1.1.0-dev`. The baseline is an isolated export of the exact v1.0.1 source,
built with the same development version and identity settings. Its application
size also matches the official v1.0.1 image. Published packages were preserved.

| Resource | Before | After | Capacity / free after |
| --- | ---: | ---: | ---: |
| Application flash | 580,284 B | 470,284 B | 581,564 B / 111,280 B free |
| Static RAM (`.data + .bss`) | 97,336 B | 97,336 B | 98,304 B / 968 B free |
| Pool | 334,348 B | 334,348 B | 344,064 B / 9,716 B free |

**Actual net application flash saving: 110,000 bytes.** Raw sample storage
falls from 221,098 to 110,549 bytes, plus a 512-byte decode table: a 110,037-byte
data saving before the net 37-byte code/alignment cost. The table and all
three encoded arrays were verified against the linked application in flash.
All other generated DSP lookup tables are byte-identical to the baseline.
The development package remains 609,680 bytes because the update container
uses a fixed application slot; package size does not measure flash savings.

The local development package is `build/x0x-1.1.0-dev.fwsc`, displaying
`1.1.0-DEV`, with matching USB/package identity `FM-1_987010100` under the
existing version-encoded scheme. It is not published.

## Reproducing validation

Run from the repository root:

```sh
./build.sh --release 1.1.0-dev
python3 tests/identity_test.py build/x0x-1.1.0-dev.fwsc build/x0x.bin
python3 tests/host/run_drum_mulaw.py --benchmark
NINEW9_DIR=build/v1.1.0-mulaw/reference-9W9 \
X0X_VERSION=1.1.0-DEV X0X_TEST_PACKAGE=build/x0x-1.1.0-dev.fwsc \
AC79_SDK=/home/falke/fw-AC79_AIoT_SDK tests/run_tests.sh
```

The codec test fingerprints all 65,536 signed 16-bit encoder inputs, all 256
decoder entries, the original sources, encoded C arrays and upstream reference
WAVs. The existing local A/B pairs also match byte-for-byte. Frozen fingerprints
remain available when that optional local listening directory is absent.
The PCM oracle needs the v1.0.1 source commit in the local Git object database
(fetch that commit when using a shallow checkout).

264 renderer comparisons against the released PCM renderer, using identical
decoded PCM input, are bit-exact for audio and checked playback state. Cases
cover all four sample voices, three pitches including both endpoints, all seven
distortions, blocks of 1/17/256 frames, retrigger/choke, stereo and sends, and
fractional interpolation at the sample end. Runtime engine structure sizes
also match.

The external 9W9 comparison uses reference revision
`10cbe5c1e1c9e65b0818bb34644ab1a13092082c` (before its er99 file rename).
As in upstream's test change, only its isolated test sample directory receives
the decoded reference WAVs. This keeps the original DSP tolerances intact and
separates codec loss from engine arithmetic; the independent fingerprints
prevent both engines from silently agreeing on an incorrect conversion.

## Validation results

The complete `tests/run_tests.sh` run passed **77 groups**, with **0 failures**
and **2 explicit skips**: the external 808 and Open303 reference comparisons
(the `schwung-8W8` and `schwung-303` checkouts were unavailable). Their local
DSP and UI tests passed. The affected external 9W9 comparison passed with
zero failures, without relaxing thresholds.

Passing checks include ORB Remix, MREV at all six established lengths,
TB3PO, Grids/drum generation, symmetric MORPH knob/SEL notifications,
save/load/copy/Undo, motion, storage, installation/update preservation,
Web MIDI and all simulator scenarios. The emulator rebuilt as `1.1.0-DEV`;
its host parity, Remix, drum-generator and 303 checks passed. Firmware build
checks passed for application capacity, RAM, pool, RAM-resident code,
single-precision arithmetic and register access. The final targeted codec /
renderer check was rerun after test-fixture refinements and passed.

The independent host CPU benchmark used GCC 15.2.0, `-O2 -ffp-contract=off`,
on an Intel Core i7-1280P under WSL. Nine trials per representation alternate
execution order. Each trial renders 16,777,216 frames with open hat, crash and
ride simultaneously active, and uses identical decoded PCM in the old renderer.
Median process CPU time: **0.332137 s PCM**, **0.341554 s mu-law**, or **2.84%
additional time in this host 909 workload**. The sampler adds two table lookups
for each interpolated output while reading sample data. These figures do not
measure whole-application CPU load or FM-1 cycles/real-time headroom.
No physical-device profiling or flashing was performed, and no additional
hardware checklist is introduced by this component.

Local evidence is under `build/v1.1.0-mulaw/` (firmware, matched baseline,
reference, full-suite and targeted-test logs) and
`build/host/drum_mulaw/benchmark.json`. The development package SHA-256 is
`4f77ad53eaf2a2fd7e2802045ad714d43ca9769982bb7d5c44bf64010a81cd49`.
The official v1.0.0 and v1.0.1 package checksums remain unchanged. No known
integration failure remains; external 808/Open303 comparisons and FM-1 cycle
measurements are the stated limits of this validation.
