# X0X-RMX v1.0.0 — Stable

**Successfully tested on a real M-VAVE FM-1.**

The first official stable release of X0X-RMX, a fork of Charles Vestal's X0X for the M-VAVE
FM-1: 909, 808, two 303s and BREAK, with independent Orb-inspired Remix for the four synth parts.

## Final P9 changes

- USB AUDIO defaults to FULL for new projects and Factory Reset; saved choices survive.
- SAVE and AUTOSAVE remember the manual pattern combination for all five parts and restore
  it on startup. Song and automatic chain changes do not replace the manual combination.
- Built-in 909 GR and 808 GR: distinct two-bar grooves at 96 BPM, rendered during the build.
- Both 303 SEQ pages: SEQUENCE → REMIX → KEYS. KEYS retains FREE PLAY ON/OFF only.
- Remix knobs, left to right: APPLY, BANK, PATTERN, REMIX ON/OFF. Musical mappings and timing
  are unchanged.
- Windows MIDI names such as FM-1 Midi 2 work with the browser break uploader. The protocol
  handshake is retained.

The release also includes the fork's Remix, Grids MAP/EUCLID, extended TB-3PO, LCD schemes
and existing 303 EXTRA/Morph implementation. Morph retains its one-voice-at-a-time restriction.

## Confirmed hardware testing

The maintainer confirmed a regular update from development firmware FM-1_900, preservation
of existing patterns and user breaks, restoration of all five manually selected startup
patterns, pattern-chain playback and a twelve-minute musical run without reported problems.
USB audio with iPad and AudioShare was also confirmed successful. The sequencer, drum
machines, bass synthesizers, effects and breakbeat engine operated successfully on the real
M-VAVE FM-1.

Separate hardware tests of FULL and FOLLOWS KNOB were not reported and are not claimed.
The maintainer explicitly approved this stable release; those separate tests are not an
additional publication condition. Dedicated Morph worst-case benchmarks were not separately
reported.

## Installation and compatibility

Package: **x0x-1.0.0.fwsc**. Product identity: **FM-1_9010000**. Display: **X0X 1.0.0**.
The hardware-tested package is published unchanged. No firmware rebuild or functional change
was made during publication.

SHA-256:

```text
eccacfa995efec6b9c819583e7bce43b3aa7a126033c8e668ae89c6a988b754c
```

Existing projects remain readable. Older projects without startup fields begin with P1 on
all five parts. Normal OTA preserves all three user sample slots; the hardware test confirmed
retention of existing user breaks. Back up projects and samples before updating. Lossless
project downgrade to older firmware is not guaranteed.

- [Web installer — Chrome or Edge, Web MIDI](https://mr-roboto.github.io/fm1-x0x-rmx/install/)
- [Firmware download](https://mr-roboto.github.io/fm1-x0x-rmx/firmware/x0x-1.0.0.fwsc)
- [Manual](https://mr-roboto.github.io/fm1-x0x-rmx/manual/)
- [Browser emulator](https://mr-roboto.github.io/fm1-x0x-rmx/emu/)
- [GitHub release](https://github.com/Mr-Roboto/fm1-x0x-rmx/releases/tag/v1.0.0)

Automated preparation passed 74 available test groups/scenarios, plus Chromium, website and
artifact checks. Three external DSP reference comparisons were skipped because their source
checkouts were unavailable. The DSP engines were unchanged by P9.

The demonstration video is an earlier **browser-emulator recording**, not physical FM-1
footage. Its outro was minimally corrected for v1.0.0 to state successful real-device validation,
while retaining the explicit browser-emulator label. The first 81.8 seconds are frame-identical
and the entire AAC audio stream is unchanged. The poster and approved banner are unchanged.
This does not imply that every individual hardware checklist item was separately tested.

X0X-RMX retains GPL-3.0 licensing and the existing upstream/third-party attribution.
