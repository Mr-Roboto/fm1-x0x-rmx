# X0X-RMX v1.0.1 — development candidate

Status: **unreleased; hardware checklist items 1–8 passed; explicit Stable release approval pending**.
External MIDI clock/dedicated extended performance testing and physical
interrupted-update recovery remain untested. Development branch:
`dev/independent-firmware-identity`; documentation committing and a normal
development-branch push are approved. No upstream X0X changes are included;
upstream integration is reserved for v1.1.0.

## Changes

- Independent namespace `FM-1_987`; this candidate reports **`FM-1_987010001`**
  in both the `.fwsc` package and the running firmware's USB identity. See the
  [identity registry check and update design](FIRMWARE_IDENTITY.md).
- Five 303 playback directions in exact UI/stored order: **FWD → REV → MREV →
  PING → RND**. Musical Reverse (MREV) starts on step 1, then visits the other
  configured steps backwards. Length 16 plays `1 → 16 → 15 → … → 2 → 1`;
  lengths 1–64 use the same rule over the entire pattern. Length 1 repeats 1.
- MREV bypasses ORB Remix, preserving its selection and enabled state. Forward
  restores Remix under the existing APPLY rules. TIE/slide lookahead follows
  the backward path. MREV counts one probability/auto-mutation cycle per full
  configured-length pass. REV's existing last-step start and wrap behavior,
  and PING/RND traversal algorithms, are unchanged.
- A rejected user WAVE selection shows exactly **MORPH IN USE** for 1.3 seconds
  in the header, for either voice and through both the knob and SEL list.
  The previous accepted waveform remains selected. The unchanged audio-side
  single-MORPH rule decides acceptance; ordinary playback, motion, direct
  parameter commands and project loading do not produce these notifications.

## Deliberate saved-direction compatibility change

No UI mapping or direction-data migration is used. Existing stored bytes retain
their values but are interpreted by the new enumeration:

| Stored byte | v1.0.0 | v1.0.1 |
| --- | --- | --- |
| 0 | FWD | FWD |
| 1 | REV | REV |
| 2 | PING | **MREV** |
| 3 | RND | **PING** |
| 4 | Not a v1.0.0 direction | **RND** |

After updating, reselect the desired direction on affected patterns and save.
If returning to v1.0.0, reselect directions there too: v1.0.1 values 2–4 do not
represent the same direction in v1.0.0. Back up projects before hardware trials.
Pattern layout remains unchanged: `bpart_t.dir` is still `uint8_t`, `bpart_t`
is 144 B, `pattern_t` 528 B and `project_t` 30,952 B. No persistent fields were
added. Saved projects and uploaded breaks retain their flash regions and data;
their survival does not imply identical direction interpretation.

## Local build

Command: `./build.sh --release 1.0.1`.

| Resource | Used | Free | Capacity |
| --- | ---: | ---: | ---: |
| App flash | 580,284 B | 1,280 B | 581,564 B |
| Static RAM (`.data + .bss`) | 97,336 B | 968 B | 98,304 B |
| Pool | 334,348 B | 9,716 B | 344,064 B |

Package: `build/x0x-1.0.1.fwsc`, 609,680 B, identity `FM-1_987010001`.
SHA-256: `42190c2dffbda6d89807e40b68dd8d2426ef502fa3848dc47bb1af37e34a2fd3`.
On 18 October 2026, the existing package was checked without rebuilding:
its checksum matches the build record committed in
`45310f6c5a20eae1951c034783e75dca7a1fd55d`, its decrypted packaged app
matches `build/x0x.bin`, and its package/app identity is `FM-1_987010001`.
Firmware and build inputs have no source differences from that commit.
See the [hardware record and artifact verification](FM1_TEST_1.0.1.md).

Relative to the identity-only candidate, these two improvements add 288 B of
app flash and 4 B of static RAM after linker alignment; pool use is unchanged.
The loader image remains 7,764 B and wrapped `ota.bin` 6,524 B, byte-identical
to the saved baseline. Firmware capacity, single-precision, RAM-call and
register-access checks passed. No functionality was removed to make it fit.

## Automated validation

Targeted sequencer tests cover both 303s at lengths **1, 8, 16, 17, 32, 64**:
exact MREV order and lookahead, full wraps and mutation boundaries, stop/start,
Continue, cue/Song pattern restart, internal swing at 1/16 and 1/32, external
clock traversal, TIE/slide and a TIE across the cycle boundary. Conventional
REV/PING and seeded RND traversal are checked. Existing Remix tests also run
across all five directions and exercise motion/source bypass.

