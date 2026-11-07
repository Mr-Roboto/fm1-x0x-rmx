# RMX release plus upstream performance and fixes

Branch: `integrate/upstream-fixes-rmx-1.1.0`.

This is a development integration, not the hardware-validated v1.1.0 release.
No installable firmware, release, website deployment or remote branch was published.

## Sources and scope

- Base: Mr-Roboto/fm1-x0x-rmx **v1.1.0**, commit
  `a4e78f3a6e16fa310b7614e5876d07e16a0020f9`.
- Reviewed upstream: charlesvestal/fm1-x0x **main**, commit
  `ab4aa06c82450900e0fb6c2c8c3b279af9bf9fae`.
- Requested scope: keep the RMX release's features and integrate upstream performance
  improvements and applicable bug fixes. Commits were selected and adapted rather
  than merging all of upstream/main and importing its new product features.
- The default RMX `orb-remix` branch is older firmware; it was not used as the base.

Remix, MREV, Grids, 22 TB-3PO scales, Morph ownership and controls, color schemes,
startup patterns, BREAK FILTER, ENV reset, mu-law cymbals and RMX USB backups remain.
The settings structure, direction numbering, project format/revision, backup profile,
backup validation and RMX product identity remain those of v1.1.0.

## Integrated changes

Original upstream commit IDs are retained in cherry-pick messages, except the
stock supply-level fix, whose source is recorded in its adaptation commit.

| Source | Change |
|---|---|
| `5048230` | Recover a valid previous pattern/song/motion stream after an interrupted save. |
| `ebfef2a` | Migrate old bass-drum pan motion lanes after loading the pattern stream. |
| `b07a759` | Prevent swing changes, reverse/random playback and Continue from drifting off the timing grid. |
| `ca7ca5c` | Clamp named parameter readouts instead of indexing beyond a label array. |
| `7ccdc06`, `9384a95` | Expose Linux host APIs and sequence stateful 4x decimation deterministically. |
| `0f2ded3`, `e78411a` | Optimize the 303's original waveforms and drives; share fixed coefficients; retain compiler portability. |
| `e7bb193` | Skip inaudible 909 sample tails and avoid interpolation at integral sample positions without drive. |
| `ae1a22b`, `8dbbc90` | Optimize the 808 metal bank with bounded 16-sample scratch blocks. |
| `ed35a45` | Keep core 1 disabled until power-off after a runtime failure or a crash alongside it. |
| `98dea5e` | Allow the recovery key combination while in safe mode. |
| `a580a28` | Keep the last crash's diagnostic numbers visible after a warm restart. |
| `c22d903` | Handle FX parameters in their correct engine voice and storage slot, including TAPE. |
| `a02727b` | Accept the known UC25HQ80 flash part while rejecting unknown parts. |
| `502b2c2` | Repair M-UPGRADE's short identity handshake and request full identities in the installer/backup clients. |
| `f295356` | Raise supply rails at boot to upstream's stock-firmware levels, before flash and dual-core work. |
| `7c226f7`, `403a4ed` | Add unchanged audio goldens and retain optimization attribution. |

Mu-law compression, BREAK FILTER, ENV reset, restore-cache invalidation and empty-object
handling already exist in the RMX release and were not replaced with upstream copies.

## Collisions resolved

**Saved settings and backups.** A full upstream merge would reuse RMX's Remix APPLY
and startup-pattern bytes for upstream's core/MIDI settings. Those additions were
excluded. RMX's `0x524D` backup profile and validation remain; standard upstream
backups are still rejected. The new handshake retains `FM-1_987...` identification.

**Direction numbering and swing.** Upstream masks directions with `& 3`; RMX's
RANDOM is 4 and MREV is 2. The mask was removed and the swing test's expected
MREV timing was adapted. Internal/external clock and Continue tests exercise all
five directions, alongside the existing Remix and MREV tests.

**Morph and 303 optimization.** Morph keeps the release's scalar voice render and
control-chunk slew timing. Saw/Square use upstream's optimized passes. Both paths
use the shared fixed coefficients and optimized drive. The scalar decimator remains
available at 2x, and the separately compiled test oracle initializes its own
coefficient table. The old baseline engine is given independent storage because
its private instance layout is larger than the optimized layout.

**Audio comparisons.** The upstream 303 hash sweep retains its original 12-pot RNG
sequence; the extra RMX Morph pot is tested separately. The original goldens were
not regenerated. The mu-law regression still compares every output sample exactly;
it normalizes only an exhausted sample's inaudible envelope counter, which the
optimization intentionally stops advancing. Position, fraction, mute count and
audible envelope state remain compared.

## Excluded upstream features

ROOM reverb replacement, PING WIDTH, tap tempo, MIDI CC, screen-reader integration,
the persistent GLOBAL second-core option, new release CI and stock-V16 installer
expansion are not part of this branch. Existing RMX reverb/delay controls and
persistent settings retain their release semantics. Runtime second-core failure
fallback is included without taking RMX's reserved settings bytes.

## Validation and remaining limits

Host validation uses Windows x64, Zig 0.13's Clang, `-ffp-contract=off` and assertions
enabled. Temporary adapters under ignored `build/tools/` supply the POSIX clock,
Python command naming and Windows shared-library arguments expected by the Unix
test runners. Barlow fonts use Pillow 12 and the installed FriBiDi library. None of
these adapters or generated files are firmware source changes.

On 2026-11-07, `tests/run_tests.sh` completed with **81 checks passed, including all
40 simulator scenarios, zero failures**. This includes the original-font UI checks,
Remix/MREV, Grids, all 22 TB-3PO scales, storage/migration, backup/restore, installer
handshakes, browser upload tools and the simulator build. The separate 27 engine
hash scenarios match the unchanged upstream goldens. Morph waveform tests pass at
1x/2x/4x oversampling and control factors 1 and 4. The extended cymbal regression
passes all 264 renderer cases. Assertions were enabled throughout the final run.

Three external reference engines and package-dependent update checks were skipped
for missing prerequisites, as listed below. Emscripten checks were not run because
Emscripten is absent. Detailed logs are under `build/host/`, including
`integration-final.log`, `dsp-hash-selected.log` and `waveform-selected.log`.

Remaining release gates:

- Build with the JieLi toolchain and AC79 SDK; inspect image, `.data + .bss`, pool
  and stack limits. They are unavailable in the current Windows environment;
  Docker's Linux engine is not running.
- Rebuild and run the browser emulator with Emscripten, which is unavailable here.
- Run external 9W9, 8W8 and Open303 reference comparisons when their checkouts are
  available. The local unchanged engine goldens provide an additional comparison.
- Validate on a physical FM-1: high-load audio, both cores, Morph, long 909 tails,
  stock-voltage initialization, safe mode, backup/restore and interrupted-save
  recovery. Host tests do not prove target CPU deadlines or electrical behaviour.
- Measure short audio fragments: upstream notes the 303 pass setup costs more for
  one-sample blocks, even though normal blocks are faster. Morph retains its original
  voice path, so Saw/Square speed gains must not be advertised as Morph benchmarks.

Do not install an arbitrary development build as the tested v1.1.0 release.
