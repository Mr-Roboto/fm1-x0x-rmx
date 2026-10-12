# Orb-style Remix Design Specification

This document is the authoritative musical and sequencing specification for the
Remix feature in `fm1-x0x-rmx`. It records agreed requirements; the runtime-only
sequencer integration status is described below. Items explicitly marked unresolved require a decision or repository
inspection before implementation; they must not be inferred from this document.

## Scope

- Remix applies independently to `909`, `808`, `303A`, and `303B`.
- Each of these four tracks has its own Remix selection.
- `BREAK` is completely unaffected.

## Selection Model and Lookup Data

The user-facing selection consists of Bank **A–P** and Pattern **1–16**. Internally,
both dimensions are zero-based **0–15**. Together they select one of **256** Remix
patterns (`16 × 16`). Each Remix pattern contains **16 source-step selections**.

**A1** is the neutral sequential mapping: source steps `1..16` in user-facing
notation, or `0..15` internally. A1 remains a valid neutral mapping regardless of
the eventual Remix ON/OFF design.

The exact **4096-value** table (`256 × 16`) must come from the project's existing
historical Kontakt/Luminary implementation. Its contents must be imported and
validated separately. Do not invent or substitute table contents.

## Core Playback Model

Remix changes which stored source step supplies the data for each chronological
playback slot. Chronological playback position and source-step position are
separate concepts and must remain separate in the implementation.

- Step 1 is always first, matching Orb behavior.
- Repeated source steps are valid.
- Remix may select any source step within the relevant 16-step block.

The lookup mapping determines source data; timing follows chronological playback
position. The exact integration with existing X0X sequencer paths requires
repository inspection before implementation.

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

Preserve existing X0X 303 semantics wherever possible while making chronological
lookahead Remix-aware. Inspect the current note, TIE, slide, and lookahead paths
before implementing this integration. Any behavior not determined by these
requirements and verified existing semantics remains unresolved.

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

The integration of Remix with existing X0X directions requires inspection and an
explicit decision wherever the requirements above do not determine behavior.

## Persistence and Storage

Avoid increasing persistent pattern/project structure sizes if possible. Prefer
existing reserved bytes for Remix selection and state. Inspect current structures
and persistence semantics before choosing a layout.

Large Remix lookup data must live in `const`/flash, not writable static RAM.
Keep the source representation readable during development. Build-time or
generated nibble packing into 4-bit entries is preferred over manually maintained
packed hexadecimal data. A packed 4096-entry 4-bit table occupies **2048 bytes**.

The exact reserved-byte layout and Remix ON/OFF encoding are **unresolved**
implementation decisions to finalize after inspecting current structures.

## User Interface

The UI must expose Bank **A–P** and Pattern **1–16** separately for each applicable
track.

An explicit Remix ON/OFF control is desired. Its exact UI and persistence encoding
are **unresolved** and must be finalized during implementation design. A1 remains
a valid neutral mapping regardless of that design.

## Implementation Discipline and Validation

### First sequencer integration (runtime only)

The sequencer now maps source reads for Forward playback only. `seq_track_t.pos`,
step callbacks, length/wrap/bar handling, transport, cueing, mutation cadence and
swing remain chronological. Drums read hit bits and accent from one mapped
column. Bass reads the mapped current event and maps the next chronological slot
separately for TIE lookahead; previous-slide state still belongs to the previously
played event. Reverse, Ping-Pong and Random bypass Remix for bass; drums have no
direction setting. BREAK is unchanged.

`seq_set_remix()` is a temporary internal control for each eligible track, using
zero-based bank/pattern and an enable flag. Call it while stopped or between audio
blocks. All selections default OFF in `seq_init()`. They survive transport and
pattern switches, but are not stored in projects. No UI or persistent layout is
implemented in this step.

Motion, automation and p-locks are not mapped: the existing step callback receives
the chronological slot and its existing directional lookahead. With Remix active,
the mapped source event therefore hears the chronological slot's parameter and
probability lane values, not those of its original stored source slot. Conditional
probability cycles still use chronological pattern wraps. Drum chance is still
checked per triggered voice; repeated source hits get the existing chance checks
on each visit. Bass chance remains checked by the existing note-on sink: a TIE
that continues a sounding note does not add a new chance check, while a TIE after
release still triggers the existing note-on path. The sequencer's sounding/slide
state and MIDI behavior when the bass sink rejects a note are unchanged. Future
decisions about source-following automation or probability are unresolved here.

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

## Unresolved Before Implementation

Every item below requires an explicit decision, repository inspection, or separate
validation before implementation:

- Locate, import, and validate the exact historical Kontakt/Luminary 4096-value
  table. Confirm its representation and its agreement with the required neutral
  A1 mapping and step-1-first behavior; do not invent missing values or silently
  resolve conflicts.
- Inspect current sequencer execution paths and determine how to keep
  chronological playback position separate from mapped source-step position,
  including swing, pattern length, and 16-step block handling.
- Inspect existing 303 note, TIE, slide, and lookahead semantics. Determine the
  integration needed to preserve them while following chronological Remix slots,
  including sustained portions selected without their original note-on. Any
  behavior left unspecified requires an explicit decision.
- Inspect drum step storage and playback to verify that mapping selects the
  complete column, including all voice hit bits and accent state.
- Inspect existing X0X directions and reverse behavior. Decide any Remix/direction
  interaction not specified here while preserving existing directions unless an
  explicit decision authorizes a change.
- Decide the naming and UI treatment for conventional X0X reverse and Orb-style
  musical reverse.
- Decide exact musical ping-pong behavior.
- Decide exact musical-reverse behavior for partial lengths and multi-block
  patterns.
- Inspect persistent pattern/project structures, reserved bytes, and persistence
  semantics. Finalize the exact per-track selection/state layout while avoiding
  structure-size increases where possible.
- Finalize the desired explicit Remix ON/OFF control's UI, behavior, and
  persistence encoding. Do not infer those details from the neutral A1 mapping.
- Inspect current UI and motion paths and determine their integration with the
  agreed per-track Bank/Pattern selection. Any unspecified motion behavior
  requires an explicit decision.
- Choose the readable table source and generation/packing approach; verify that
  the resulting lookup data resides in const/flash rather than writable static
  RAM.
- Inspect the current architecture to confirm whether dedicated `seq/remix.c` and
  `seq/remix.h` fit the lookup/mapping implementation.
- Inspect existing tests and determine the focused regression coverage and
  validation paths for the requirements above.