UI/storage tests cover five knob and SEL direction choices in exact order,
legacy raw values without migration, storage sizes, save/load, copy, Undo and
redo. MORPH tests cover both voices and input paths, exact text and duration,
framebuffer visibility while the knob readout/list is drawn, unchanged accepted
waveforms, no automatic replay, silent automation/loading, and a queued release
followed by a successful user acquisition. Existing exclusion/DSP tests remain.

Identity inspection compares the built app to the encrypted packaged app and
checks the exact USB identity. The OTA handshake and original-v1.0.0-to-candidate
loader simulations pass, preserving all seeded project, sample and settings
bytes. Hub decoder/parser/recognition checks pass against the saved live
2026-10-17 snapshot; no Hub changes or physical installation are implied.

The complete suite ran with `X0X_VERSION=1.0.1`, the candidate package and the SDK:
74 groups passed directly. The scheme snapshot group initially failed because
the DIR control now draws five position markers. Its updated test verifies
those five markers and normalizes only their 38×9-pixel region for the
historical hash comparison; it then passed on a focused rerun. Historical
screen hashes were not regenerated. Thus all **75 suite groups pass across the
run and corrected rerun**, with no unresolved failures. Three reference
comparisons were skipped because `schwung-9W9`, `schwung-8W8` and `schwung-303`
source checkouts are unavailable. Existing signedness compiler warnings remain.

The rebuilt v1.0.1 emulator passed audio parity, panel/settings/persistence,
303 parameters, slots, schemes, Remix parity and drum-generator checks.
The final UI rerun additionally checks actual 1:2 probability lanes against
complete MREV passes on both voices at all six lengths, and rejection when
the peer's effective Morph comes from motion rather than its saved base.

The package-free local documentation preview passes all six-page link, anchor,
asset, provenance and installation-disabled checks. Preview generation now
leaves a historical firmware URL public if that package is absent locally;
it does not create or change any public download. Manual HTML was regenerated
and compared byte for byte with a fresh conversion. Logs are under
`build/v1.0.1/` and `build/host/`; conflict screenshots are in
`build/host/waveform-ui/conflict-{a,b}-{knob,sel}.png`.

## Changed files (including preserved identity work)

- Firmware: `firmware/src/app/engine.c`, `engine.h`, `ui.c`;
  `firmware/src/seq/pattern.h`, `sequencer.c`; `firmware/src/x0x.c`.
- Build/site tools: `tools/build.py`, `tools/make_pages.py`.
- Tests: `tests/host/bass303_ui_test.c`, `scheme_ui_test.c`, `seq_test.c`;
  `tests/identity_test.py`, `hub_identity_test.mjs`, `install_test.py`,
  `ldr_test.c`, `ota_test.c`, `pages_test.py`, `run_tests.sh`; `web/test_web.mjs`.
- Documentation: `BUILDING.md`; `docs/FIRMWARE_IDENTITY.md`, `MANUAL.md`,
  `manual.html`, `NEW.md`, `REMIX_DESIGN.md`, `TB303_WAVEFORMS_DESIGN.md`,
  `RELEASE_1.0.1.md`.

## Reported hardware validation — 18 October 2026

The maintainer successfully installed and tested this v1.0.1 candidate on a
real M-VAVE FM-1. **The first eight items of the nine-item hardware checklist
passed:** independent identity `FM-1_987010001`, five directions including
MREV, correct Musical Reverse traversal, Remix bypass/restoration, `MORPH IN USE`
through the knob, the same message through SEL, save/restart persistence,
and existing project data/user samples. The device also ran stably without
audible dropouts or unexpected interruptions.

**Item 9 was not performed:** external MIDI equipment was not connected, so
external MIDI clock and the dedicated extended performance test are untested.
**Interrupted-update recovery on physical hardware was not tested.** The stable
run above is not a pass for the dedicated extended test. No duration or more
granular hardware measurements were reported. The complete result table and
verification limits are in [FM1_TEST_1.0.1.md](FM1_TEST_1.0.1.md).

Remaining preparation: decide how to address these unperformed checks, recheck
the live Hub registry before publication, and obtain explicit approval before
publishing v1.0.1 Stable. A physical Firmware Hub installation has not been
reported; automated Hub compatibility checks do not establish that result.
Retain the successfully tested binary and its checksum; a necessary firmware
rebuild would require new hardware validation.

## Release protection

The official v1.0.0 release remains unchanged, including identity
`FM-1_9010000` and SHA-256
`eccacfa995efec6b9c819583e7bce43b3aa7a126033c8e668ae89c6a988b754c`.
Its package was not rebuilt or replaced, its historical release records were
not rewritten, and `main` was not modified. A documentation-only commit and a
normal push of `dev/independent-firmware-identity` to origin are approved.
No v1.0.1 tag, GitHub release, Pages deployment or public download/installer
change is authorized. Wait for explicit approval before publishing v1.0.1 Stable.
