# Live drum rolls and percussion fills

Five momentary patterns use Live mode's formerly spare black keys. The inspected
keyboard mapping has 16 white and 11 black keys; zero-based black indices 5–9
map to physical key indices 13, 15, 17, 20 and 22. Existing black keys 1–5 still
mute parts; key 11 still selects immediate versus bar-queued 303 transpose.

| Black key, counted from the left | Label | Pattern |
|---|---|---|
| 6 | R1 | 909 snare, eighth notes |
| 7 | R2 | 909 snare, sixteenth notes |
| 8 | R3 | 909 snare, thirty-second notes |
| 9 | P1 | 808 clap with snare pickups |
| 10 | P2 | 909 high tom with 808 clap accents |

Press to enable, release to stop future hits. Any combination may be held.
Patterns repeat over a four-quarter-note bar and use the supplied fixed velocity
scores in `sequencer.c`. There are no added menus or persistent settings.

## Transport and arbitration

The existing Live transport clock is 24 ppqn, with 96 ticks per bar. Every three
ticks provides the common 1/32 grid; the slower rolls and percussion scores use
subsets of it. Keys never restart the clock. A key pressed between grid points
waits for its next eligible point. Internal audio splitting uses the existing
sequencer event scheduler; external MIDI clocks advance the same phase.

Live hits for the same drum at a timestamp merge by maximum velocity. Different
voices remain independent. An accepted stored sequencer hit has priority over a
coincident live hit, retaining its original velocity and already-applied parameter
locks. If that stored hit is muted or rejected by its chance lane, the live hit
still plays. Sequence mutes and chance do not suppress live fills.

All hits use the existing 909/808 voices, mixer, pan, sends and effects, including
the Live DRUMS filter. No DSP synthesizer, continuous effect, heap allocation or
wall-clock timer was added. The existing snare retrigger path is retained.
P1 uses the 808 clap/snare to contrast with R1–R3. P2 uses the 909 high tom with 808 clap accents. Its live high-tom hits retain
+9 dB of per-hit gain; ordinary sequenced hits and saved voice/mixer levels
are unchanged. Velocity variation and normal mixer controls
still apply.

## Key lifecycle

Held state is a five-bit mask. Repeated key-down events do not enqueue another
activation. Release clears only its own bit, leaving current drum tails alone.
Leaving Live or stopping transport clears all fill activations; restarting does
not resume a stale held state. Keys pressed while stopped do not arm a fill.
Live LEDs and the on-screen R1/R2/R3/P1/P2 labels identify held patterns.

## Validation

Changed implementation files are `firmware/src/app/ui.c` (pads and indicators),
`firmware/src/app/engine.c` and `.h` (audio-thread commands and live chance bypass),
and `firmware/src/seq/sequencer.c` and `.h` (scores, shared-clock scheduling and
collision arbitration). The manual, test runner and new native/browser fill tests
complete the feature.

`tests/host/live_fills_test.c` compares the fixed score against all 31 nonempty
key combinations at 135, 150 and 155 BPM, with 1-, 17- and 256-frame blocks. It
also checks phase-aligned entry/switching, tempo changes, external MIDI clock,
maximum-velocity merging, sequence collision priority, chance/mute bypass,
physical key mapping, key release/exit/stop cleanup and unchanged saved patterns.
The real engine test exercises the existing snare, clap and percussion voices,
mixer and effects together for eight seconds, plus collision/parameter-lock
behavior and natural release tails.

Hardware listening and dropout checks with R3 + P1 + P2 over a busy project are
still required: host timing and finite/nonzero rendered audio cannot establish
the FM-1's worst-case real-time headroom or subjective retrigger quality.
