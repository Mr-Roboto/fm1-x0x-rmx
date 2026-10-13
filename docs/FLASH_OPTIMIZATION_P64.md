# P6.4 Flash optimization audit

The unchanged P6.3 working tree on `orb-remix` was built before optimization.
It reproduced the recorded 581,500-byte firmware image. Existing uncommitted
P6.2/P6.3 changes were retained. No branch change, commit or push was performed.

## Retained change

`firmware/src/app/ui.c`: replace the four by-value `pref_t` arguments of
`add_page()` with one four-reference array passed to `add_page_refs()`.
The existing call sites use a macro to construct that row; the helper copies
it into the existing page buffer. Page titles, groups, bounds, descriptors,
parameter values and control behavior are unchanged.

The target disassembly showed four separate four-byte argument copies at page
construction sites. The new helper performs one 16-byte copy. This removes
duplicated ABI preparation without changing persistent data or adding static
storage. Actual linked savings are **1,712 bytes**: `build_pages()` decreases
from 5,478 to 3,848 bytes, the page helper from 202 to 122 bytes, and image
alignment accounts for the remaining two bytes.

Only this UI source optimization is retained. Generator algorithms, source
data, compiler/linker flags, DSP, playback and publication code remain as P6.3.

## Measurements and rejected attempts

| Build | Firmware image | Static RAM | Pool |
| --- | ---: | ---: | ---: |
| Reproduced P6.3 | 581,500 | 97,324 | 334,348 |
| Generator-only `-Oz` experiment | 581,388 | 97,324 | 334,348 |
| `-Oz` plus UI row arguments | 579,676 | 97,324 | 334,348 |
| Final UI row arguments, original compiler flags | **579,788** | **97,324** | **334,348** |

A `minsize` attribute on `build_pages()` increased size and caused a 144-byte
XIP linker overflow; it was reverted. Generator-only `-Oz` saved 112 bytes,
but introduced six row-helper calls during protected publication. It was
reverted to retain the original publication instruction sequence. No global
compiler change or behavior compromise was needed.

| Resource | P6.3 | P6.4 | Delta | Capacity | Remaining |
| --- | ---: | ---: | ---: | ---: | ---: |
| Firmware Flash | 581,500 | 579,788 | -1,712 | 581,564 | **1,776** |
| Static RAM | 97,324 | 97,324 | 0 | 98,304 | **980** |
| Pool | 334,348 | 334,348 | 0 | 344,064 | **9,716** |

Both Flash targets (512 and 1,024 free bytes) are met. The pool has 1,524 bytes
above its mandatory 8,192-byte free reserve. Two final target builds produced
byte-identical images. The linked MAP table remains exactly 1,200 bytes and is
byte-identical to P6.3. Publication remains 86 bytes with the same executable
instructions after relocation; unreachable alignment padding was excluded
from the instruction comparison.

## Verification

- Core tests: all original node conversions, exhaustive MAP interpolation,
  reference trigger/accent arithmetic, Euclidean lengths/fills/offsets,
  longer patterns, NEW/MUTATE, RNG behavior, target-row and accent preservation.
- Project and UI tests: serialization/layout, scoped Undo/Redo, separate
  transient settings, normalization, page/button dispatch, no-op transactions,
  NEW FIRST, playback edits and guarded publication.
- The full runner produced 68 passing entries. Browser break parity passed
  separately outside the sandbox, giving 69 available checks passed. The
  runner exits nonzero because it treats missing external references and the
  sandbox subprocess error as failures; these are not claimed as passes.
- Three external DSP comparisons are unavailable: `schwung-9W9`,
  `schwung-8W8`, and `schwung-303`. They are skipped for this audit. Vendored
  Grids arithmetic/data comparisons ran successfully.
- Existing Remix, TB-3PO, motion, sequencer, storage, update-loader, available
  DSP/audio, host/browser parity and simulator checks pass. The browser emulator
  rebuild and its generator rendering/navigation tests pass.
- A separate differential executable compares the saved P6.3 core against the
  current core over 800,000 seeded NEW/MUTATE operations. Statuses, prepared
  edits, applied drum bytes and complete transient settings/RNG states match.
  Core source and header are also unchanged.
- All 190 unaffected legacy LCD golden hashes pass. Rebuilding the host UI test
  against the saved P6.3 UI reproduces the generator screenshot files exactly.
  Representative MAP/EUCLID screenshots were visually inspected.
- Replaying every simulator scenario with baseline and optimized host binaries
  compares 82 PNG/WAV artifacts. Seventy-seven are byte-identical, including
  all six WAVs. Five screenshots differ only in host wall-clock CPU readings
  (`perf/p01_live`, `perf/p02_running`, `perf/p03_results`,
  `tour/t14_mix_pump`, `showcase/s02_mix_comp`); all remaining pixels match.
- Core ASan/UBSan passes without exclusions. UI ASan/UBSan passes with the
  existing `-fno-sanitize=shift-base` exclusion for the integration binary,
  covering the known negative signed shift in `dsp/fastmath.h:57`.
  This is not a fully clean, unrestricted UBSan result. No additional sanitizer
  check was disabled, and DSP source was not modified. LeakSanitizer ran
  outside the sandbox.

## Reproduction and evidence

Target builds: `./build.sh`. Full regression command:

```sh
AC79_SDK=/home/falke/fw-AC79_AIoT_SDK \
X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L tests/run_tests.sh
```

Build logs are `build/p64-baseline.log`, `build/p64-core-oz.log`,
`build/p64-pages-minsize.log`, `build/p64-page-refs.log`,
`build/p64-final-firmware.log` and `build/p64-repeat-firmware.log`.
Regression logs are `build/p64-final-tests.log`, `build/p64-equivalence.log`,
`build/p64-scenario-comparison.log`, `build/p64-publication-comparison.log`,
`build/host/breaks-web-retry.log`, `build/host/p62-core-sanitizers.log` and
`build/host/p63-ui-sanitizers-compatible.log`.
Saved baseline sources, ELF/image, differential harness and UI reference host
are under `build/p64-baseline/`; baseline/final scenario replays are under
`build/p64-baseline/scenarios/` and `build/p64-final/scenarios/`.

Physical FM-1 timing and playback stability remain unmeasured. The preferred
Flash margin is reached; further optimization was intentionally stopped.
