# Offline drum generator (P6.2 core and P6.3 integration)

The generator writes ordinary drum masks. The core has no playback, interrupt,
automatic generation or persistence hooks. P6.3 connects explicit UI commands
and existing Undo storage to the core. Existing persistent layouts, Remix bytes
and TB-3PO behavior are unchanged.

## Session state and commands

`firmware/src/seq/drum_gen.h` defines a 16-byte `drum_gen_cfg_t` and two independent
`drum_gen_session` entries (909, 808). Their initialized RAM defaults are MAP,
X/Y 128/128, CHAOS 0%, FILL 50/35/65%, LEN 16/16/16 and OFFSET 0/4/2. Restart
restores these defaults. `drum_gen_init(seed)` can explicitly initialize both
machines with separate nonzero random states; `drum_gen_defaults()` initializes
an individual configuration. Never put this type in `proj` or an Undo region.

`drum_gen_normalize(cfg, pattern_length)` clamps FILL/CHAOS, mode and LEN, then
normalizes offsets. MAP bounds are always 0..15; EUCLID bounds are 0..LEN-1.
LEN is 1..min(32, pattern_length), and shortening it does not remember a hidden
previous length. The P6.3 caller must normalize on pattern/length/mode changes,
including those caused by Undo/Redo. Switching patterns otherwise retains each
machine's configuration. Do not reset session settings on project load.

`drum_gen_new()` and `drum_gen_mutate()` prepare a 48-byte `drum_gen_edit_t` from
the current drum part and configuration. Preparation changes neither argument.
The edit holds three hit rows, shared accents and the next session configuration.
Result statuses are `DG_CHANGED`, `DG_UNCHANGED`, `DG_NEW_FIRST` or `DG_INVALID`.
Kit must be 0/909 or 1/808 and pattern length must be 1..64. NEW normalizes its
prepared configuration; MUTATE rejects unnormalized configuration rather than
silently changing LEN/FILL/OFFSET. Inputs/pointers must be valid.

For a changed edit, the P6.3 caller should:

1. Capture the active drum part and prepare the edit on the main thread.
2. Close pending Undo edits in the selected drum part before publishing.
3. Exclude audio interrupts briefly and call `drum_gen_apply()`.
4. Close the generation's Undo transaction immediately afterward.

Do not calculate rhythms, scan Undo or change the destination between prepare
and apply. Apply only copies aligned mask words and session state; it does not
scan the pattern, allocate memory or calculate rhythms. Its mask destination
must be the same part used during preparation. The caller supplies the short
publication critical section in P6.3; no platform/IRQ changes are made here.

An unchanged NEW may still advance MAP RNG or normalize session configuration;
its prepared session update can be applied without an Undo entry. Unsuccessful
MUTATE changes no session state and must not create an Undo entry. No artificial
empty entries or additional Undo buffers are needed.

## Rhythms

MAP uses the original Grids 5x5 spatial ordering, two-stage `U8Mix` truncation,
salience threshold and >192 accent threshold. Percentages map to bytes with
`(percent * 255 + 50) / 100`. CHAOS draws one saturating, nonnegative perturbation
per instrument per bar, using a nonzero 16-bit Galois LFSR. Each 16-step bar is
independent; CHAOS 0 neither draws random values nor varies equivalent bars.
The offline draw schedule intentionally differs from original live clocking.

`tools/gen_grids.py` checks pinned source data, keeps positions 0,2,...,30 in
each channel, and emits exactly 1,200 bytes of immutable MAP nodes in spatial
order. Full source tables are never included in firmware. See
`assets/grids/README.md` for pinning, checksum and licensing. Builds are offline.

EUCLID uses `(LEN * FILL + 50) / 100` hits and the arithmetic test
`(source_position * hits) % LEN < hits`. It repeats each channel's cycle over
the stored pattern. Positive offsets shift later, using normalized circular
source positions. Nonempty canonical cycles start with a hit at position zero;
their accent rotates with that hit. FILL 0 has no hit or accent.

The arithmetic rhythms match the original 1,024 Euclidean table entries at
their actual hit counts: 646 directly and 378 after circular rotation. Equal
FILL percentages need not give original hit counts because original density is
quantized into 32 choices. In particular, length 32 / FILL 50% yields exactly
16 hits here, versus 17 with the original byte/density mapping. Original reset
outputs are not used as accents on silent cycles.

NEW replaces only Kick/Snare/Closed Hi-Hat, indices 0/1/7 (909) and 0/1/10 (808),
inside `0..len-1`. MAP rotates full bars before cropping a partial bar. All other
rows, hidden steps, length, rate, metadata and other parts remain unchanged.
Remix can still read preserved hidden steps, as before.

