# X0X-RMX v1.1.0 development: ENV + knob default reset

This component starts at the completed BREAK FILTER commit
`0800a4a94dfb98a76e12780e5dc8f32d66498557`, on `dev/v1.1.0-env-reset`.
It ports Charles Vestal's GPL-3.0-only upstream shortcut from
`102c47b9e9a4c5ac499b03698ed990a2599744f2`, without other upstream features.
It is development work, not a public release.

## Behavior

Hold ENV and turn KNOB 1–4 in either direction to reset the parameter shown
under that knob to its existing `x0x_param_t.def`. This is the defined parameter
default, which can differ from X0X-RMX's factory mix. For example, the 909
kick's factory Decay is 80, while its descriptor default is 90. No default
lookup table, persistent field, queue command type or DSP state is added.

The shortcut uses existing preference setters. A Wave reset uses X0X-RMX's
existing `engine_bass_wave_user()` queue path, keeping the single-Morph guard
and accepted-wave mirror. Wave resets to Saw (0); the Morph shape parameter
resets to 64, preserving its existing three shapes and the selected waveform.
Normal rejected Morph selections retain the symmetric `MORPH IN USE` warning.
BREAK FILTER resets to 64/OFF. Remix uses the existing descriptors and setters:
Bank A, Pattern 1, enable OFF, APPLY BEAT, each independent of the other fields.
Direction resets to FWD without discarding Remix selection or enable state.
TB3PO's custom defaults and Grids' session-only settings keep their own paths.

Input priorities are unchanged: parameter locks and held 303 step edits run
first, followed by SAVE + knob motion removal, then ENV reset, then ordinary
knob movement. Questions and performance tests still block the knobs. SELECT,
ALGORITHM and PRESETS keep their existing functions. ENV's accents, keyboard
notes and ENV + OCT rest entry are preserved. The ENV hold-help and English
manual controls table describe the shortcut; manual HTML is synchronized.

A changed reset calls `motion_touch()` as upstream does: while recording it
requests recording (allocating a lane when needed), or otherwise holds an
existing lane for a pass. An already-default parameter does nothing: no dirty
flag, command, motion touch or persistent modification. Blank controls and
actions do not execute a reset. Existing Undo/Redo and save/load paths apply.

## Build and memory

The baseline and new sources were built with the established JieLi toolchain
and identical `./build.sh --release 1.1.0` settings. The baseline is
an isolated export of the exact starting commit. No existing package is
rebuilt or replaced. The previous component's 471,036 B application used the
longer `1.1.0-FILTER` label; its matching numeric-version baseline is 16 B
smaller. Relative to that earlier artifact, the new image grows by 208 B;
the reset feature itself costs 224 B under identical build settings.

| Resource | Before | After | Change | Free after |
| --- | ---: | ---: | ---: | ---: |
| Application flash (581,564 B capacity) | 471,020 B | 471,244 B | +224 B | 110,320 B |
| Static RAM (98,304 B capacity) | 97,336 B | 97,336 B | 0 B | 968 B |
| Pool (344,064 B capacity) | 334,360 B | 334,360 B | 0 B | 9,704 B |

All twelve `build/o2/*.o` files (DSP, engine and sequencer/motion units) are
byte-identical to the matching baseline. No audio-rendering workload or DSP
behavior is introduced. The shortcut runs only in the UI input path when a
parameter knob moves. The existing mu-law implementation, BREAK FILTER,
sound revision 5 and persistent layouts are unchanged.

The local package is `build/x0x-1.1.0.fwsc`, 609,680 B (fixed-size
update container), displaying `1.1.0`. USB and package retain the
existing development identity `FM-1_987010100`; the identity scheme is unchanged.
SHA-256: `1124707f702e57ba97ec0e89d2fdde6751b2efcc2af8250cef69a292c1cee366`.

## Validation

```sh
./build.sh --release 1.1.0
sh tests/host/run_env_reset.sh
python3 tests/identity_test.py build/x0x-1.1.0.fwsc build/x0x.bin
X0X_VERSION=1.1.0 X0X_TEST_PACKAGE=build/x0x-1.1.0.fwsc \
NINEW9_DIR=build/v1.1.0-mulaw/reference-9W9 \
AC79_SDK=/home/falke/fw-AC79_AIoT_SDK tests/run_tests.sh
```

The focused test drives actual input, command consumption, preferences, motion,
Undo and simulated project storage. It covers all four knob positions and both
turn directions, 909/808/303/BREAK/FX/mix defaults, both Waves and Morph ownership,
shape endpoints returning to 64, all Remix fields, MREV to FWD, TB3PO and Grids,
no-op/blank controls, blocked questions, recording and holding motion, creating
a recorded lane, Undo/Redo, persistence, all editing priorities, ENV accents,
keyboard/rest behavior and help fit. The upstream `envreset.x0x` scenario and
its host-only `knobdefK` expectation are included unchanged.

An initial emulator regression rejected the long temporary `1.1.0-ENVRESET`
label in ABOUT's existing fixed-size message. The final build uses the ordinary
`1.1.0` version, and that check passes without modifying ABOUT or weakening the
test. The rejected candidate is retained only under the ignored local evidence
directory as `rejected-long-label.fwsc`; it is not the validated package.

The final complete suite passed **81 groups**, with **0 failures** and
**2 skips**: the external 808/Open303 reference comparisons, whose source
checkouts are absent. Their available local DSP/UI tests pass. The external
9W9 comparison, mu-law sample/renderer equivalence, BREAK FILTER tests,
MREV, Remix, Morph notifications, TB3PO, Grids, storage/update preservation,
Web MIDI installer, all emulator checks and all simulator scenarios pass.
No test tolerance or existing assertion was weakened. A final focused run also
covers recording a reset into a newly allocated motion lane.

Evidence is under
`build/v1.1.0-env-reset/`. Main, Pages, release tags and existing firmware
packages are untouched. No physical device is flashed and no new hardware
release prerequisite is introduced.
