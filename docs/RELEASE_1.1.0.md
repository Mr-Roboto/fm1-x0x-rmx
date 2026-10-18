# X0X-RMX v1.1.0 — draft release notes / RC2

**Release candidate, not Stable. Physical FM-1 validation is pending.**
The four-feature candidate is frozen for evaluation. USB backup remains an
experimental inclusion pending the maintainer's physical results and final decision.
Nothing has been released, tagged or deployed for
v1.1.0. v1.0.1 remains the public Stable release.

The [original RC1 release record](RELEASE_1.1.0_RC1.md),
[RC1 hardware checklist](FM1_TEST_1.1.0.md), branch and immutable package are preserved.

## Candidate changes

- **TR-909 cymbal compression:** Charles Vestal's G.711 µ-law encoder and
  flash-resident decode table store the built-in hi-hat, ride and crash in
  eight bits per sample. Original sources, sample counts, 44.1 kHz rate,
  hi-hat 24-to-16-bit rounding, interpolation, pitch and envelopes are retained.
  This is lossy sample compression, not a change to the remaining 909 engine.
- **BREAK FILTER:** one bipolar control, LP to the left, OFF in the center,
  HP to the right. BREAK > EDIT > SOUND places Filter on KNOB 1 and Level on
  KNOB 2; SEL also exposes Filter. Default 64 and positions 60–68 bypass the
  filter exactly. Upstream's TPT design, cutoff curves, Q and smoothing remain.
- **ENV + KNOB 1–4:** hold ENV and turn a parameter knob to reset to its
  descriptor default. An already-default value stays unchanged. Held-step
  editing, parameter locks and SAVE + knob keep their priorities. Accepted
  changes follow existing motion, Undo/Redo, dirty-state and Morph ownership
  paths. BREAK Filter resets to 64/OFF.
- **USB project backup/restore:** local Chrome/Edge Web MIDI page and Python
  client export saved settings, sounds, patterns, song and motion. Playback
  backup exports saved data; stop first to include unsaved edits. Restore
  requires stopped playback and explicit **SEL** confirmation on the FM-1;
  **HOME** cancels. Small streamed chunks, strict validation and a verified
  journal protect project activation. Uploaded loops and calibration are
  neither included nor overwritten. No whole-project RAM buffer is added.

ORB Remix, MREV, TB3PO, Grids, the three Morph shapes and the single-Morph
guard remain. ENV reset adds no audio-rendering workload. No other upstream
features are integrated. See the component records for
[µ-law](DEV_1.1.0_MULAW.md), [BREAK FILTER](DEV_1.1.0_BREAK_FILTER.md),
[ENV reset](DEV_1.1.0_ENV_RESET.md), and the
[USB backup implementation and safety record](DEV_1.1.0_USB_BACKUP.md).

## Compatibility

Firmware and package identity are **`FM-1_987010100`**, retaining the existing
X0X-RMX numeric/version identity scheme. The displayed version is **1.1.0**.

Sound revision advances from 4 to 5. Older projects load BREAK Filter as
64/OFF; revision-5 projects preserve their filter value and motion. Filter
occupies an existing unused sound byte. Settings, sound, pattern and project
sizes remain 36, 1,344, 528 and 30,952 bytes respectively; the packed raw
stream remains 29,572 bytes. Existing project, pattern, motion, Remix and
user-sample data follow the existing preservation paths. Automated migration
and update tests pass; v1.1.0 physical confirmation remains pending.

**The v1.0.1 direction compatibility caveat still applies to upgrades directly
from v1.0.0:** FWD=0, REV=1, MREV=2, PING=3, RND=4, without direction migration.
Old v1.0.0 PING (2) becomes MREV; old RND (3) becomes PING. Reselect and save
affected directions. Projects already using v1.0.1 keep their direction values.
The [historical v1.0.1 notes](RELEASE_1.0.1.md) are unchanged.

USB backup files use the **RMX profile `0x524D`**, not the standard X0X profile.
Matching object IDs or an X0XB header does not imply compatibility: standard
X0X and other variants are rejected. Supported RMX sound revisions 0–5 load
through existing migrations; malformed, partial or unsupported layouts fail
validation. For older project formats, load and save with this firmware before
backing up. Grids' written patterns are saved; session-only generator settings,
Undo history and unsaved playing edits are not. Keep uploaded loop WAVs
separately; a backup retains slot references, not sample audio.

## Candidate provenance and artifact

Verified linear ancestry, with no upstream merge:

| Source | Commit |
| --- | --- |
| Published v1.0.1 | `a3cca394f942d66b7b75a47af3e944ed03728765` |
| µ-law | `47ca00828bb0689ca404a4ed35d61b0168259e7a` |
| BREAK FILTER | `0800a4a94dfb98a76e12780e5dc8f32d66498557` |
| ENV reset | `dd9afd4760cf8df7063b94792f02bc32ed53033a` |
| RC1 | `1e78336dc7d1948a47ea95b1df0032aff70528ea` |
| USB backup / RC2 firmware source | `77fc6692e6d875fe6970e85b4c057b43f4056515` |

