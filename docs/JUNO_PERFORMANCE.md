# Juno performance controls

## SEL: EXTRA

Select JUNO and press SEL. The EXTRA section opens directly:

- **VOICE MODE:** POLY (six voices), MONO (last-note priority, envelopes retrigger),
  LEGATO (last-note priority, overlapping keys retain their envelopes).
  Releasing the newest key returns to a previous held key; releasing all keys
  releases the sound. Sustain pedal works with the voice modes.
- **GLIDE:** OFF at zero, then approximately 2 ms to 4 seconds. In mono/legato it
  slides the current voice to the new key. In poly mode each reused voice can
  glide from its own previous pitch.
- **TRANCE GATE:** OFF/ON. While this row is highlighted, the 16 white keys toggle
  the 16 gate steps, with lit keys indicating active steps. These presses edit
  the gate, not notes or sequencer steps. ALGORITHM changes the switch; HOME
  closes EXTRA and restores the keyboard. Gate editing also works with the
  switch off.
- **ARP LATCH:** holds the arp chord after keys are released. A new chord replaces
  the old latched chord. Switch latch off, or stop transport, to clear it.

The default trance pattern is `10110101 10110101` (steps 1, 3, 4, 6, 8, 9, 11,
12, 14, 16). It spans one four-beat bar at sixteenth-note resolution. Its edges
have a short amplitude ramp to avoid clicks. It gates Juno's output and effect
sends; existing shared effect tails continue naturally.

## ARP button

On JUNO, ARP toggles the arpeggiator and opens its control page. EDIT returns to
sound editing while the arp keeps running; press ARP again to switch it off.
The ARP light indicates when it is enabled.

| Knob | Control |
|---|---|
| 1 | UP, DOWN, UP/DOWN, DOWN/UP, AS PLAYED, RANDOM |
| 2 | 1/8, 1/8T, 1/16, 1/16T, 1/32, 1/32T |
| 3 | One, two or three octaves |
| 4 | Note length as a percentage of the arp step |

Up/down orders do not repeat endpoints. AS PLAYED preserves key order. Both
the device keyboard and MIDI keyboard feed the arp. Latch is in SEL → EXTRA;
MIDI sustain pedal can also hold an arp chord.

Arp and trance gate follow the sequencer's 24-ppqn clock, including external MIDI
clock. With transport stopped they run at the current tempo for keyboard playing.
Stopping transport clears held/latching input; it does not disable the saved arp
or trance-gate setting. Press keys again to start a fresh phrase.

## Juno MIX and patch 33

On JUNO's EDIT → MIX page, knob 2 is **DJ FILTER**: low-pass to the left, neutral
in the middle, high-pass to the right. It affects only Juno, including its sends.
Its normal pan setting remains available through the main mixer.

Preset **33 CLASSIC HOOVER** now loads **T1 = 70**. On first loading an older
project, an unadjusted T1 of 2 in that selected preset is updated to 70; other
edited attack values are retained.

## Saving and implementation

SAVE retains voice mode, glide, gate switch/pattern, DJ filter, arp settings and
latch alongside the current sound. Preset browsing keeps these performance
settings. Existing project and pattern sizes are unchanged: the new controls
occupy a previously unused sound bank, marked for one-time initialization.

The gate and arp share transport phase. Extra audio splits are requested only at
their required tick boundaries; when both are off the render uses its ordinary
block path. Mono note memory and arp chord/order storage are fixed size. No
allocation, oversampling or voice reduction is added.

Focused checks cover mono/legato retrigger behavior and fallback, glide, six arp
orders, octave/latch/pedal handling, exact internal sixteenth timing stopped and
playing, external clock phase, trance-gate audio/key/LED behavior, DJ-filter
stability, persistence and the T1 correction. The web emulator runs the same code.

Hardware build: image **387,304 B**; static RAM **96,940 / 98,304 B**;
pool **333,684 / 344,064 B**. Six-voice polyphony remains available.

## Juno step sequencer

On Juno, press SEQ for a 16-step line. While stopped, the four knobs are ROOT,
SCALE (the same 22 scales as the 303 generator), OCTAVE and NOTE LEN (percent of
one sixteenth). Defaults are C, minor pentatonic, octave 3 and 75 percent.
OCT up/down toggles the selected step's octave up/down flag; knob 3 remains the base octave.

Press a white key to select its step: an empty step turns on immediately; a
short tap on an occupied step turns it off, as on drums. The last selected step
stays selected after releasing the key. Black keys 1�11 assign successive scale
degrees, turn that step on and preview its note while stopped. Holding the white
key while assigning a note keeps an existing step on. Root, scale and octave
retune the stored degrees across the line. White LEDs show active steps and the
playhead; black LEDs show the selected step's scale degree.

