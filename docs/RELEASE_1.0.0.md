# X0X-RMX v1.0.0 — Stable release text (local preparation)

Publication is blocked until final FM1 testing and explicit approval. No tag or public
release exists as part of this preparation. Use the text below for the eventual GitHub
release, with tag `v1.0.0`, target the approved commit on `orb-remix`, and **Pre-release off**.

## Prepared release description

X0X-RMX 1.0.0 is the first official stable version of this fork of Charles Vestal's X0X:
909, 808, two 303s and BREAK, with independent Orb-inspired Remix for the four synth parts.

The final P9 changes are:

- USB AUDIO defaults to FULL for new projects and Factory Reset. Saved choices survive.
- SAVE and AUTOSAVE remember the manual pattern combination for all five parts and restore
  it on startup. Song and automatic chain changes do not replace the manual combination.
- Built-in 909 GR and 808 GR: distinct two-bar grooves at 96 BPM, rendered during the build.
- 303 SEQ pages: SEQUENCE → REMIX → KEYS. KEYS retains FREE PLAY ON/OFF only.
- Remix knobs, left to right: APPLY, BANK, PATTERN, REMIX ON/OFF. Musical mappings and timing
  are unchanged.
- Windows MIDI names such as FM-1 Midi 2 work with the browser break uploader. The protocol
  handshake is retained.

Package: `x0x-1.0.0.fwsc`. Device/product identity: `FM-1_9010000`. Display: `X0X 1.0.0`.
Existing development firmware `FM-1_900` uses the retained update path. Existing project
patterns, sounds and samples remain compatible. Old projects without a saved startup
combination begin with P1 on all five parts. Normal OTA does not erase the three user sample
slots. Keep backups of projects and samples before updating.

This release also contains the fork's Remix, Grids, extended TB-3PO, LCD schemes and existing
303 EXTRA/Morph implementation. Morph retains its one-voice-at-a-time restriction. No new
musical feature or button assignment is introduced by the final P9 update.

The demonstration video is an earlier **browser-emulator recording**, not device footage.
Its closing validation card predates the P9 observations. It does not certify final hardware
validation. Final hardware results must be added here before publication.

## Local artifacts and final publication steps

- Firmware: `build/x0x-1.0.0.fwsc`; checksum and budgets: [P9 report](P9_REPORT.md).
- Device checklist: [P9_FM1_TEST.md](P9_FM1_TEST.md). Record all outcomes and exact package hash.
- Local website preview remains package-free, with a reserved installer.
- After successful device tests and explicit approval, create the `v1.0.0` tag on the
  approved `orb-remix` commit and a GitHub **stable** release with the package and checksum.
- Then generate the approved website using the retained checked mode:
  `python3 tools/make_pages.py --validated-release build/x0x-1.0.0.fwsc 1.0.0 build/pages-v1.0.0`.
  The prepared installer metadata points to `../firmware/x0x-1.0.0.fwsc`; the package identity
  and loader checks still run. Verify links, checksum, version and installer metadata locally.
- Publish the approved Pages tree by a normal history-preserving commit; no force-push.
  Keep `main` unchanged. The final download URL is
  `https://mr-roboto.github.io/fm1-x0x-rmx/firmware/x0x-1.0.0.fwsc` and the installer URL is
  `https://mr-roboto.github.io/fm1-x0x-rmx/install/`. These are planned links, currently disabled.
- Verify public downloads, installer identity, emulator and manual after publication.

Do not execute the publication steps while the hardware/approval gate is open.
