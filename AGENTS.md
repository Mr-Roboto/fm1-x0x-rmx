# X0X-HARDER repository guidelines

## Identity, branches and attribution

- X0X-HARDER is custom M-VAVE FM-1 groovebox firmware and a matching web emulator.
- It builds on [Mr-Roboto's X0X-RMX](https://github.com/Mr-Roboto/fm1-x0x-rmx) and [Charles Vestal's X0X](https://github.com/charlesvestal/fm1-x0x), combining RMX features with selected upstream fixes and performance improvements.
- Juno's design and raw DCO gate relationships reference [mikerodd's June-21](https://github.com/mikerodd/june-21). Preserve its attribution and the existing Felucca, drum, Open303, TB-3PO, Grids and other component notices. See `LICENSING.md` and `docs/JUNO_SAW.md`.
- Keep project-specific work off `main`. Check the actual branch and remotes before making changes; do not assume the historical `orb-remix` branch is current. The integration checkpoint branch is `integrate/upstream-fixes-rmx-1.1.0`; `x0x-harder` is the proposed publication branch, not an assumed existing branch.
- `origin` has pointed to Mr-Roboto's repository and `charles` to Charles Vestal's repository. Verify remotes before publishing; the user's GitHub account is `deluded`, and its personal fork must not be confused with either upstream.
- Review upstream changes when integration is relevant; do not merge unrelated upstream work into a feature task automatically.
- Do not commit or push unless requested. A request to commit locally does not authorize a remote push.

## Current instruments and behaviour

- The hardware and web builds use `X0X_JUNO`: **909, 808, 303A, 303B and JUNO**. JUNO replaces BREAK; do not restore the break player, upload UI or loop bank into these builds unintentionally.
- The ordinary native host retains BREAK for historical reference tests. Juno-specific host runners explicitly select `X0X_JUNO`; passing a legacy host test alone does not validate Juno hardware behaviour.
- Juno retains six voices, 44 patch parameters and 64 original starting presets. PRESETS recalls patches on JUNO EDIT without replacing patterns, mixer settings or performance options. Preserve edited-preset identity and SAVE/reload.
- SEL opens Juno EXTRA: POLY/MONO/LEGATO, keyboard GLIDE, TRANCE GATE and ARP LATCH. ARP has six standard orders, six rates and one to three octaves. The trance gate editor uses the 16 white keys when its row is highlighted.
- Juno SEQ has 16 steps. Stopped knobs are ROOT, SCALE, OCTAVE and NOTE LEN. White keys select/toggle steps, black keys assign scale degrees; playback exposes sound pages and step parameter locks.
- ENV cycles NORMAL -> TIE -> SLIDE -> TIE + SLIDE. Ties continue the previous voice without retriggering. An outgoing slide looks through ties to the next pitched note, reaching it on its step boundary with envelope continuity. Rests break connections; orphan ties are silent. Lookahead wraps through the pattern and accounts for swing/tempo.
- Sequence slides use sequence timing, independently of the keyboard GLIDE setting. OCT +/- toggles the selected step's mutually exclusive octave flags; knob 3 retains the sequence's base octave. Preserve arrows, slide marks and LED feedback.
- Juno MIX uses LEVEL / DJ FILTER / REV / DLY; normal pan remains in the main mixer. Current Juno gain is **+15 dB total**, applied before mixer/sends (`5.62341325f`), including the extra +12 dB requested after the initial +3 dB.
- Both 303s have HPF and a fourth EDIT page for HPF/PAN/REV/DLY, all lockable. 303B can mirror 303A with a transpose offset.
- Live mode is the third HOME page from EDIT: ACID1/2, delay FEEDBACK and a DRUMS DJ filter. White keys transpose both 303s in semitones, key 8 = zero; the rightmost black key selects immediate/next-bar changes. Retain mutes, queued-transpose LEDs, momentary rolls/fills and ENV's feedback/drums reset. Live DRUMS filtering excludes Juno.
- Preserve 909/808 CH/OH velocity and locks, 909 OH hold, Orb-inspired Remix, Grids, 22 TB-3PO scales, Morph ownership, song mode, motion and the existing effects.
- Remix remains independent for 909/808/303A/303B; Juno is unaffected. Bank A-P / Pattern 1-16 and neutral A1 follow `docs/REMIX_DESIGN.md`.

## Source layout and references

- `firmware/src/dsp/alpha_juno.*` and `alpha_juno_wave.h`: voices, oscillators, envelope, filter and chorus.
- `firmware/src/dsp/juno_part.*`: keyboard/MIDI, voice modes, arp, gate, sequence voice ownership, DJ filter and gain. Historical `breaks_*` bridge names still connect the fifth engine.
- `firmware/src/seq/`: clock, patterns, scales, motion and Remix. `firmware/src/app/`: engine integration, UI and project migration.
- `firmware/hal/` and `firmware/loader/`: hardware support and updater. Keep register access in the HAL.
- `host/`, `web/emu/`, `tests/host/`, `tests/scenarios/`: host, shared web build and validation. `tools/` and `assets/`: build tools and source assets.
- Use `README.md`, `docs/JUNO_PERFORMANCE.md`, `docs/JUNO_PRESETS.md`, `docs/JUNO_SAW.md`, `BUILDING.md` and `LICENSING.md`. The inherited `docs/MANUAL.md` describes older BREAK behaviour; some `docs/JUNO_WEB.md` sections are historical implementation checkpoints.
- Generated builds, temporary scripts, packages, device backups, captures and logs belong under ignored `build/`. Do not commit personal device projects or dependency caches.

## Architecture, storage and resource limits

- Inspect the existing execution path and verified data structures before editing. Keep changes modular and avoid unrelated cleanup.
- Preserve persistent sizes whenever practical: `pattern_t` is 528 B and `sound_t` is 1,344 B. Prefer reserved bytes and explicit/idempotent migration over format expansion.
- Juno uses the retired BREAK pattern settings/slice bytes, marked by `0x4A/0x53`. Slice low bits hold degree; high bits hold tie, slide and octave flags. Existing Juno patterns must retain these flags on load. Unmarked legacy BREAK lines initialize as empty Juno lines.
- Performance options occupy the previously unused Juno sound bank 3, marked `0xB7`. Preset identity occupies bank 11's final two bytes, marked `0xA6`. Use the constants and migration functions in source; do not repurpose these bytes as spare storage.
- The latest verified hardware build (2026-11-08) is **391,484 B image**, **96,928 / 98,304 B static RAM**, **333,768 / 344,064 B pool**. These are a checkpoint, not substitutes for measuring a new build. Keep at least 8 KiB pool headroom; the build enforces it.
- Put immutable tables in const/flash. Use fixed-size state and float DSP, no audio-path allocation or linked soft-double routines. Do not add oversampling, band limiting or reduce voices without discussing the resource/sound tradeoff when it changes the requested result.
- Juno runs with the 808 on the second core. Preserve job handoff, render splitting and single-context mutation rules.
- Keep oscillator, envelope, LFO and control-update phase continuous across render chunks. New voices need immediate pitch/filter coefficients. Tie/slide continuations retain voice state; stale sequence note-offs must not kill a keyboard/arp voice that stole that slot.

## Build and focused checks

Run checked-in commands from the repository root; consult `BUILDING.md` for prerequisites.

- `./build.sh`: generate assets, compile hardware firmware/loader and package `build/x0x.fwsc`. Requires Python/Pillow, JieLi tools and AC79 SDK; use Docker on supported non-native hosts. `JIELI_TOOLCHAIN` and `AC79_SDK` override tool locations.
- `sh web/emu/build.sh`: build the Juno web emulator with Emscripten. `python -m http.server 8765 --directory build/emu` serves it locally.
- `sh tests/host/run_juno_performance.sh`: voice modes, arp/gate, timing, gain, SEQ, ties/slides, locks, storage and held-note/tie audio equivalence.
- `sh tests/host/run_juno_hardware.sh`: hardware-profile migration, presets, keyboard and save/reload.
- `sh tests/host/run_alpha_juno.sh`: oscillator/synth DSP checks.
- After a web build, use `node tests/host/juno_emu_test.mjs build/emu/x0x.wasm` and the `juno_performance_emu_test.mjs`, `juno_sequencer_emu_test.mjs` and `juno_slides_emu_test.mjs` runners for affected interactions.
- `tests/run_tests.sh`: broader host, reference, storage, update and scenario checks. External references/Csound are optional dependencies; state which comparisons were unavailable.
- `sh host/build_host.sh` and scripted `tests/scenarios/*.x0x` exercise the inherited host profile. Create scenario output folders before running it.

Run focused checks first, then broaden only for changes or failures that justify it. Rebuild hardware after firmware changes and report image/RAM/pool results. Rebuild shared WASM after browser-visible or sequencing changes. The user prefers compile-to-hardware iteration over repeating full browser UI tests for every small change; use focused native/WASM checks and inspect visuals when the UI itself changes. Documentation-only changes need link/format checks, not a firmware rebuild.

## Hardware iteration

- When flashing is authorized, complete the build/checks and proceed without repeatedly asking for the same permission. Verify the connected device and package; do not assume a previously connected FM-1 is still available.
- Before flashing, stop playback and capture a verified backup including unsaved edits. Preserve saved projects; never factory-reset or restore over user edits as a routine validation step.
- The package identity is `FM-1_987`; current editor INFO reports `X0X-HARDER JUNO DEV`. Ports normally contain `FM-1`, but numeric suffixes can change; discover them.
- The current backup CLI expects dated firmware identities whereas DEV advertises bare `FM-1_987`. Account for this known mismatch with an exact-device-checked adapter; do not weaken model checks globally or treat a rejected identity as proof the device is absent.
- Validate the packaged app against the compiled binary and target slot before installing. Keep a named package, checksum and pre-flash backup under `build/`.
- `python tools/fm1_install.py build/x0x.fwsc --port FM-1` installs the compiled package. After restart, verify runtime INFO and read back saved project data. Explain differences rather than claiming preservation from a successful flash alone.
- Do not erase fresh user edits during verification. Newly initialized defaults may remain only in RAM until SAVE/autosave. A backup taken while playing may exclude unsaved changes; check its flags.
- For audio reports, isolate other engines, mutes, automation and effects before attributing a pulse to an envelope/LFO reset. Reproduce from a backed-up project where useful.

## Development and review discipline

- Explain the relevant path and intended change before substantial implementation unless the user has already instructed direct implementation. Resolve routine choices from the session; ask only for missing decisions that materially affect the result.
- If behaviour or persistence contradicts a requested design, report the concrete conflict before choosing a different interpretation.
- Follow existing C99/Python style: four-space indentation, snake_case functions/files, uppercase constants and `-ffp-contract=off`. Preserve SPDX headers and component licences. Do not reformat legacy files wholesale.
- Add meaningful regression coverage for audio, sequencing, storage and UI behaviour. Inspect logs/captures under `build/`; do not claim checks passed merely because they exist in a runner.
- Report capacity problems before sacrificing existing functionality. Six voices working with the other engines is a validated listening checkpoint, not a universal worst-case CPU guarantee.
- Commits and PRs should describe the concrete resulting behaviour, relevant validation and any material limitation. Keep upstream credit and original image/source attribution intact. Verify the working tree and requested publish destination before handing off a push.
