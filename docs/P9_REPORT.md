# P9 Final Implementation — X0X-RMX v1.0.0 Stable

P9 firmware commit: `4694051a499aeaceb84692a0d195463029bf94b0` on `orb-remix`.
Implementation baseline: `e5cf961ccb23f4a8834869b0af6e98c9acde30fb`.
`main` remains at `47f8e4c4ad602591b81c21bc5480db7a94f6d6af`.

The maintainer approved publication on 16 October 2026 after confirming a regular update
from FM-1_900, preserved patterns and user breaks, restoration of the five manual startup
patterns, chain playback, a twelve-minute musical run without reported problems, and USB
audio with iPad/AudioShare. Real-device sequencer, drum, bass synth, effects and breakbeat
engine operation was also confirmed. Separate FULL/FOLLOWS KNOB hardware checks were not reported
and are not an additional release condition. See [the hardware record](P9_FM1_TEST.md).
The tested package is published unchanged; publication makes no firmware change or rebuild.

## Implementation decisions

- **P9-04:** `project_defaults()` selects USB FULL. Factory Reset applies settings to the
  engine. Loading saved settings over the defaults retains an existing FOLLOWS KNOB or FULL
  choice. Both choices remain available. Automated checks cover defaults and saved choices;
  the physical confirmation covers USB audio with iPad/AudioShare without separate mode claims.
- **P9-05:** Five `start_pat[5]` bytes are appended to `settings_t`, after `rsv[0]`.
  Only manual PRESETS/HOME selection updates them and marks the project dirty. The existing
  SAVE/AUTOSAVE object/hash mechanism writes them; pattern changes do not write flash.
  Requested manual choices are remembered even before a playing cue takes effect. Song,
  automatic chains and performance tests use the sequencer directly and cannot overwrite the
  startup bytes. A HOME chain remembers its first manual choice; the second held key defines
  the endpoint. Boot restores the combination through existing cues.
- **Storage:** Settings grow 28 → 36 B (five data bytes plus alignment), project RAM
  30,944 → 30,952 B. Existing settings offsets remain unchanged; Remix APPLY still occupies
  `rsv[0]` at offset 27. Startup fields occupy offsets 28–32, with zero-based values 0–31.
  Missing fields default to P1; invalid values are bounded to P1. The settings object grows
  32 → 40 B including its format word, without a new object or sector. `pattern_t` stays
  528 B and the format-2 pattern/song/motion raw stream stays 29,572 B. Format-1 migration
  remains available. New firmware reads old projects; lossless downgrade of the longer
  settings object to older firmware is not guaranteed.
- **P9-07:** 909 GR and 808 GR replace the two former 909 loops. Both are two bars at 96 BPM,
  110,250 mono samples at 22,050 Hz, encoded as 4-bit IMA ADPCM: 55,125 B each. The build-time
  host generator links the existing 808 renderer. Two warm-up cycles carry tails across the
  seam; 32-sample edge ramps and 85% normalization avoid hard cuts/clipping. There is no boot
  rendering and no new target engine.
- **P9-08/09:** Both 303s use SEQUENCE → REMIX → KEYS; KEYS has FREE PLAY only. All four
  Remix parts expose APPLY / BANK / PATTERN / REMIX left to right. DSP, Remix algorithms,
  pattern parameter formats and chronological timing remain unchanged.
- **Uploader:** The existing filter additionally accepts narrow FM-1 names including
  `FM-1 Midi 2`. Identically named input/output pairs and the existing X0X SysEx handshake
  are still required. Unrelated names are not probed; wrong headers/commands or missing replies
  do not identify a device. The upload protocol and sample layout are unchanged.
- **Stable versioning:** Regular versions no longer receive an implicit status suffix.
  The product-ID scheme and SDK, loader, image, RAM, pool, MMIO and soft-double checks remain.
  Explicit historical suffixes remain explicit; this package is 1.0.0 Stable.