MAP MUTATE starts from stored hits and performs at most `ceil(CHAOS * 4 / 100)`
flips, capped at four, from at most 96 sampled candidates. It selects nonzero
map weights near the FILL threshold, protects weights >192, and never revisits
the same column during one command. CHAOS 0 changes nothing, including accents.
Random state supplied through configuration makes commands reproducible.

EUCLID MUTATE first verifies both Snare/Hi-Hat active rows against current
configuration, even if a channel has no effective rotations. Mismatch returns
NEW FIRST. It enumerates the four channel/direction combinations, excludes
rotations whose hit mask is unchanged, and randomly selects a remaining +/-2
rotation. No effective candidate returns unchanged; there is no +/-1 fallback.
Success updates exactly one offset and its rhythm. Kick hits/offset, LEN and
FILL are preserved. Hidden steps do not participate in the consistency check.

## Shared accents

NEW unions the old target-hit columns and applies this rule inside active length:

`accent = (old_accent & ~(old_target_hits & ~unrelated_hits)) | generated_accent`

Every old accent coincident with any of the eight unrelated instrument rows is
preserved. Unaffected/hidden accents remain unchanged. MUTATE scopes cleanup to
the affected row's old hits and relevant columns; remaining generated strong
accents in those columns are retained/reconstructed where applicable. Columns
outside the affected rhythm are not changed by mutation.

The shared row has no provenance. Old generator accents coincident with
unrelated hits, or orphaned by later manual hit removal, cannot reliably be
distinguished from manual accents and may persist. Manual accents on affected
target-only columns may be cleared. Variant C intentionally changes every
sounding instrument's velocity at a shared accented column, including Kick
when a Snare/Hi-Hat mutation changes that column's accent.

## Capacity gate

The device core is built with `-Os`. Public APIs were explicitly retained by
linker roots for the P6.2 measurements, so garbage collection could not hide
their cost before UI callers existed. P6.3 uses normal linking: real UI callers
retain the required core; unused initialization APIs are collected. Initialized
session RAM still restores defaults at firmware startup.

| Resource | Unchanged baseline | P6.2 | Delta | Free |
| --- | ---: | ---: | ---: | ---: |
| Firmware image | 576,348 | 580,188 | +3,840 | 1,376 |
| Static RAM | 97,244 | 97,308 | +64 | 996 |
| Pool | 334,348 | 334,348 | 0 | 9,716 |

The linked MAP symbol is 1,200 bytes in XIP `.text`/constant data; session state
is 32 bytes in `.data`. RAM's extra 32 bytes are linker padding before the
existing aligned `.bss`, not an additional buffer. Linked core functions total
2,584 bytes; the remaining image delta includes defaults, initialized session
data and link alignment. Only 1,376 image bytes remain for P6.3: measure each UI
increment and report a capacity conflict before sacrificing existing behavior.

## Validation

`sh tests/host/run_drum_gen.sh` verifies all original-node conversions, source
checksum, reproducible generation, all 3,145,728 MAP interpolation inputs, 540
MAP trigger/accent cases with seeded offline perturbations, all EUCLID lengths,
FILL values and offsets, original reference cycle shapes, both drum machines,
the requested pattern lengths, mutation eligibility/strength, NEW FIRST after
manual edits and simulated Undo, shared accents and persistent structure sizes.
The original full Grids live-clock fidelity suite is not substituted for these
adaptation-aware comparisons; this core deliberately has different clocking,
Euclidean density/phase and accent semantics.

`sh tests/host/run_drum_gen_project.sh` exercises actual project save/load and
existing Undo/Redo with generated steps, proving that session settings remain
outside both mechanisms and an identical NEW creates no Undo entry. This test
does not add production UI/Undo wiring.

The P6.2 focused suite and Address/UndefinedBehavior sanitizers pass, as do the
project integration test, existing Remix/TB-3PO/Undo/storage checks, browser
build/parity tests and available simulator scenarios. The broad runner reports
three unavailable external DSP reference checkouts (9W9, 8W8, schwung-303).
Its Linux host build needs the existing `X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L`
workaround; a separate build with that flag passes. The browser break conversion
test and LeakSanitizer pass outside the sandbox after sandbox restrictions
blocked their first attempts. The update-loader test also passes when supplied
the SDK path that `build.sh` already uses by default. Existing app signedness
warnings remain; the new core compiles cleanly with `-Wall -Wextra -Werror`.

## P6.3 UI and final capacity gate

ARP opens GRIDS for either drum machine, TB-3PO for either 303, and rejects BREAK.
Repeated ARP cycles exactly three drum pages; SELECT moves through the same
pages. ALGORITHM retains generator view across supported parts and exits it
for BREAK. PRESETS continues to cue only the displayed part's pattern.

