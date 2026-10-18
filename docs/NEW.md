# X0X-RMX version history

X0X-RMX v1.0.1 is the current Stable release; v1.0.0 remains available historically. See the [manual](MANUAL.md),
[GitHub release](https://github.com/Mr-Roboto/fm1-x0x-rmx/releases/tag/v1.0.1),
[installer](https://mr-roboto.github.io/fm1-x0x-rmx/install/) and
[public emulator](https://mr-roboto.github.io/fm1-x0x-rmx/emu/).

## 1.1.0 · RC2 · hardware validation pending

RC2 retains the three RC1 features and adds experimental USB project
backup/restore for one normal-device validation session. Only supported RMX
backup files are accepted; uploaded break audio and calibration are excluded
and preserved. The local Chrome/Edge page is not publicly deployed. USB's
final inclusion and Stable publication remain subject to the maintainer's decision.
See [current draft notes and exact checksum](RELEASE_1.1.0.md) and the
[RC2 checklist and WSL preview](FM1_TEST_1.1.0_RC2.md).

## 1.1.0 · RC1 · hardware validation pending

The development candidate adds G.711 µ-law storage for built-in 909 cymbals,
BREAK FILTER (LP/OFF/HP), and ENV + KNOB 1–4 reset to descriptor defaults.
Existing project layouts and v1.0.1 direction values remain intact; older
projects initialize BREAK Filter to OFF. This candidate is not a public release.
See [original RC1 notes, exact package and checksum](RELEASE_1.1.0_RC1.md) and the
[short FM-1 validation checklist](FM1_TEST_1.1.0.md).

## 1.0.1 · Stable · 18 October 2026

- Independent firmware identity: `FM-1_987010001` in both the package and USB response.
  The official v1.0.0 package, identity and SHA-256 are unchanged.
- Five 303 directions in order: **FWD → REV → MREV → PING → RND**. MREV starts
  at step 1, then traverses the remaining configured steps backwards. It bypasses
  Remix while preserving its selection and ON state; Forward restores Remix.
- **Compatibility change:** stored values are 0 FWD, 1 REV, 2 MREV, 3 PING, 4 RND.
  No direction migration is performed. Existing v1.0.0 PING patterns (2) now use
  MREV; existing RND patterns (3) now use PING. Reselect the desired direction
  and save. Storage sizes and layouts remain unchanged.
- A user WAVE selection rejected by the single-MORPH rule shows exactly
  **MORPH IN USE**, through either the knob or SEL parameter list. The rejected
  voice retains its accepted waveform. Motion and loading remain silent.
- Hardware checklist items 1–8 passed on a real M-VAVE FM-1, including save/restart
  and existing project/user-sample data. Stable operation had no audible dropouts
  or unexpected interruptions. External MIDI clock/dedicated extended performance
  testing and physical interrupted-update recovery were not performed. Publication
  is explicitly approved with these limitations. No upstream X0X changes are
  included; those are reserved for v1.1.0.

See [v1.0.1 release notes and validation](RELEASE_1.0.1.md).

## 1.0.0 · Stable · 16 October 2026

Hardware testing confirmed the regular firmware update, preservation of existing patterns
and user breaks, restoration of the five manual startup patterns, pattern-chain playback
and a twelve-minute musical run without reported problems. USB audio with iPad/AudioShare
was also confirmed successful. Real-device operation of the sequencer, drums, bass synths,
effects and breakbeat engine was confirmed. Separate FULL/FOLLOWS KNOB hardware tests were not reported
and are not claimed. Publication was explicitly approved by the maintainer.

- USB AUDIO defaults to FULL for new projects and Factory Reset; saved choices remain intact.
- SAVE/AUTOSAVE remember the last manual startup pattern combination for all five parts.
  PRESETS and HOME pattern keys are supported; automatic Song/Chain changes do not replace it.
  Older projects start at P1 on all parts; the packed pattern/song/motion format is unchanged.
- Two built-in loops: 909 GR and 808 GR, distinct two-bar grooves at 96 BPM, rendered during
  the build. Their 110,250-byte combined ADPCM payload is unchanged in size. The three user
  slots, their addresses and upload protocol are unchanged; ordinary OTA preserves them.
- Both 303 SEQ pages now follow SEQUENCE → REMIX → KEYS. KEYS retains only FREE PLAY.
- All four Remix parts use KNOB 1 APPLY, KNOB 2 BANK, KNOB 3 PATTERN, KNOB 4 REMIX ON/OFF.
  The musical mapping, saved Remix values and APPLY timing are unchanged.
- Browser break uploader recognizes Windows names such as FM-1 Midi 2 and still requires
  the X0X protocol handshake on an identically named input/output pair.
- Official package: x0x-1.0.0.fwsc; product FM-1_9010000; Git tag v1.0.0.
  SHA-256: eccacfa995efec6b9c819583e7bce43b3aa7a126033c8e668ae89c6a988b754c.

### Included fork changes

- Independent Orb-inspired Remix for 909, 808, 303A and 303B; BREAK is unaffected.
  SEQ > REMIX exposes Bank A–P, Pattern 1–16, ON/OFF and STEP/BEAT/BAR/LOOP APPLY.
  Forward playback and sound-motion recording follow mapped source steps; bass Reverse,
  Ping-Pong and Random retain their existing behavior.
- Grids MAP/EUCLID drum generators with NEW/MUTATE, FILL, OFFSET, CHAOS and Undo integration.
- Extended TB-3PO: 22 scales, Minor Pentatonic defaults for new configurations, short
  motifs with rhythmic answers and restrained mutation. Written notes are retained when
  generator knobs change; older saved scale indices are not automatically migrated.
- Six saved LCD schemes, coordinated part colors, and retained GLOBAL list position
  during a session. WHITE LILAC is the factory default for new projects and reset;
  saved scheme selections and legacy DEFAULT decoding are preserved.
- Public X0X-RMX browser emulator, documentation, stable firmware download and web installer.
- Both 303s have TONE, VOICE and EXTRA EDIT pages. EXTRA exposes MORPH, ACCDEC, DRVTYPE and
  TUNE; only VOLUME remains under MORE.
- Browser-only Device color selector: Cream & Lilac by default, Classic Dark with the
  original appearance, plus Sunset Orange and Pink Lilac exterior variants inspired by
  the existing LCD palettes. Device appearance and GLO > SCHEME remain independent.
  The emulator and landing page share the project SVG favicon.
  The visual choice is stored separately and can change during playback.

### Experimental Morph waveform

- **Experimental Wave 3:** the 303 WAVE choices are Saw, Square and Morph. On EDIT's **EXTRA** page,
  **KNOB 1 (MORPH)** moves continuously through 0 = Sharkfin, 64 = Trapezoid and
  127 = Notch-Trapezoid, with independent settings and a 5 ms slew for each voice.
- Only 303A or 303B may use Morph at a time. Conflicting UI, direct parameter and motion
  requests keep the prior accepted Wave; they do not force the other voice to change or wait
  for ownership to become available. Returning the owner to Saw or Square releases the limit.
- Projects save accepted Wave settings and both Morph values in the existing sound allocation.
  A conflicting loaded pair keeps A on Morph and loads B as Saw.
- The implementation uses cubic four-box BLEP/BLAMP, replacing the original triangular
  correction proposal for Wave 3. Saw/Square, the 2× default and overload guard are unchanged.
  The release hardware run reported no problems; dedicated Morph worst-case timing and
  listening tests were not separately reported. These fork changes do not describe upstream releases. See the [local manual](MANUAL.md#6-303a-and-303b) and
  [waveform specification](TB303_WAVEFORMS_DESIGN.md).

## Original X0X history (Charles Vestal)

The numbered entries below are retained upstream history. Their release, installer and
hosted emulator links refer to original X0X, not X0X-RMX or a validated fork release.
Hardware performance figures and release claims below are the original author's historical
reports; they are not new measurements or release claims for this fork.

## 1.0.6 · 11 October 2026

- **Free play on the 303s:** press SEQ, turn SELECT to the KEYS page and set FREE PLAY to ON. The keys
  play the 303 on every 303 screen without writing anything, so you can try lines and shape the sound
  first; REC still records.
- **The song plays from the SONG screen:** press PLAY there and the song plays from the selected bar,
  and keeps playing wherever you go. PLAY anywhere else plays the patterns. The MODE setting is gone.
- **GLO > USB AUDIO:** FULL keeps USB audio at full level whatever the MASTER knob says, so you can
  record over the cable with the speaker turned down. FOLLOWS KNOB (the default) is as before.
- GLO is in sections: MIDI, AUDIO, KEYS AND SCREEN, PROJECT, SYSTEM.

## 1.0.5 · 10 October 2026

- A sharper check on the second processor core: on an FM-1 whose second core misreads, X0X now also
  catches the one misread 1.0.4 could miss, and carries on with one core. Nothing changes on an FM-1
  whose cores both work.

## 1.0.4 · 10 October 2026

- On FM-1s whose second processor core doesn't work reliably, X0X notices and carries on with one core,
  instead of clicking or stopping in the middle of an update. GLO > PERFORMANCE says 2 CORES, or
  1 CORE: BAD READS on such a unit. Thanks to Jangada's maintainer for the measurements and the fixes.
- A crash on the second core no longer freezes the FM-1.

## 1.0.3 · 10 October 2026

- GLO > BATTERY shows the battery's charge: FULL, 2/3, 1/3 or EMPTY, or USB POWER while a computer powers
  the FM-1.

## 1.0.2 · 10 October 2026

- Probability cycles count from the first pass again when the 909 changes pattern, as the other parts
  already did: switch patterns and a 1:4 step plays on the new pattern's first bar.

## 1.0.1 · 10 October 2026

- Probability cycles: hold a step and turn SELECT down past 5% for 1:2, 2:2, 1:3 … 8:8. A step at 1:4
  plays on the first of every four passes of its pattern, 2:4 on the second.

![STEP 15: 1:4, the open hat on the first of every four passes](img/new/1.0.1-cycle.png)

## 1.0 · 10 October 2026

### Highlights
- Uses both of the FM-1's processor cores: no dropouts, even on the most complex patterns. The busiest test pattern went from about 115% to 68% load. Thanks @keremcode
- Simpler navigation: HOME for the pattern and song, SEQ for the selected part's steps, EDIT for its sound. Press a button again, or turn SELECT, for the next page.
- Clearer screens: each knob row says whose knobs they are, a hint line shows what the keys do, holding any button explains it, and labels are clearer throughout.
- Up to 64 steps per part and 32 patterns
- Per-step p-locks (on EDIT, hold a step and turn a knob) and probability (hold a step and turn SELECT)
- A drum mixer with per-track mutes, sends and hit meters
- A new [break tool (original X0X)](https://charlesvestal.github.io/fm1-x0x/breaks/): load and manage your own loops from the browser (BR1, BR2…)

![HOME: a line per part in every pattern with notes](img/new/1.0-home.png)
![The 303's steps on SEQ, two ties as one long note](img/new/1.0-303-steps.png)
![A p-lock: the kick's own TUNE on step 5](img/new/1.0-plock.png)
![Probability: STEP 3 at 50%](img/new/1.0-probability.png)
![The 909 mixer, its meters lit by a hit](img/new/1.0-drum-mixer.png)
![MASTER: the output level, the pump at work](img/new/1.0-master.png)
![The song, bar 1 played four times](img/new/1.0-song-repeat.png)

### Sequencing
- New 303 note entry: REC turns the keys into a keyboard, ties show as one long note, and holding a step + OCT moves it an octave
- Copy and paste whole patterns: hold SAVE, then press a white key
- Song bars can repeat, ×1 to ×8
- A break step can have its own slice: hold the step and press black keys 1–8
- The break's COMPLEXITY now eases ROLL and FILL in gently

### Sound and mix
- Reverb and delay sends on both kicks
- DECAY on the 909 snare
- One-knob COMP and FILTER on the master, and the MASTER page shows the output level
- A calmer factory reverb
- OUTPUT LOUD, for quiet headphones
- The 808's hats always choke like the 909's (CHOKE setting removed)

### MIDI
- A MIDI channel per part, in and out, or OFF
- A steady tempo readout when following an external clock

### Feel and fixes
- Knobs: no more missed or doubled clicks, and small turns are exact
- Key lights in two brightness levels: bright for chosen, dim for available
- Saving waits until you stop, so the audio never cuts out
- B CHANCE works without PHRASE
- CLEAR PATTERN no longer leaves a bar showing on HOME
- The keys no longer trigger steps or slices on the MASTER and FX screens

Projects saved with 0.10 load as they were.

## Earlier versions

<details><summary>0.10.3-beta · 8 October 2026</summary>

- **PLAY is green**: PLAY uses its green light, and turns red only while recording (REC on). Its glow is green too.

</details>

<details><summary>0.10.2-beta · 8 October 2026</summary>

- **Readable buttons in the dark**: the buttons that aren't lit now glow dimly, so their labels can be read on the black FM-1. The KEY LIGHTS setting is now LIGHTS: ON (the default, and what a saved "on" becomes) shows the key lights and the glow, KEYS shows the key lights only, OFF neither.
- The performance page also reports how deep the interrupts' stack has gone.

</details>

<details><summary>0.10.1-beta · 6 October 2026</summary>

- No more zippering when you turn volume, pan or the sends quickly: part volume, pan and sends, the master volume and every drum voice's pan now glide over about 10 ms instead of jumping.

</details>

<details><summary>0.10-beta · 6 October 2026</summary>

- **Fixes the brightness freeze**: setting BRIGHTNESS low could freeze the FM-1, and it then stayed stuck at the boot screen (dim, no input) even after reinstalling. The setting is gone, a saved value is ignored, and the screen is always at full brightness. If your FM-1 is stuck, install 0.10-beta from the web installer.
- **Stereo**: every part has a PAN (its SENDS page, or MIX > PANS), and so does every 909 voice and 808 track (the last setting on its page). With everything centred it sounds exactly as before.
- **Stereo reverb**, and a **ping-pong delay** (PING on the FX TAPE page; delay times over one second stay centred).
- **TRS MIDI IN**: notes, clock and transport from the TRS jack, the same as over USB.

</details>

<details><summary>0.9-beta · 5 October 2026</summary>

- **USB audio**: the FM-1 shows up on a computer as a 44.1 kHz audio input ("X0X FM-1"), no driver. It is resampled to the computer's USB clock, so live monitoring (an AU host, a DAW) doesn't drift or drop out.
- **USB MIDI clock and transport fixed**: clock, Start, Stop and Continue sent over USB were ignored; X0X now follows a DAW's clock and transport.
- **Fewer dropouts on busy patterns**: audio renders in 11.6 ms blocks, so a heavy step no longer overruns (adds 5.8 ms of latency).
- **808**: voices stop once their tail is 60 dB under the hit, saving CPU without changing what you hear.
- **303**: holding a step no longer toggles it (only a tap does); holding or editing a step follows KEY SOUND like the drum keys, so it's silent while the pattern plays.
- **TB-3PO**: the GENERATE and SCALE knobs no longer rewrite the line; they set up the next **OCT+** (new line) or **OCT-** (mutate). The screen marks changed settings with a star.

</details>

<details><summary>0.8-beta · 5 October 2026</summary>

- Fast knob turns count: a quick flick no longer gets lost (the encoder decoder keeps up with skipped states)
- Overload guard: when the FM-1 nears its CPU limit, the 303s drop oversampling and the 808's tails end sooner instead of dropping out; GUARD shows next to the audio load, and the performance page counts how often it engaged
- Less screen tearing: the display runs at 60 MHz, the main area sends only what changed, and both halves change together. Some tearing remains: the FM-1 has no tearing-sync line

</details>

<details><summary>0.7-beta · 5 October 2026</summary>

- Knob acceleration by speed: turn slowly for single steps, quickly to sweep (a fast half turn covers the whole range).
- GLO > BRIGHTNESS: the screen's brightness, 1 to 8.
- tools/upload_breaks.py: no longer crashes when given more loops than fit; retries a write the USB link lost.

</details>

<details><summary>0.6-beta · 5 October 2026</summary>

- Knobs and LEDs under load: the key/knob/LED matrix scan now keeps its rhythm while the audio renders. Fast knob turns no longer lose their steps, and the LEDs no longer flicker with the audio load. (Found on the first hardware run.)

</details>

<details><summary>0.5-beta · 5 October 2026</summary>

- Undo and redo: HOME + REC / HOME + PLAY, up to 32 steps, for sounds, patterns, the song and knob motion (CLEAR PATTERN and FACTORY RESET included). The screen says what was undone.
- AUTOSAVE (GLO, on by default): while stopped, changes save themselves after 4 s of quiet. Saves now write only what changed.
- Help cards: hold any button for a second to see what it does on the current screen, with its combinations.
- Safe mode: after two failed starts X0X starts with no sound and USB on, so the web installer can always reach it (it used to drop into the chip's update mode, which no installer can see). PLAY tries again.
- After a crash, the next start says so and shows where.
- The web installer can put M-VAVE's stock firmware (FM-1 V15) back.
- The browser version can load your own breaks.

</details>

<details><summary>0.4-beta · 4 October 2026</summary>

- Black drum keys select silently while the pattern plays (new GLO > KEY SOUND, default STOPPED); they still sound when stopped and with REC on. ALWAYS keeps the old behaviour.
- The 303 lines start empty, like the drums: a fresh FM-1 (or FACTORY RESET) no longer plays two generated lines. OCT+ on TB-3PO writes one. Saved projects are unchanged.
- Delay: a division longer than the 2 s line (1/2. below 90 BPM, 1/2 below 60) now halves and stays on the beat instead of being cut off the grid.
- GLO > ABOUT shows the real version (every release said 0.1).
- New: X0X in the browser, https://charlesvestal.github.io/fm1-x0x/emu/
- New: recovering an FM-1 that won't start, from a Mac (manual, Troubleshooting).

</details>

<details><summary>0.3-beta · 4 October 2026</summary>

- Fixes the crash on hardware ("X0X CRASH" screen soon after start): a divide-by-zero trap fired in the master limiter. The trap is off and the divide is clamped.
- Includes the PERFORMANCE screen and PERF TEST (GLO menu) from 0.2-beta.

</details>

<details><summary>0.2-beta · 4 October 2026</summary>

**Still not run on a real FM-1.** This build measures itself, so the first install tells us what the FM-1 can do. Installing is at your own risk; M-VAVE's updater (M-UPGRADE) puts the official firmware back.

**Please run the performance test:** GLO > PERFORMANCE, then SEL > RUN PERF TEST. It plays three built-in patterns for about 15 seconds, puts your project back as it was, and shows a table. A photo of that screen is the most useful thing you can send.

- **Install:** https://charlesvestal.github.io/fm1-x0x/install/
- **Manual:** https://charlesvestal.github.io/fm1-x0x/manual/

Changes since 0.1-beta:
- PERFORMANCE page: clock, audio load and peak, dropouts, each part's share of the CPU; RUN PERF TEST
- Big-beat factory mix (break up front, 909 kick under it, RAT 303s into tape delay, a pumping master)
- No clicks on the kick: the 909 kick's attack back to 9W9's, the pump eases in, a look-ahead limiter
- Cheaper engines: silent parts skipped, lighter master, faster tanh, a RAT clipper without square roots or divides
- The manual rewritten in plain sentences, with a Getting started tour

</details>

<details><summary>0.1-beta · 4 October 2026</summary>

First public build. **Not yet run on a real FM-1**: the first people to install it are the first test. Installing is at your own risk; M-VAVE's updater (M-UPGRADE) puts the official firmware back.

- **Install:** https://charlesvestal.github.io/fm1-x0x/install/ (Chrome or Edge, the FM-1 on USB), or `python3 tools/fm1_install.py x0x-0.1-beta.fwsc` with the file below.
- **Manual:** https://charlesvestal.github.io/fm1-x0x/manual/

What's in it:
- A 909 (from 9W9 / ER-99) and an 808 (from 8W8), eleven tracks each
- Two 303s (from schwung-303 / Open303), each with a TB-3PO line generator
- A breakbeat generator (from BB Gen); load your own breaks with `tools/upload_breaks.py`
- 16 patterns, each part on its own pattern; a song mode; recorded knob moves
- Reverb and tape delay sends on every part; a master compressor with kick-keyed pump, a filter and a limiter
- A big-beat factory mix

</details>

