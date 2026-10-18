# X0X-RMX v1.1.0 — draft release notes / RC1

**Release candidate, not Stable. Physical FM-1 validation is pending.**
The feature set is frozen. Nothing has been released, tagged or deployed for
v1.1.0. v1.0.1 remains the public Stable release.

## Frozen changes

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

ORB Remix, MREV, TB3PO, Grids, the three Morph shapes and the single-Morph
guard remain. ENV reset adds no audio-rendering workload. No other upstream
features are integrated. See the component records for
[µ-law](DEV_1.1.0_MULAW.md), [BREAK FILTER](DEV_1.1.0_BREAK_FILTER.md) and
[ENV reset](DEV_1.1.0_ENV_RESET.md).

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

## Candidate provenance and artifact

Verified linear ancestry, with no upstream merge:

| Source | Commit |
| --- | --- |
| Published v1.0.1 | `a3cca394f942d66b7b75a47af3e944ed03728765` |
| µ-law | `47ca00828bb0689ca404a4ed35d61b0168259e7a` |
| BREAK FILTER | `0800a4a94dfb98a76e12780e5dc8f32d66498557` |
| ENV reset / frozen firmware source | `dd9afd4760cf8df7063b94792f02bc32ed53033a` |

RC branch: `dev/v1.1.0-release-candidate`. Final review required no firmware
source changes. Subsequent RC documentation commits do not change build inputs.

`./build.sh --release 1.1.0` produced `build/x0x-1.1.0.fwsc`, **609,680 B**.
The rebuilt package is byte-for-byte identical to the previously validated
development artifact, including this SHA-256:

```text
1124707f702e57ba97ec0e89d2fdde6751b2efcc2af8250cef69a292c1cee366
```

The uniquely named, read-only hardware candidate is:

```text
build/v1.1.0-rc1/x0x-1.1.0-rc1-1124707f702e.fwsc
```

The filename suffix identifies this local candidate; the embedded version
remains 1.1.0. `SHA256SUMS`, `candidate.json`, build/test logs and the preserved
prebuild package/app/ELF are in `build/v1.1.0-rc1/`. These generated local
artifacts are not committed. Read-only permissions guard against accidental
overwriting; SHA-256 is the authority for byte identity.

**Install and test this exact copy. Keep it for eventual publication without
rebuilding or repackaging.** A changed checksum identifies a different candidate
and invalidates the association with the user's hardware results.

## Capacity and automated validation

| Resource | Used | Capacity | Free |
| --- | ---: | ---: | ---: |
| Application flash | 471,244 B | 581,564 B | 110,320 B |
| Static RAM (.data + .bss) | 97,336 B | 98,304 B | 968 B |
| Pool | 334,360 B | 344,064 B | 9,704 B |

All figures match the validated development artifact. The 609,680-byte package
is a fixed-size update container, not application flash usage. The build checks
capacity, single-precision linkage, RAM code and register-access boundaries.
Explicit package inspection confirms matching USB/package identity and that
the decrypted packaged application equals the built 471,244-byte app.

The final established RC suite passed **81 groups, 0 failures, 2 skips**.
The skips are the unavailable external 808 and Open303 reference checkouts;
their available local tests pass. Coverage includes µ-law codec/sample/renderer
equivalence (all three saved A/B pairs and 264 renderer cases), BREAK
filter bypass and LP/HP response, ENV reset and input priorities, persistence,
legacy loading, Remix, MREV, TB3PO, Grids, Morph, identity, update preservation,
installer, emulator and simulator scenarios. Unavailable external 808/Open303
reference comparisons are reported as skips, not release blockers or passes.

RC validation commands (local artifacts only):

```sh
python3 tests/identity_test.py \
  build/v1.1.0-rc1/x0x-1.1.0-rc1-1124707f702e.fwsc build/x0x.bin
X0X_VERSION=1.1.0 \
X0X_TEST_PACKAGE=build/v1.1.0-rc1/x0x-1.1.0-rc1-1124707f702e.fwsc \
NINEW9_DIR=build/v1.1.0-mulaw/reference-9W9 \
AC79_SDK=/home/falke/fw-AC79_AIoT_SDK tests/run_tests.sh
```

The current Markdown manual and generated HTML are synchronized. Existing
v1.0.0/v1.0.1 release notes and hardware records are not rewritten.

## Physical validation and release boundary

Use the short [FM-1 checklist](FM1_TEST_1.1.0.md). All v1.1.0 hardware results,
including CPU load and two-core operation, are pending; earlier release
observations are not v1.1.0 results. NO CYCLE COUNTER does not prevent this
CPU-percentage comparison. Host timings do not establish FM-1 CPU overhead.

No physical flashing, release tag, GitHub release, Pages deployment or public
installer update is part of RC preparation. Main and published v1.0.0/v1.0.1
artifacts remain unchanged. Wait for the maintainer's hardware results and
explicit publication approval before publishing the exact tested package.