- **Test maintenance:** The C99 host build gets its required POSIX feature definition.
  Structure/layout and control-order expectations were updated. Historical LCD pixel tests
  normalize only the intentionally renamed second loop. `X0X_TEST_PACKAGE` selects the exact
  stable package. Missing external reference checkouts produce explicit skips; present
  reference comparisons still fail normally on errors. The inherited web test skips the absent
  Felucca editor but checks X0X package parsing and the existing browser OTA protocol.

## Budgets and artifacts

| Measurement | Baseline e5cf961 | Final 1.0.0 | Reserve / limit |
|---|---:|---:|---:|
| App image | 579,820 B | 579,980 B | 1,584 B to 581,564 B (`0x8DFBC`) |
| Static RAM .data+.bss | 97,324 B | 97,332 B | 972 B to 98,304 B |
| Audio pool | 334,348 B | 334,348 B | 9,716 B to 344,064 B |
| Pool reserve above required 8 KiB | 1,524 B | 1,524 B | Build check passed |
| Built-in ADPCM payload | 110,250 B | 110,250 B | Unchanged |
| OTA package | 609,680 B | 609,680 B | Fixed package layout |
| Loader ota.bin | 6,524 B | 6,524 B | Existing implementation |

Flash delta +160 B; RAM delta +8 B. No feature was removed and no last-minute DSP
optimization was introduced. The small remaining flash/RAM reserves remain a development
constraint; all existing build limits pass.

- Package: `build/x0x-1.0.0.fwsc`; product `FM-1_9010000`; display `X0X 1.0.0`; tag `v1.0.0`.
- SHA-256: `eccacfa995efec6b9c819583e7bce43b3aa7a126033c8e668ae89c6a988b754c`.
- Firmware build log: `build/host/p9/stable-build-final.log`.
- Baseline build log: `build/host/p9/baseline-build.log`.
- P9 WASM SHA-256: `1e11309b6e47bd1081cc1af798ee31a99af37e67f9f60d0a8747b17ebf43e030`.
  Core provenance is retained in `build/emu/build-info.json`. Release website text can be
  updated without changing this tested binary.
- Public package and installation links: [release notes](RELEASE_1.0.0.md).

## Project and sample compatibility

Actual host save/load and migration tests cover historical settings, format-1/format-2
projects, Remix/APPLY, schemes, song and motion. Manual startup state is isolated from
playback state. SAVE during PLAY still waits for STOP; AUTOSAVE retains its idle delay and
twenty-second gap. The maintainer additionally confirmed project/sample retention and startup
pattern restoration on the physical device.

The three user slots remain at `0xA0000`, `0xB4000`, `0xC8000`, each `0x14000` B, with data
offset 512 and unchanged headers/protocol. `slots.c`, upload write paths, package layout and
loader write windows were not changed. The loader test checks sentinels over every byte from
`0xA0000` through `0xDBFFF`; a regular update preserves them, and an identical repeat update
erases nothing. The physical test confirmed retention of existing user breaks. Explicit
uploader erase/replace operations intentionally write their selected slots.

## Automated results

The full P9 run used:

```sh
AC79_SDK=/home/falke/fw-AC79_AIoT_SDK \
X0X_TEST_PACKAGE=build/x0x-1.0.0.fwsc X0X_VERSION=1.0.0 tests/run_tests.sh
```

**74 available test groups/scenarios passed; three external DSP comparisons were skipped.**
No available test failed in the final run. Logs are in `build/host/`; the complete summary
is `build/host/p9/regression-final.log`:

```text
ok   tb3po
  ok   drum-generator
  ok   drum-generator-project
  ok   drum-generator-ui
  ok   remix
  ok   remix-state
  ok   schemes
  ok   fastmath
  ok   sequencer
  ok   encoder
  ok   bass303-ui
  ok   bass303-waveform
  ok   bass303_lite
  ok   uac
  ok   trs
  ok   undo
  ok   lz
  ok   motion
  ok   drum-pan
  ok   stereo
  ok   master
  skip drum909 (external reference unavailable: ../schwung-9W9/src/dsp/er99_engine.c)
  skip drum808 (external reference unavailable: ../schwung-8W8/src/dsp/sc808_engine.cpp)
  skip bass303 (external reference unavailable: ../schwung-303/src/dsp/open303/rosic_Open303.cpp)
  ok   breaks
  ok   storage
  ok   ota-entry
  ok   update-loader
  ok   installer
  ok   upload-images
  ok   web-installer
  ok   break-connect
  ok   builtin-breaks
  ok   breaks-web
  ok   host-build
  ok   emu
  ok   p9-emu
  ok   bass303-extra-emu
  ok   emu-slots
  ok   scheme-emu
  ok   remix-emu
  ok   drum-generator-emu
  ok   scenario-autosave
  ok   scenario-extclock
  ok   scenario-freeplay
  ok   scenario-getting_started
  ok   scenario-groove
  ok   scenario-guard
  ok   scenario-help
  ok   scenario-jam
  ok   scenario-keysound
  ok   scenario-knobs
  ok   scenario-migrate
  ok   scenario-migrate0102
  ok   scenario-patterns
  ok   scenario-perf
  ok   scenario-plock
  ok   scenario-prob
  ok   scenario-probcond
  ok   scenario-probreset
  ok   scenario-remix_motion
  ok   scenario-remix_parity
  ok   scenario-remix_recording
  ok   scenario-remix_ui
  ok   scenario-savewait
  ok   scenario-showcase
  ok   scenario-slices
  ok   scenario-song
  ok   scenario-songrep
  ok   scenario-step303
  ok   scenario-steps64
  ok   scenario-stereo
  ok   scenario-tb3po
  ok   scenario-tour
  ok   scenario-trackmute
  ok   scenario-undo
  ok   scenario-upload
all tests passed
```

Additional successful checks:

- `tests/release_test.py`: exact stable product/display, loader marker, app limit,
  emulator version and hash (`build/host/p9/release-artifacts.log`).
- `tests/pages_test.py`: links, anchors, images/video, package-free preview gate and WASM
  provenance (`build/host/p9/pages-final.log`). Release-mode website checks additionally
  validate the exact package/hash, active installer metadata and stable release routes.
- `tests/tb3po_scales_doc_test.py`: all 22 scales, names and note spellings.
- Real Chromium device-palette checks: mouse/touch/keyboard, audio, Morph/Remix/Grids,
  persistence, mobile layout, favicon and the then-reserved release gate. No page errors;
  measured audio peak 0.273466. Results: `build/host/p9/device-browser/results.json`.
- Real Chromium factory-scheme checks: startup, SAVE/restart, retained choices, historical/
  short settings, firmware/browser reset and independent exterior color. No page errors.
  Results: `build/host/p9/factory-browser/results.json`.
- Three real Chromium Remix captures: 909 A1/OFF/BEAT, 909 B3/ON/BAR and 303A C4/ON/LOOP.
  Both 303 KEYS pages and all four Remix pages also match the shared host renderer in WASM.
- `git diff --check`: no whitespace errors.

Earlier failed attempts remain in the local logs: old test sizes/control mappings, a missing
POSIX feature definition, sandbox EPERM on child processes, and the inherited test for the
absent `web/editor.html`. Those causes were corrected before the successful final run.
The Felucca editor is not part of X0X; its checks are explicitly inapplicable, while package
parsing and browser OTA pass. External 9W9, 8W8 and schwung-303 checkouts were unavailable.
The actual DSP engines were unchanged by P9. The inherited `tests/target_budget.py` has no
Felucca budget dataset or matching X0X function set here and is not part of the X0X suite;
the target build's image/RAM/pool/soft-double/MMIO checks ran successfully.

## Built-in break measurements