RC2 branch: `dev/v1.1.0-rc2`, created directly from the USB-backup commit.
RC2 preparation changes documentation only; firmware and build inputs are unchanged.

`./build.sh --release 1.1.0` produced `build/x0x-1.1.0.fwsc`, **609,680 B**.
The verification rebuild is byte-for-byte identical to the existing experimental
package. RC2 uses a read-only copy of that original package, not a repack:
`build/v1.1.0-usb-backup/x0x-1.1.0-usb-backup-b343c135779c.fwsc`.
SHA-256:

```text
b343c135779c1de933d655236b14628103816036e170d65faed706aba620dcaa
```

The uniquely named, read-only hardware candidate is:

```text
build/v1.1.0-rc2/x0x-1.1.0-rc2-b343c135779c.fwsc
```

The filename suffix identifies this local candidate; the embedded version
remains 1.1.0. `SHA256SUMS`, `candidate.json`, build/test logs and the
local preview are in `build/v1.1.0-rc2/`. These generated local
artifacts are not committed. Read-only permissions guard against accidental
overwriting; SHA-256 is the authority for byte identity.

**Install and test this exact copy. Keep it for eventual publication without
rebuilding or repackaging.** A changed checksum identifies a different candidate
and invalidates the association with the user's hardware results.

## Capacity and automated validation

| Resource | RC2 used | Capacity | RC2 free | Change from RC1 |
| --- | ---: | ---: | ---: | ---: |
| Application flash | 476,940 B | 581,564 B | 104,624 B | +5,696 B |
| Static RAM (.data + .bss) | 97,336 B | 98,304 B | 968 B | 0 B |
| Pool | 335,332 B | 344,064 B | 8,732 B | +972 B |

All figures match the experimental USB-backup build. The 609,680-byte package
is a fixed-size update container, not application flash usage. The build checks
capacity, single-precision linkage, RAM code and register-access boundaries.
Explicit package inspection confirms matching USB/package identity and that
the decrypted packaged application equals the built 476,940-byte app.
The existing 8,192-byte minimum pool-headroom gate passes with 540 bytes
to spare. Static RAM remains tight; real USB/audio timing is pending hardware.
Backup adds no per-sample DSP work. Stopped restore briefly pauses audio
rendering for commit/reload; backup runs cooperatively in the main loop.

The final established RC suite passed **82 groups, 0 failures, 2 skips**.
The skips are the unavailable external 808 and Open303 reference checkouts;
their available local tests pass. Coverage includes µ-law codec/sample/renderer
equivalence (all three saved A/B pairs and 264 renderer cases), BREAK
filter bypass and LP/HP response, ENV reset and input priorities, persistence,
legacy loading, Remix, MREV, TB3PO, Grids, Morph, identity, update preservation,
installer, emulator and simulator scenarios. USB coverage includes RMX data
round-trip, revision-5/older loading, malformed CRC/LZ/container rejection,
HOME cancellation, transfer interruptions, simulated commit recovery, sample/
calibration preservation, CLI and browser protocol conversations. The local
experimental Pages preview passes its existing validation checks. Unavailable
external 808/Open303 reference comparisons are reported as skips, not release
blockers or passes.

RC validation commands (local artifacts only):

```sh
python3 tests/identity_test.py \
  build/v1.1.0-rc2/x0x-1.1.0-rc2-b343c135779c.fwsc build/x0x.bin
X0X_VERSION=1.1.0 \
X0X_TEST_PACKAGE=build/v1.1.0-rc2/x0x-1.1.0-rc2-b343c135779c.fwsc \
NINEW9_DIR=build/v1.1.0-mulaw/reference-9W9 \
AC79_SDK=/home/falke/fw-AC79_AIoT_SDK tests/run_tests.sh
```

The current Markdown manual and generated HTML are synchronized. Existing
v1.0.0/v1.0.1 release notes and hardware records are not rewritten.

## Physical validation and release boundary

Use the [RC2 FM-1 checklist and local preview instructions](FM1_TEST_1.1.0_RC2.md).
All v1.1.0 hardware results,
including CPU load and two-core operation, are pending; earlier release
observations are not v1.1.0 results. NO CYCLE COUNTER does not prevent this
CPU-percentage comparison. Host timings do not establish FM-1 CPU overhead.

No physical flashing, release tag, GitHub release, Pages deployment or public
installer update is part of RC preparation. Main and published v1.0.0/v1.0.1
artifacts remain unchanged. Wait for the maintainer's hardware results and
final USB-feature inclusion decision and explicit publication approval before
publishing the exact tested package. No hardware result is implied by automated tests.
