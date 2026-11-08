# Alpha Juno web experiment

The hardware-safe checkpoint is commit `76c57b1`: Live fills with the 909 high-tom
P2 and ENV resetting only Live FEEDBACK/DRUMS. The installed hardware package is
`build/x0x-harder-dev-p2-hightom.fwsc`, SHA-256
`c0483942d774c7d5f45ca39a03af8a86c946411e829c7b92d806c18759f05a9d`.

Both the web build and the hardware build now replace BREAK with JUNO.
The standard native host build retains BREAK for its existing reference tests;
`run_juno_hardware.sh` compiles the Juno hardware profile explicitly.

## Performance controls

JUNO now has mono/legato, glide, editable trance gate, a six-pattern arp and
a DJ filter on MIX. See [controls and saving](JUNO_PERFORMANCE.md).

## Playing

Choose **JUNO** with ALGORITHM. EDIT cycles through the sound pages; SELECT moves
between them. All 27 white and black keys play chromatic notes, F3–G5 initially.
OCT changes the keyboard octave. Notes keep their original pitch until released,
even when the octave, part or page changes. Six notes can sound at once; a seventh
reuses a released voice first, then steals the oldest held voice.

Computer keys and multitouch use the emulator's normal keyboard mapping. The
**Connect MIDI keyboard** button also accepts a real keyboard, including velocity,
sustain pedal, pitch bend, mod wheel and channel pressure. This dedicated browser
input sends every MIDI channel to JUNO; app MIDI routing still uses GLOBAL's
JUNO MIDI CH. **All notes off** kills voices and clears the chorus delay.

SAVE stores the patch, mixer and project in a separate browser storage namespace,
`x0x-emu-juno-store-v1`. The earlier emulator project and uploaded break samples
remain in their original storage. This prototype plays JUNO from the keyboard;
the former break step/slice pattern does not trigger synth notes.

## Oscillator update

The saw gates and companion pulse/sub relationships now follow the raw June-21
DCO equations. See [the correction and reference checks](JUNO_SAW.md).

## Presets

JUNO EDIT uses PRESETS to browse 64 named starting patches. A `*` marks edits;
SAVE keeps the edited patch and its preset name. The active pattern and mixer
are preserved. See [the complete bank](JUNO_PRESETS.md).

## Pages

| Page | Knob 1 | Knob 2 | Knob 3 | Knob 4 |
|---|---|---|---|---|
| DCO | Saw variant | Pulse variant | Sub variant | Sub level |
| OSC MIX | Noise | Range | Fine tune | Level |
| PWM | Pulse width | Depth | Rate | Source: LFO / ENV / manual |
| VCF | Cutoff | Resonance | Envelope depth | Envelope mode |
| FILTER | HPF | Key tracking | LFO depth | Velocity depth |
| ENV 1 | T1 | L1 | T2 | L2 |
| ENV 2 | T3 | L3 | T4 release | Key follow |
| PITCH | Envelope depth | Envelope mode | LFO depth | Bend range |
| LFO | Rate | Delay | Fade | Shape |
| VCA | ENV / gate / velocity modes | Velocity depth | Pressure depth | Drive |
| CHORUS | On/off | Rate | Depth | Mix |
| MIX | Part level | DJ filter | Reverb send | Delay send |

The envelope travels through L1, L2 and L3 over T1, T2 and T3, sustains at L3,
then releases to zero over T4. Normal/inverted and velocity-scaled envelope
routes feed pitch and filter. The raw DCO combines saw/pulse variants with
octave-divided/gated sub waves and noise. A resonant four-pole filter uses two
cascaded state-variable stages. Chorus uses one interpolated modulated delay.
JUNO goes through the existing mixer, sends and master; Live DRUMS excludes it.

## Implementation and validation

`alpha_juno.c` uses float arithmetic, six fixed voices, no allocation, no
oversampling and no anti-aliasing. Pitch/filter coefficients update every 16
samples; oscillator phases, envelope segments and filter state advance per
sample. Complete synth state, including chorus memory, occupies **4,644 bytes**
in the host build. This is not a hardware RAM or CPU measurement.

The hardware build links `alpha_juno.c` and `juno_part.c` instead of
`breaks.c`; the web bridge includes that same part implementation. The bridge preserves the fifth mixer lane and existing project sizes;
the 44 patch parameters use three existing T_BRK parameter banks. The web binary
does not include the break renderer, generator, built-in loop bank or upload UI.

Build with `sh web/emu/build.sh` using Emscripten. Focused checks:

- `sh tests/host/run_alpha_juno.sh`: pitch, distinct saw variants, voice stealing,
  and finite output/state across extreme pitches and controls.
- `node tests/host/juno_emu_test.mjs build/emu/x0x.wasm`: all 44 controls and
  twelve pages, chords, note release across UI changes, MIDI sustain/note zero,
  panic, audio and SAVE/reboot across all parameter banks.

## Hardware installation

The Juno state runs in the RAM pool, on the second core alongside the 808.
The 1,024-sample chorus buffer covers the full 817.5-sample maximum modulated
delay. All six voices are retained. Sound revision 9 initializes the new patch
when loading a previous hardware project and clears only used T_BRK automation
lanes. Drums, both 303s, mixer, pattern and song data keep their values. Existing
BREAK step/slice data stays stored but does not trigger synth notes. Save after
adjusting the new patch to keep it across restarts.

The browser uses its own project namespace and already contains Juno patches,
so its revision update preserves those patch controls. Native hardware migration
and all-bank SAVE/reload are checked in `run_juno_hardware.sh`.

Hardware build: image **376,004 B**, static RAM **96,932 / 98,304 B**, pool
**333,300 / 344,064 B**. The package is `build/x0x-harder-juno-dev.fwsc`;
USB identity stays FM-1_987 and the running editor identifies **X0X-HARDER JUNO DEV**.
The flash was verified, with before/after project backups under
`build/device-backups/harder-{before,after}-juno-flash.x0b`. The device's migrated
backup changes only the sound revision and Juno parameter area. All unrelated
sound, mixer, pattern, song and packed automation data compare byte-for-byte.

## Reference and licensing

Design reference: [mikerodd/june-21](https://github.com/mikerodd/june-21), specifically
[`june-21.csd`](https://github.com/mikerodd/june-21/blob/b46c09a0548cdbe1d23089c9406f3c67d4a83fb0/src/cabbage-module/june-21.csd).
June-21 is a GPL-3.0 Csound/Cabbage Alpha Juno 2 emulator. The waveform family,
four-stage envelope and routing informed this original C implementation; Csound,
presets, lookup tables and plugin binaries are not copied or required. The new
implementation follows this repository's GPL-3.0-only license. It intentionally
favours a raw musical approximation over circuit/sample-accurate emulation.