| Loop | Decoded 16-bit peak | RMS | Loop-boundary difference |
|---|---:|---:|---:|
| 909 GR | 25,916 | 4,798.5 | 10 |
| 808 GR | 28,288 | 3,227.3 | 4 |

Generation is deterministic; duration, headroom and distinct rhythmic envelopes pass (envelope
distance 2,961.3). Listening WAVs are in `build/host/p9/909-groove.wav` and `808-groove.wav`.
These measurements do not claim a separate subjective listening test of each loop or an
exhaustive hardware benchmark.

## Hardware record and publication

The maintainer's confirmed results and explicit release approval are recorded at the top of
this report and in [P9_FM1_TEST.md](P9_FM1_TEST.md). No unreported individual mode, Song,
AUTOSAVE, Factory Reset, Morph stress or platform-specific uploader test is marked as passed.
Separate FULL/FOLLOWS KNOB checks are not a publication condition.

The release uses the existing hardware-tested package, with no firmware change/rebuild.
Publish `orb-remix`, the `v1.0.0` tag, a stable GitHub release and the history-preserving
`gh-pages` update; never force-push or modify `main`. Verify the public package checksum,
installer metadata, stable version, website routes and tested emulator binary after deployment.
The release publication verification record is `build/host/v1-release/`.

The video remains identified as an earlier browser-emulator recording. The outro text was
corrected for release to state successful real-device validation. The first 81.8 seconds are
frame-identical, only the final 2.2-second encoding group is re-encoded, the full AAC audio
stream is unchanged, and the approved banner/poster are unchanged. Historical
upstream version names remain historical attribution, not labels for this stable release.

## P9 changed files

The four pre-existing untracked user files (`docs/P82_SCREENSHOT_REVIEW.md`,
`docs/P83_VIDEO_PLAN.md`, the root banner and its Zone.Identifier) remain untouched and
excluded from commits. Generated artifacts/logs remain under `build/`.

- `BUILDING.md`
- `README.md`
- `docs/MANUAL.md`
- `docs/NEW.md`
- `docs/P9_FM1_TEST.md`
- `docs/P9_REPORT.md`
- `docs/PAGES.md`
- `docs/RELEASE_1.0.0.md`
- `docs/REMIX_DESIGN.md`
- `docs/img/p82/05-remix-909-off-a1.png`
- `docs/img/p82/06-remix-909-on-b3.png`
- `docs/img/p82/19-remix-303a-on-c4.png`
- `docs/img/p82/README.md`
- `docs/manual.html`
- `firmware/src/app/project.c`
- `firmware/src/app/ui.c`
- `firmware/src/app/x0x.h`
- `host/build_host.sh`
- `tests/host/break_connect_test.mjs`
- `tests/host/builtin_breaks_test.mjs`
- `tests/host/drum_gen_test.c`
- `tests/host/drum_gen_ui_test.c`
- `tests/host/p9_emu_test.mjs`
- `tests/host/p9_test.c`
- `tests/host/remix_state_test.c`
- `tests/host/remix_ui_test.c`
- `tests/host/run_remix_state.sh`
- `tests/host/scheme_ui_test.c`
- `tests/host/tb3po_ui_test.c`
- `tests/install_test.py`
- `tests/ldr_test.c`
- `tests/pages_test.py`
- `tests/release_test.py`
- `tests/run_tests.sh`
- `tests/scenarios/freeplay.x0x`
- `tests/scenarios/migrate.x0x`
- `tests/scenarios/remix_motion.x0x`
- `tests/scenarios/remix_parity.x0x`
- `tests/scenarios/remix_recording.x0x`
- `tests/scenarios/remix_ui.x0x`
- `tools/build.py`
- `tools/gen_builtin_break.c`
- `tools/gen_builtin_break.sh`
- `tools/make_pages.py`
- `web/emu/build.sh`
- `web/test_web.mjs`
- `web/x0x_breaks.html`
