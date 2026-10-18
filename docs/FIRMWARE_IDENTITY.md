# X0X-RMX firmware identity

X0X-RMX uses the permanent numeric namespace **`FM-1_987`** for new builds.
Release identities are `FM-1_987XXYYZZ`, where `XX`, `YY` and `ZZ` are the
major, minor and patch version as two decimal digits each (0–99).
For example, v1.0.1 is **`FM-1_987010001`**. An omitted patch is zero.
An explicit prerelease suffix changes the displayed version and filename, but
does not change the numeric identity, as in the original build scheme.
Development builds use `FM-1_987` without version digits; Pages rejects this
development identity for a package-enabled installer.

## v1.1.0 Stable registry check

The live [Firmware Hub catalogue](https://thefirmwarehub.com/catalog.json)
was checked again on 18 October 2026 (snapshot `2026-10-18 11:00 UTC`,
57 packages). Our own `x0xrmx` entry already publishes v1.0.1 with identity
`FM-1_987010001` and recognizes the `FM-1_987` release family. No unrelated
package or identity rule claims `FM-1_987010100`, and no unrelated package
uses our chosen RMX family. This is a current catalog observation, not a
globally exclusive registration. The v1.1.0 package and embedded USB identity
were verified as `FM-1_987010100`. Indexing of the new release may be asynchronous.
The audit accepts our existing published entry and rejects unrelated claimants.

## Registry check

Checked the live [Firmware Hub identity guidance](https://thefirmwarehub.com/kb/#identity)
and its [identity catalogue](https://thefirmwarehub.com/catalog.json) on
2026-10-17. The fetched catalogue reports `updated: 2026-10-17 21:34 UTC`.
None of its 53 FM-1 package identities starts with `FM-1_987`.
The X0X-RMX entry already includes `FM-1_987\d{6}` in its family recognition
rule. The fetched Hub JavaScript recognizes `FM-1_987010001` as X0X-RMX.
This verifies the live Hub registry, not an exclusive reservation across
unlisted firmware projects. Recheck the live registry before public release.

The Hub requires a numeric identity after `FM-1_`, matching the running
firmware's USB answer and the package markers. Our release identity occupies
14 of the package format's 20 marker characters. `tools/build.py` passes the
same `PRODUCT` to the firmware compiler as `FELUCCA_ID` and to the package
writer. The package inspector verifies that the packaged app equals the built
app and contains this exact null-terminated USB identity.

The RAM update loader retains **`ota-FM-1_900`**. It is a separate recovery
protocol identity, not the installed firmware identity. This keeps the loader
binary unchanged and matches the live Hub's `^ota-FM-1_9\d\d$` interrupted-update
recognition rule. Both our installer and the Hub parse the new running identity
and compare its complete string after restart. Once a future approved release
is indexed, the Hub can match its package identity exactly. No catalogue,
GitHub release or public site is changed by this development work.

## Immutable v1.0.0 Stable

The existing hardware-tested release remains:

| Field | Published value |
| --- | --- |
| Package | `x0x-1.0.0.fwsc` |
| Identity | `FM-1_9010000` |
| SHA-256 | `eccacfa995efec6b9c819583e7bce43b3aa7a126033c8e668ae89c6a988b754c` |

Do not rebuild, replace, retag or modify that release. The build rejects the
reserved unsuffixed version 1.0.0 (including `1.0` and zero-padded equivalents)
before generating artifacts. Historical release notes, manual statements,
hardware reports and stable-release tests continue to describe that original
package. Pages retains support for its historical identity. New identity
tests supplement those historical checks rather than relabeling them.

## Update persistence

The identity change does not change persistent structs, storage formats,
flash addresses, chip key, app slot, update protocol or loader implementation.
`firmware/loader/ldr_core.c` writes the app in `[0x4000, 0x93000)` and clears
valid update records. X0X project objects are below the user slots at
`0x97000..0x9FFFF`; the three uploaded break slots occupy
`0xA0000..0xDBFFF`; calibration and motion occupy `0xDC000..0xDFFFF`;
settings/globals are in `0xFC000..0xFEFFF`. These addresses remain unchanged.

The loader regression starts with the original v1.0.0 package installed,
seeds all these data regions, installs the new package and checks every seeded
byte. It also checks unchanged flash headers, update-record cleanup, a repeat
install with no app erases, and refusal of a damaged/foreign-key app. Existing
storage and project tests cover reading and writing project formats. These
checks support preservation on update; physical-device confirmation remains
required.

## Local validation commands

Use a separate development branch; do not touch `main`. No command here
publishes or talks to a device:

```sh
./build.sh --release 1.0.1
python3 tests/identity_test.py build/x0x-1.0.1.fwsc build/x0x.bin
python3 tests/install_test.py
X0X_TEST_PACKAGE=build/x0x-1.0.1.fwsc node web/test_web.mjs
cc -O2 -o build/identity/ldr_test tests/ldr_test.c
build/identity/ldr_test build/x0x-1.0.0.fwsc build/x0x-1.0.1.fwsc
cc -O2 '-DFELUCCA_ID="FM-1_987010001"' -o build/identity/ota_test tests/ota_test.c
build/identity/ota_test build/x0x-1.0.1.fwsc
X0X_VERSION=1.0.1 sh web/emu/build.sh
X0X_TEST_PACKAGE=build/x0x-1.0.1.fwsc tests/run_tests.sh
```

Create `build/identity/` first. The update-loader portion of the full suite
also needs `AC79_SDK` exported, as described in BUILDING.md. Its default OTA
test models the legacy running identity; the explicit command above additionally
exercises the new running identity.

To repeat the offline Hub compatibility audit, fetch the Hub home page and
catalogue into local files, then run:

```sh
node tests/hub_identity_test.mjs HUB.html catalog.json build/x0x-1.0.1.fwsc
```

This checks the fetched Hub's own decoder, identity parser, recovery-loader
rule and family matching, plus a simulated future catalogue refresh for exact
matching. It never writes to the Hub. The test deliberately fails if the Hub
snapshot changes these interfaces or already contains a new-namespace package;
inspect and update the audit for that later registry state.

## Development build results (2026-10-17)

The following records the earlier identity-only candidate. The local v1.0.1
artifact is now superseded by the MREV/MORPH candidate described in
[v1.0.1 development release notes](RELEASE_1.0.1.md), with its current checksum,
capacity results and intentional direction compatibility change.

Branch: `dev/independent-firmware-identity`. Local candidate:
`build/x0x-1.0.1.fwsc`, identity `FM-1_987010001`, SHA-256
`bb3b79d3bfb005231fe94f52b30153102dbcc91eb2e6ebf26774e437f684c4c3`.
This package is not approved for public release.

| Resource | Used | Capacity | Free |
| --- | ---: | ---: | ---: |
| App image | 579,996 B | 581,564 B | 1,568 B |
| Static RAM (`.data + .bss`) | 97,332 B | 98,304 B | 972 B |
| Pool | 334,348 B | 344,064 B | 9,716 B |

Package size: 609,680 B. Loader image: 7,764 B; wrapped `ota.bin`: 6,524 B,
byte-identical to the saved baseline loader. The app is 16 B larger than the
saved stable build; RAM and pool symbols are unchanged. Firmware safety checks
(RAM calls, single-precision arithmetic, capacity and register access) passed.
The emulator also rebuilt successfully, and its panel/settings/persistence
check passed.

Focused identity/package inspection, the original-v1.0.0-to-candidate loader
test, the new-identity OTA handshake test, CLI installer simulation and the
offline live-Hub audit passed. The original v1.0.0 identity and package checksum
were rechecked unchanged.

The broad suite passed 72 checks directly, including storage, project/UI,
OTA/loader, emulator and simulator scenarios. Three Node subprocess checks
(`web-installer`, `builtin-breaks`, `breaks-web`) encountered sandbox `EPERM`
and passed when rerun outside the sandbox, bringing the total to 75 passing
checks. Three external DSP reference comparisons
were skipped: `schwung-9W9`, `schwung-8W8` and `schwung-303` are unavailable.
Logs and the fetched registry snapshot are in `build/identity/`; the broad
suite's component logs are in `build/host/`.

Changed sources: `tools/build.py`, `firmware/src/x0x.c`, `tools/make_pages.py`,
`tests/identity_test.py`, `tests/hub_identity_test.mjs`, `tests/install_test.py`,
`tests/ldr_test.c`, `tests/ota_test.c`, `tests/run_tests.sh`, `web/test_web.mjs`,
`BUILDING.md` and this document. Historical v1.0.0 documentation and tests are
unchanged. No commits, pushes, tags, releases, firmware downloads or Pages
deployments were made.

## Hardware and release gate

The maintainer's 18 October 2026 hardware report confirms checklist items 1–8,
including the independent identity, MREV, Remix restoration, both MORPH warning
paths, save/restart and retained project data/user samples. See the
[v1.0.1 hardware record](FM1_TEST_1.0.1.md) for the exact reported results and
the unchanged tested package checksum. External MIDI clock/dedicated extended
performance testing (item 9) and physical interrupted-update recovery remain
untested. The maintainer explicitly approved v1.0.1 Stable publication on
18 October 2026 with these limitations and no additional hardware-test requirement.
A publication-day recheck found no competing `FM-1_987` package identity among
57 live catalogue entries; X0X-RMX family recognition remains present.

The earlier validation plan below is retained for repeatability; it is not a
claim that every subcheck was performed. Before approval, on an FM-1 running
the original v1.0.0 package:

1. Save representative projects with Remix, patterns, song and settings; upload
   recognizable user breaks into all three slots and record their contents.
2. Install the candidate with the project's Web MIDI installer and verify the
   exact USB identity `FM-1_987010001` against the package. Power-cycle and
   verify project settings, pattern/song playback and all uploaded breaks.
3. Check reinstall and interrupted-update recovery through `ota-FM-1_900` on
   hardware. Repeat the agreed update-chain checks and musical/USB smoke test.
4. Validate the Hub install on hardware when an approved candidate can be made
   available through its agreed validation process. Current checks exercise its
   parser and matching code, not a physical Hub installation.

v1.0.1 Stable publication, its exact tested package, the public installer and
Pages update are explicitly approved. The original v1.0.0 release remains
unchanged. Future releases require their own validation and authorization.
