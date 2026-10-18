# X0X-RMX v1.1.0 development: BREAK FILTER

This component follows the completed G.711 cymbal integration at
`47ca00828bb0689ca404a4ed35d61b0168259e7a` on
`dev/v1.1.0-break-filter`. It is development work, not a public release.

## Port and controls

The DSP and controls come from Charles Vestal's GPL-3.0-only upstream commit
`0417760656494d4994aadbf4cbf4751ede807b6b`. Only that feature is ported;
there is no upstream merge. `breaks.c` and `breaks.h` match that revision
byte-for-byte. The completed mu-law generator, 909 sampler and codec tests
are unchanged.

BREAK EDIT retains its LOOPS page and Pitch mapping. EDIT again, or SELECT,
opens SOUND: FILTER on K1 and LEVEL on K2. SEL also exposes FILTER. The one
parameter ranges from 0 to 127, defaults to 64, and reads LP n%, OFF or HP n%.
Positions 60–68 are exact bypass. Left of that zone is a low-pass; right is
a high-pass. The English manual and its generated HTML describe the new page
as v1.1.0 development functionality.

The filter runs after the existing mono BREAK render, before mixer pan and
sends. It uses upstream's TPT state-variable equations, inverse Q 1.2
(Q = 0.833333…), and cutoff curves:

- LP: `18000 * 2^((value - 60) * 8 / 60)` Hz (about 70 Hz at full left).
- HP: `20 * 2^((value - 68) * 8.5 / 59)` Hz (about 7.24 kHz at full right).

The coefficient moves halfway to its target per rendered block. OFF leaves
the audio buffer untouched and resets filter state. There is no new audio
buffer or allocation. Existing playback, interpolation, pitch, timing,
loop generation, gates, live controls and level calculations are unchanged.

## Project compatibility

`PROJ_SND_REV` advances from 4 to 5. FILTER uses the existing unused sound
pot at `T_BRK/0/2`; Level and Pitch keep indices 0 and 1. Projects with earlier
sound revisions load that pot as 64 (OFF), regardless of its old unused byte.
Revision-5 projects retain their configured filter and filter motion.
Historical short settings without a sound-revision field use revision 0 as
the compatibility default, ensuring the older conversions and new bypass
initialization run. Missing sound objects keep factory OFF.

Persistent sizes stay unchanged: settings 36 B, sound 1,344 B, pattern 528 B,
project 30,952 B, packed-stream raw content 29,572 B. The project object format
and packed-stream format do not change. Existing sound pots, motion lanes,
Remix settings, direction values, patterns and user sample storage are preserved.
The revision-4 test compares the whole project byte-for-byte after loading,
allowing only the sound revision and newly assigned filter byte to change.
This migration does not reinterpret the v1.0.1 direction numbering.

## Memory and package

Both source revisions were built with the established JieLi toolchain and
`./build.sh --release 1.1.0-filter`, using an isolated export for the baseline.
The baseline size also matches the completed mu-law development build.

| Resource | Before | After | Free after |
| --- | ---: | ---: | ---: |
| Application flash (581,564 B capacity) | 470,284 B | 471,036 B | 110,528 B |
| Static RAM (98,304 B capacity) | 97,336 B | 97,336 B | 968 B |
| Pool (344,064 B capacity) | 334,348 B | 334,360 B | 9,704 B |

Net cost: **752 B flash, 0 B static RAM, 12 B pool**. The added parameter fits
existing struct padding; three floats account for the pool growth. The
compiled `brk` symbol grows from 6,272 B to 6,284 B in the pool. The 110,000 B
mu-law flash saving remains; the combined application is 109,248 B smaller
than v1.0.1.

The local development package is `build/x0x-1.1.0-filter.fwsc`, 609,680 B
(the update container has a fixed application slot), displaying `1.1.0-FILTER`
and retaining the existing v1.1.0 identity `FM-1_987010100` for USB and package.
No firmware identity scheme changes are made. Existing v1.0.0, v1.0.1 and
mu-law development packages are untouched.

## Validation

Run from the repository root:

```sh
./build.sh --release 1.1.0-filter
sh tests/host/run_break_filter.sh
BREAK_FILTER_SANITIZE=1 sh tests/host/run_break_filter.sh
python3 tests/host/benchmark_break_filter.py
python3 tests/identity_test.py build/x0x-1.1.0-filter.fwsc build/x0x.bin
X0X_VERSION=1.1.0-FILTER X0X_TEST_PACKAGE=build/x0x-1.1.0-filter.fwsc \
NINEW9_DIR=build/v1.1.0-mulaw/reference-9W9 \
AC79_SDK=/home/falke/fw-AC79_AIoT_SDK tests/run_tests.sh
```

The bypass oracle compiles the actual pre-filter BREAK renderer from the
starting commit (that Git object must be available, including in shallow
checkouts). All nine OFF settings produce bit-identical audio and checked
playback state across three pitches, 90/170 BPM, reverse, half-speed, stutter,
retrigger, pinned slices, gates, a stopped interval/restart, and blocks of
1/17/256 frames. All 128 settings have finite, bounded impulse responses.
LP/HP responses at six cutoff positions and four frequencies match an
independent double-precision transfer function using the intended curves and Q.
Full LP gain at 8 kHz is 0.00006110; full HP gain at 100 Hz is 0.00015786.
Coefficient-glide, bounds, all readouts, knob/SEL, Undo/Redo, motion/base
restoration, persistence and legacy loading tests pass.

The upstream `brkfilter.x0x` scenario is adapted to this simulator's existing
`sound3.0.2` assertions: 64 initially, 34 after turning left, 94 after turning
right. It retains the upstream page/readout screenshots. The historical LCD
regression still compares every original LOOPS pixel after normalizing only
the intentional extra page indicator; historical golden hashes are unchanged.

An additional sanitizer run exposed an existing negative signed shift in
`fm_exp2_nf`, used by the LP curve. Its bit adjustment now uses unsigned
arithmetic. Address/undefined-behavior sanitizer checks of the filter and the
full fastmath test pass without suppressions. 402,002 sampled exponent outputs
are bit-identical to the old helper, and rebuilding the firmware after this
change produces a byte-identical application image. No DSP equation or sample
representation changes as a result.

## Final results and host CPU timing

The final complete suite passed **79 groups**, **0 failed**, **2 skipped**.
The skips are the external 808 and Open303 reference comparisons, whose
`schwung-8W8` and `schwung-303` checkouts are absent; their available local
DSP/UI checks pass. The external 9W9 comparison passes. Passing regressions
include ORB Remix, MREV, TB3PO, Grids/drum generation, Morph, storage/update
preservation, browser installer, all emulator checks and all simulator
scenarios, including the adapted upstream filter scenario. The final targeted
filter test was rerun after matching baseline/current compilation visibility
for the CPU benchmark and still passes. No golden image hashes or audio
comparison tolerances were weakened.

The optional host benchmark uses GCC 15.2.0, `-O2 -ffp-contract=off`, on an Intel
Core i7-1280P under WSL. Both renderers are compiled with identical flags and
source visibility. Nine trials rotate the order of baseline, OFF, full LP and
full HP. Each renders 16,777,216 frames of continuously active BREAK audio;
loop setup and file I/O are outside the timed region.

| BREAK renderer | Median host process CPU time | Relative to baseline |
| --- | ---: | ---: |
| Pre-filter baseline | 0.085499 s | 1.00x |
| OFF | 0.084648 s | 0.99x (within measurement variation) |
| Full LP | 0.227018 s | 2.66x (+165.5%) |
| Full HP | 0.230129 s | 2.69x (+169.2%) |

There is no measurable OFF-path increase in this run. Active filtering adds
roughly 0.14–0.145 s of host CPU time per 16.78 million output frames in this
isolated BREAK workload. These are **BREAK-stage host timings**, not total
application CPU percentages or FM-1 cycle/real-time headroom measurements.
No physical device was flashed or profiled. This component adds no hardware
release prerequisite.

Evidence is local under `build/v1.1.0-break-filter/` (build, matched baseline,
full suite, scenario, sanitizer and identity logs) and
`build/host/break-filter/benchmark.json`. The development package SHA-256 is
`e09af4e471639a6e6d60f65b7afa1c79e4a1f306a26c25c672b40f48dcec33f2`.
The v1.0.0, v1.0.1 and earlier mu-law development package hashes are unchanged.
No known integration failure remains; the two absent external comparisons
and unmeasured device cycle cost are the stated limits of validation.
Main, release tags, published packages, installer and Pages are untouched.