| Page | Knob 1 | Knob 2 | Knob 3 | Knob 4 |
| --- | --- | --- | --- | --- |
| MAP | X | Y | CHAOS | MODE |
| EUCLID | LEN K | LEN S | LEN H | MODE |
| FILL | FILL K | FILL S | FILL H | Unused |
| OFFSET | OFF K | OFF S | OFF H | Unused |

MAP/EUCLID share the same first-page slot; FILL/OFFSET are common to both modes.
Transient parameter refs bypass dirty marking and cannot record motion. MODE
also refreshes the SEL list while keeping MODE selected. Values are normalized
when pages are built, when the drum length is edited, and before a command.
This covers active pattern cues, part/mode switches, project loads and Undo/Redo
length restoration without changing stored pattern lengths. Session values are
not reset by project load. The host/browser simulated boot explicitly restores
generator defaults; its pre-existing UI page cursor retention is unchanged.

OCT+ prepares NEW and OCT- prepares MUTATE. Effective operations display NEW
DRUMS or MUTATED. Ineffective commands display NO CHANGE; stale Euclidean rows
display NEW FIRST and remain untouched. Current offsets are read directly from
session state. Existing SAVE+OCT and other modifier precedence is preserved.

`undo_commit_range()` reuses the existing shadow/pool. A changed command first
commits pending edits in its captured drum part, publishes, then immediately
commits that part again. Quiet-period batching cannot merge rapid commands.
Other pending edits, including audio-side bass auto-mutation and motion, remain
pending outside this span instead of entering the generator transaction. Normal
Undo/Redo retains its existing all-project behavior; if other playback edits
have occurred, those can be separate history entries. No-change commands do not
flush or manufacture entries, and RNG/knobs stay outside Undo.

`plat_edit_lock()`/`plat_edit_unlock()` are non-nested, main-loop wrappers around
the established device IRQ masking facilities; host/browser execution is already
serial. The device disassembly verifies CLI, the prepared apply call, CSYNC/STI,
then the post-command Undo scan. Generation, allocation, rendering and history
scans are outside the critical section. Physical hardware timing was not measured.

| Resource | P6.2 | P6.3 | Delta | Capacity | Remaining |
| --- | ---: | ---: | ---: | ---: | ---: |
| Firmware image | 580,188 | 581,500 | +1,312 | 581,564 | **64** |
| Static RAM | 97,308 | 97,324 | +16 | 98,304 | **980** |
| Pool | 334,348 | 334,348 | 0 | 344,064 | **9,716** |

The MAP stays exactly 1,200 bytes in flash. The pool retains the required 8,192
free bytes, with 1,524 bytes above that reserve. The new mutable parameter
descriptor is 12 bytes; RAM alignment accounts for the other four bytes. Page
construction reuses the existing page buffer. No core algorithm, persistent
structure, font, sample, DSP feature or existing UI function was removed.

Incremental build logs are `build/p63-pages.log` (581,404 bytes),
`build/p63-buttons.log` (initial 224-byte XIP overflow),
`build/p63-buttons-compact.log` (581,532 bytes), `build/p63-undo.log`
(581,452 bytes), and `build/p63-final-candidate.log` (581,500 bytes after list
refresh). Compact construction of the new pages and removal of temporary P6.2
linker roots resolved the overflow without changing behavior or compiler flags.

Focused host UI tests cover real panel dispatch, separate settings, normalization,
no dirty/motion writes, rapid history boundaries, no-op history, NEW FIRST,
length restoration, pending playback edits and publication hooks. They also run
actual playback with bass auto-mutation and verify no automatic Grids changes.
Host screenshots are in `build/scenarios/drum_gen/`; the WebAssembly test compares
the rendered header/controls with those screenshots, exercises current OFFSET
values, NEW FIRST and simulated restart, and verifies session edits do not save.
The 190 unaffected legacy LCD hashes still match; the ten previously unreachable
drum TB-3PO fixtures now check the approved GRIDS layout instead.

Core, project, UI, Undo and browser generator tests pass. Available existing
Remix/TB-3PO, storage, update-loader, browser parity and scenario checks pass;
the three external DSP reference checkouts remain absent. The browser break test
passes outside the sandbox after its Python subprocess was blocked with EPERM.
The fully enabled UI sanitizer run finds the pre-existing signed shift at
`firmware/src/dsp/fastmath.h:57`; DSP code was left unchanged. The UI suite passes
Address/UndefinedBehavior sanitizers with that shift-base check excluded and
LeakSanitizer run outside the sandbox. Full generator-core sanitizer checks are
not relaxed. Existing app signedness compiler warnings remain unchanged.