During playback the four knobs expose the normal Juno EDIT sound pages, including
MIX. SELECT or SEQ browses pages. Hold a white step and turn a knob to set a
parameter lock; releasing that step keeps it enabled. Lock dots appear beside
steps. The existing REC motion workflow remains available. EDIT leaves SEQ for
the normal playable keyboard. The EXTRA trance gate editor retains priority
when highlighted, so its white keys continue editing the gate pattern.

The line follows the existing transport, pattern selection, probability lane,
part mute and internal/external clock. Sequence notes obey voice mode; their slides use sequence timing, independently of keyboard GLIDE;
the sequence plays directly alongside the independent keyboard arp. Note-off
timing runs even with arp and trance gate disabled. The Juno output has an
additional **3 dB** before the mixer and effect sends.

The retired break step/slice/settings bytes hold the Juno line, with a distinct
marker. Old break patterns initialize as empty Juno lines on first loading; drum,
303, song and motion storage sizes stay unchanged. SAVE/autosave persists this
initialization and subsequent edits.

Updated hardware build: image **389,484 B**; static RAM **96,928 / 98,304 B**;
pool **333,688 / 344,064 B**. Six voices remain available. Native checks cover
note length, shared-pitch keyboard ownership, step gestures, scales, playback,
locks applied before note onset and save/reload. A WASM interaction test covers
stopped knobs, note preview, playback, locks, stop and saved setup.

## Ties, sequence slides and octave flags

In Juno SEQ, select a white step, then tap ENV to cycle:
NORMAL -> TIE -> SLIDE -> TIE + SLIDE -> NORMAL.
The step stays selected after releasing the white key. ENV also works while
holding that key; releasing it after editing keeps the step enabled. Assigning
a black-key note converts a tie back to a pitched step, retaining slide/octave.

A TIE continues the previous voice without retriggering its envelope or entering
a new pitch. A SLIDE on a pitched step sweeps toward the next pitched step,
looking through intervening ties. Example: C3 + SLIDE, TIE, TIE, C4 sweeps one
octave over three sixteenth intervals, reaching C4 exactly at step 4, without an
envelope retrigger at that destination. A TIE + SLIDE can start a sweep from an
already-held note; if a sweep is in progress it continues to the same target.
Plain ties sustain pitch. A rest breaks both ties and slides; an orphan tie is
silent. Lookahead wraps through step 16 to step 1 but stops at any rest.

Tie/slide connections keep the gate open despite NOTE LEN. Ordinary notes and
the end of a tied phrase retain NOTE LEN gating. Sweeps use linear semitone
interpolation and follow tempo, swing and the external clock tempo estimate.
Each continuation step adjusts remaining sweep time to the current timing.
SEL -> EXTRA's GLIDE remains the keyboard/arp portamento setting and does not
change sequence slide duration. POLY keeps six voices; the sequence reuses its
own voice during ties/slides and preserves its oscillator/envelope state.

OCT+ toggles +12 semitones on the selected step; OCT- toggles -12. They clear
one another. They also work with the white step held. These buttons no longer
change the entire sequence's base octave. Ties contain no independent pitch;
any stored octave flag becomes audible when converted back to a pitched note.

Every step shows a right arrow for TIE, up/down arrow for octave flags and a
small rising stroke for SLIDE. The selected step is underlined. Its description
shows TIE or its note and octave, plus SLIDE where enabled. The octave buttons
light for the selected flags; ENV lights for tie and blinks for slide. These
flags use the high four bits of the existing per-step degree byte, so existing
Juno lines, project sizes and save compatibility are retained.

Slide build: image **391,452 B**, static RAM **96,928 / 98,304 B**, pool
**333,768 / 344,064 B**. Focused native tests cover exact pitch progression and
endpoint, envelope continuity, rests/orphan ties, loop lookahead, tempo/swing,
external 24-PPQN, independent keyboard glide, flags and save/reload. The WASM
check covers flag controls, octave exclusivity/pitch, display and persistence.

### Continuous modulation during ties

Pitch/filter control updates retain their 16-sample phase across render calls.
Sequencer boundaries can split a hardware audio block without restarting that
phase or forcing extra modulation updates. New voices still calculate their
pitch/filter on their first sample. Regression coverage compares preset 36
BRIGHT HOOVER held with transport stopped against a note continued through ten
or more ties: output is bit-identical through the complete mixer, effects and
master chain. Existing slide, clock, voice-mode and persistence checks remain.

Control-phase fix build: image **391,484 B**; RAM **96,928 / 98,304 B**;
pool **333,768 / 344,064 B**. All six voices retained.

### Juno output gain

Juno now has +15 dB total gain before the mixer and effect sends: another
+12 dB above the earlier +3 dB boost. This gain follows Juno's trance gate and
DJ filter, before the shared mixer, effects and output processing. Existing
patch levels, mixer controls and saved projects keep their stored values.
