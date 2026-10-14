# Orb-style Remix Design Specification

This document is the authoritative musical and sequencing specification for the
Remix feature in `fm1-x0x-rmx`, implemented on `orb-remix`. Persistent state, Forward
source mapping, the SEQ REMIX page, APPLY boundaries and mapped Motion playback/recording
are integrated. The [manual](MANUAL.md#remix) explains operation without storage internals.
Orb musical reverse and musical ping-pong are not implemented; unresolved requirements
for those possible extensions must not be inferred as current behavior.

## Scope

- Remix applies independently to `909`, `808`, `303A`, and `303B`.
- Each of these four tracks has its own Remix selection.
- `BREAK` is completely unaffected.

## Selection Model and Lookup Data

The user-facing selection consists of Bank **A–P** and Pattern **1–16**. Internally,
both dimensions are zero-based **0–15**. Together they select one of **256** Remix
patterns (`16 × 16`). Each Remix pattern contains **16 source-step selections**.

**A1** is the neutral sequential mapping: source steps `1..16` in user-facing
notation, or `0..15` internally. A1 remains a valid neutral mapping with Remix either ON or OFF; new patterns read A1/OFF.

The exact **4096-value** table (`256 × 16`) must come from the project's existing
historical Kontakt/Luminary implementation. The exact import is retained in [assets/remix/remixes.txt](../assets/remix/remixes.txt), with
pinned digest and nibble-packing documented in [its README](../assets/remix/README.md).
The project owner **Mr-Roboto** compiled the historical table by recording all 256
patterns from a physical Future Retro Orb into Cubase, analyzing the MIDI sequences
and manually constructing the mappings. That transcription/compilation is distinct
from Future Retro's original patterns and hardware; it does not claim ownership of
underlying intellectual property or resolve all licensing questions. The existing
source notice remains unchanged. Do not invent or substitute table contents.

## Core Playback Model

Remix changes which stored source step supplies the data for each chronological
playback slot. Chronological playback position and source-step position are
separate concepts and must remain separate in the implementation.

- Step 1 is always first, matching Orb behavior.
- Repeated source steps are valid.
- Remix may select any source step within the relevant 16-step block.

The lookup mapping determines source data; timing follows chronological playback
position. The implemented sequencer path is described under Sequencer integration below.

## Timing and Pattern Length

Swing follows **chronological playback-slot parity**, not mapped source-step
parity. Pattern length controls the number of chronological playback slots; it
does not reduce the source choices available to the Remix mapping.

For example, a length-8 pattern plays eight chronological slots. Those eight slots
may select any stored source steps `1–16`, including source steps beyond the
pattern's chronological length.

## 16-Step Block Rule

For normal forward Remix playback, repeat the same 16-slot Remix mapping
independently in every 16-step block:

- Chronological slots `0–15` map to source steps `0–15` using the selected Remix
  mapping.
- Chronological slots `16–31` use the same mapping plus 16.
- Chronological slots `32–47` use the same mapping plus 32.
- Chronological slots `48–63` use the same mapping plus 48.

Pattern length determines only the number of chronological playback slots.
Partial final blocks use only the required number of chronological mapping slots.

Examples:

- Length 24: block 0 has all 16 slots; block 1 has its first eight chronological
  slots.
- Length 40: blocks 0 and 1 have all 16 slots; block 2 has its first eight
  chronological slots.
- Length 64: four complete 16-step Remix blocks.

This rule does not resolve musical-reverse or ping-pong behavior for partial or
multi-block patterns; those cases remain unresolved.

## 303 Note, Tie, and Slide Behavior

TIE/slide lookahead must follow the **next chronological Remix slot**, rather than
simply using `source_step + 1`.

Existing Orb/Luminary behavior shows that note data belongs to the selected source
step. Sustained portions may behave differently when selected without their
original note-on. This observation does not specify additional X0X behavior.

The implemented Forward path maps both the current event and the next chronological
slot for lookahead, while previous-slide state belongs to the previously played event.
It retains existing X0X note/TIE semantics rather than adding a new sustain model.

## Drum Behavior

For `808` and `909`, a Remix-selected source step moves the **complete drum column**
for that step. This includes all voice hit bits and that step's accent state.

Drum timing and swing remain based on chronological playback position.

## Direction and Reverse

Existing X0X directions must not be silently removed.

Orb backward behavior is musical reverse: step 1 remains first, followed by
`16, 15, ..., 2` for a 16-step pattern. Orb documentation says backward playback
overrides Remix. The selected Remix remains stored and resumes when playback
returns to forward.

Existing X0X reverse behavior differs from this Orb behavior and must be preserved
unless an explicit design decision says otherwise.

The following are **unresolved** and must not be invented:

- Naming and UI treatment for conventional X0X reverse versus Orb-style musical
  reverse.
- Exact musical ping-pong behavior.
- Exact musical-reverse behavior for partial lengths and multi-block patterns.

The implemented integration preserves conventional X0X bass Reverse, Ping-Pong and
Random by bypassing Remix in those directions. The unresolved musical-reverse variants
above are possible extensions, not a missing decision in the current Forward feature.

## Persistence and Storage

The implemented layout uses existing reserved bytes for Remix selection and state,
without enlarging persistent pattern/project structures. Preserve the layout and legacy
handling below in future changes.

Large Remix lookup data must live in `const`/flash, not writable static RAM.
Keep the source representation readable during development. Build-time or
generated nibble packing into 4-bit entries is preferred over manually maintained
packed hexadecimal data. A packed 4096-entry 4-bit table occupies **2048 bytes**.

Remix state is persistent per pattern, with no structure-size or format change:

| Part | Selection byte | State byte | ON bit |
| --- | --- | --- | --- |
| 909 | `drum[0].rsv[0]` | `drum[0].rsv[1]` | 0 |
| 808 | `drum[1].rsv[0]` | `drum[1].rsv[1]` | 0 |
| 303A | `pattern.rsv[0]` | `pattern.rsv[2]` | 0 |
| 303B | `pattern.rsv[1]` | `pattern.rsv[2]` | 1 |

Selection is `(bank << 4) | pattern`, both zero-based. A drum marker is valid
only when `(state & 0xFE) == 0xA4` (OFF `0xA4`, ON `0xA5`). The shared bass
marker is valid only when `(state & 0xFC) == 0xA4` (`0xA4`..`0xA7`). All
remaining bits must match; zero, erased `0xFF`, and other invalid state bytes
read as A1/OFF regardless of selection bytes. This marker is an explicit
signature, not a checksum: historical bytes that exactly match it are
indistinguishable from deliberately stored state.

New patterns remain zero-initialized (no marker), reading as A1/OFF. Format-1
migration ignores historical reserved bytes and retains these defaults. Existing
format-2 streams need no conversion: getters safely decode missing/invalid
markers, and valid bytes round-trip through the existing stream. First setting a
bass state with an invalid shared marker initializes BOTH bass selections to
A1/OFF before applying the change. Helpers in `seq/pattern.h` own the encoding;
selection and enable setters preserve each other and the other part.

Part copy includes selection and ON/OFF; bass copy explicitly transfers its
pattern-level state without replacing the sibling's state. Part clear preserves
Remix, just as it preserves length/rate/direction. BREAK uses no Remix bytes.

## User Interface

The UI must expose Bank **A–P** and Pattern **1–16** separately for each applicable
track.

The dedicated V_SEQ REMIX page has four knobs, left to right: BANK (A–P),
PATTERN (1–16), REMIX (OFF/ON), APPLY (STEP/BEAT/BAR/LOOP). It follows
SEQUENCE on drums and KEYS on the 303s. BREAK has no Remix controls. A1
remains the neutral mapping. These references cannot record motion or p-locks.

Bank, pattern and enable edits use the existing persistent setters immediately,
so the UI, save and undo operate on the latest requested state. Playback holds
an independent audible snapshot for each of the four parts. The latest stored
state replaces that part's pending snapshot; equality with the active snapshot
means there is no pending change. At the chosen boundary selection and enable
activate together. Undo/copy edits enter the same path. A playing pattern switch
adopts the new pattern's state and discards the previous pattern's pending state.
Stopped changes apply immediately; stopping also resolves outstanding changes.

STEP activates on the next chronological step, BEAT (default) when the incoming
chronological position is divisible by four, BAR when divisible by sixteen,
and LOOP on the track's actual pattern wrap. Source mapping never determines
boundaries. The graphical step playhead shows the active mapped source for
Forward Remix playback, while page visibility follows the chronological 16-step
block. It is hidden on other visible pages and never switches the visible page.
OFF, BREAK and other directions retain their chronological playhead. Changing APPLY affects outstanding changes too;
it does not reset or enqueue selections.

APPLY occupies only `settings_t.rsv[0]`: `(byte & 0xFC) == 0xB4` is valid,
with bits 0–1 encoding STEP=0, BEAT=1, BAR=2, LOOP=3. Invalid bytes (including
zero and erased 0xFF) and absent bytes in shorter settings objects read as BEAT.
An exact historical signature match is indistinguishable from a valid setting.
Settings size (28 bytes), project size, formats and all pattern bytes are unchanged.
The existing settings object saves this global preference, outside pattern undo;
it is shared across parts and patterns. Defaults explicitly clear its byte.

## Implementation Discipline and Validation

### Sequencer integration (persistent state)

The sequencer now maps source reads for Forward playback only. `seq_track_t.pos`,
step callbacks, length/wrap/bar handling, transport, cueing, mutation cadence and
swing remain chronological. Drums read hit bits and accent from one mapped
column. Bass reads the mapped current event and maps the next chronological slot
separately for TIE lookahead; previous-slide state still belongs to the previously
played event. Reverse, Ping-Pong and Random bypass Remix for bass; drums have no
direction setting. BREAK is unchanged.

`seq_set_remix()` sets persistent state on the track's currently playing pattern,
using zero-based bank/pattern and an enable flag. Call it while stopped or between
audio blocks. Playback reads each part's current `ppat` independently. `seq_init()`
does not reset stored Remix state. Transport preserves it; pattern changes use
the new pattern's stored state. The Remix UI is described above; Orb musical
reverse is not implemented.

Motion, parameter locks and Probability follow the same mapped source step as
notes and drum columns during Forward Remix playback. Motion interpolates from
the current mapped source value toward the next chronological slot's mapped
source value, including repeated and nonadjacent sources and the established
16-step block mapping through 64 steps. Both endpoints use the active audible
Remix snapshot; a pending APPLY edit cannot affect a ramp before activation.
When a new snapshot activates, that incoming slot starts its ramp using the new
snapshot. A preceding ramp does not anticipate the pending selection.

Chronological slots still determine ramp duration, Swing parity, recording
positions, recording-gap interpolation and hold/gesture counters. The engine
step callback passes separate timing and source coordinates to Motion; the
sequencer's source accessor owns audible-state mapping and direction bypass.
Remix OFF, Reverse, Ping-Pong, Random and BREAK retain their existing behavior.
MOT_NONE/base fallback and interpolation rules are unchanged, including existing
interpolation of Probability lanes between set values.

Conditional probability cycles still use chronological pattern wraps. Drum
chance is checked per triggered voice; repeated source hits get the existing
chance checks on each visit. Bass chance remains checked by the existing note-on
sink: a TIE that continues a sounding note does not add a new chance check,
while a TIE after release still triggers the existing note-on path. The
sequencer's sounding/slide state and MIDI behavior when the bass sink rejects
a note are unchanged.

### Motion recording with active Remix

Forward Remix Motion recording writes endpoints to the actually audible mapped
source step. Recording gaps still use chronological positions and the existing
pass/wrap calculation and straight-line interpolation. Each intermediate value
is written in chronological order to its source step resolved through the active
audible mapping at the time of the write. The previous recorded endpoint supplies
the starting value from its original source destination, even if APPLY has since
switched the mapping. Repeated destinations may overwrite earlier values; later
writes win, including the final endpoint. Pending APPLY edits do not affect any
write until their audible snapshot activates.

For example, recording at chronological positions 3 and 7 fills the three values
for positions 4, 5 and 6 as before, but writes all five values to their mapped
source steps. No separate Remix Motion storage exists: **recording with Remix ON
changes the original source-step Motion data, so playback with Remix OFF may
sound different afterward.**

Remix OFF, Reverse, Ping-Pong, Random and BREAK retain their existing recording
behavior. Timing, playback interpolation, Swing, gesture and manual hold behavior
are unchanged. The knob wins during the existing recording gesture and manual
hold; afterward playback reads the active source lane again.

Before implementation, inspect the current sequencer, pattern persistence, UI,
motion, and tests. Verify the relevant execution paths and structures before
changing them. Do not infer behavior where this document explicitly marks it
unresolved.

Keep implementation modular and rebase-friendly. Prefer dedicated `seq/remix.c`
and `seq/remix.h` files for lookup and mapping logic if they fit the current
architecture.

Add focused tests for:

- Mapping and the neutral A1 selection.
- Chronological playback-slot/source-step separation.
- Swing based on chronological parity.
- 303 chronological TIE/slide lookahead.
- Complete drum columns, including voice hit bits and accent.
- The 16-step block mapping through length 64, including partial final blocks.
- Reverse interaction, once the relevant unresolved behavior is decided.

Import and validate the historical table separately; generated or packed data must
preserve its exact values.

## Implemented checks and unimplemented extensions

The implementation uses `seq/remix.c` / `seq/remix.h`, the imported 4096-value table,
2048-byte flash packing, the reserved-byte state layout above, chronological timing,
complete drum columns, mapped bass lookahead, and mapped Motion/Probability. Regression
coverage is in `tests/host/remix_test.c`, `remix_state_test.c`, `remix_ui_test.c`,
`remix_motion_test.c` and `motion_test.c`, with corresponding runner scripts.
These completed integration items are not pending design decisions.

No Orb musical reverse or musical ping-pong extension is part of the current feature.
Before adding one, decide its name/UI and behavior for partial lengths and multiple
16-step blocks explicitly. The existing bass Reverse, Ping-Pong and Random directions
currently bypass Remix and must remain available. Physical FM-1 validation remains pending.
