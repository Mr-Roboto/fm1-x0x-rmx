# P8.1 documentation and credits review

Prepared for review on `orb-remix`, from source commit
`c5d828379a329d4b67dd4547d94429ee07e1c116`. This phase changes documentation only.
The initial P8.1 review performed no commit, push, deployment, firmware build or release.
The project owner subsequently authorized finalization and publication together with P7.4
browser device palettes; this report preserves the original review and records that clarification.

## Documents reviewed and changed

Reviewed `AGENTS.md` and the requested README, manual, version history, licensing record,
Remix specification, drum-generator record, waveform specification, Pages record and
Grids attribution. Also checked `BUILDING.md`, LCD schemes, the scale reference, the P6.4
capacity audit, Remix data provenance, 909 sample provenance and bundled font licenses.

Changed:

- [README](../README.md): five-part entry point, current public emulator, three 303 EDIT
  pages, browser/LCD palette distinction, corrected BREAK length, source link and shorter
  Morph explanation.
- [Manual](MANUAL.md) and generated `docs/manual.html`: browser startup/controls/storage,
  keyboard map, separate reset behaviors, full Remix workflow and APPLY semantics,
  Grids mode/pages/operations, EXTRA parameters, English scale names/intervals, saving
  lifetimes, Undo boundaries, exact Tune range and upstream hardware/recovery context.
- [Version history](NEW.md): current fork features grouped as unreleased development;
  original version numbers/dates/contributions retained and explicitly distinguished.
- [Licensing](../LICENSING.md), [909 samples](../assets/909/README.txt),
  [Grids](../assets/grids/README.md) and [Remix data](../assets/remix/README.md): fuller
  component lineage, consistent sample attribution and visible provenance questions.
- [Remix specification](REMIX_DESIGN.md): implemented work no longer listed as unresolved;
  Orb musical reverse/ping-pong still explicitly outside current implementation.
- [Drum-generator record](DRUM_GENERATOR_CORE.md), [waveform record](TB303_WAVEFORMS_DESIGN.md),
  [LCD schemes](COLOR_SCHEMES.md) and [Pages](PAGES.md): corrected current branch/pages,
  historical measurement labels, separate visual settings and completed P7.2/P7.3 status.

The main public-facing documents remain English. The original German scale notes and
waveform mathematics remain supplementary technical records; English operational details,
scale intervals and a waveform implementation summary are now available without them.
No equations, lookup values, source notices or historical release dates were changed.

## Implementation checks

Descriptions were checked against `firmware/src/app/ui.c` (page construction, input,
Grids dispatch, Undo and settings), `project.c` / `engine.c` (saving and accepted Wave
state), `seq/sequencer.c`, `pattern.h`, `motion.c`, `tb3po.c`, `drum_gen.c` and the 303
parameter descriptors. Browser controls, persistence, palettes and reset were checked
against `web/emu/index.html`, `worklet.js` and `x0x_web.c`.

Important distinctions now explicit:

- Remix mappings A1–P16 are separate from project patterns P1–P32; Forward source mapping
  can read hidden stored steps in partial blocks. APPLY is global/saved, outside Undo.
- Grids has three pages: MAP or EUCLID, FILL, OFFSET. CHAOS affects MAP; generator knobs
  remain session-only, while generated steps are saved and undoable.
- TB-3PO knob edits do not immediately rewrite notes; enabled automatic mutation can act
  later. Old scale indices and zeroed Morph bytes retain their documented compatibility caveats.
- BREAK is fixed at 16 steps / 1/16 rate. Swing affects 1/16 and 1/32, not triplet rates.
- Browser Factory reset deletes the saved project and reloads; GLO reset is an undoable
  project edit whose GLOBAL-setting changes are outside Undo. Break slots and visual
  preferences remain separate browser storage.
- Upstream dual-core timing figures and recovery version numbers are historical context,
  not validation of X0X-RMX or availability of a fork installer.

## Remaining attribution questions

The existing GPL, MIT, OFL and permission statements are retained, with their component
origins. This review does not add a new license grant.

1. The project owner **Mr-Roboto** has clarified that he recorded all 256 physical Future
   Retro Orb patterns into Cubase, analyzed their MIDI and manually compiled the mapping
   table used by Kontakt/Luminary and later X0X-RMX. This owner-supplied account establishes
   the documented creator/compiler credit; it does not independently verify underlying
   rights, imply ownership of Future Retro's original patterns/hardware intellectual
   property, or legally resolve all licensing questions. The local source notice is retained.
2. BB Gen is the pre-existing breakbeat component inherited from Charles Vestal's X0X,
   originally by mestela. The existing author-permission statement is retained. The original
   correspondence has not been supplied or independently verified here; no grant is removed,
   invented or reinterpreted.

Hortator is credited as a C-arithmetic reference; Emilie Gillet / Mutable Instruments
remain the original Grids authors. Existing references to Charles Vestal, Leo Kuroshita,
athousanddetails, Matthew Cieplak, Robin Schmidt, midilab, Dave Mollen, djphazer and
contributors, mystran/kunn, mestela and font authors are preserved or expanded.

## Validation

- `python3 tools/manual_html.py`: generated the current standalone HTML manual.
- `python3 tools/make_pages.py --preview build/p81-review-preview/fm1-x0x-rmx` and
  `python3 tests/pages_test.py build/p81-review-preview/fm1-x0x-rmx`: passed for all six
  sections, relative assets/links/anchors, project subpath, identity and release gate.
- A temporary documentation check passed 111 local Markdown links/anchors across 14
  reference documents and independently matched all 22 manual scale rows to `tb3po.c`.
- Read-only external checks: six public routes and 14 selected source/provenance links
  returned HTTP 200; `/firmware/` returned 404. The public installer contained no script,
  button or input. Results: `build/host/p81-external-links.json`. Other historical,
  mathematical, video and SDK links were not exhaustively fetched.
- Generated emulator assets match the existing verified `build/emu/` byte-for-byte.
  WASM SHA-256 remains `e3951651ecc4363b01f4575ac817ce994e0a26bb0240a00bd4204cc7ddb72dce`.
- `git diff --check`: passed. No firmware/DSP/UI/sequencer, web behavior, graphic, favicon,
  audio or publishing-tool source changes. No firmware or WASM rebuild was needed.

The first preview caught a LICENSE link that the package-free site could not resolve;
it now points explicitly to the source repository. External checks initially hit sandbox
DNS restrictions and passed when rerun with network access. No check failure remains.

## Recommended P8.2 visuals

The existing manual images are retained upstream illustrations. New fork-specific visuals
are recommended rather than implying those older screenshots show every current page:

- All four emulator finishes, with normal/lit/held controls and the Device color selector;
  a separate LCD SCHEME comparison to show the distinction.
- SEQ > REMIX on drums and a 303: BANK/PATTERN/ON/APPLY, an A1-to-variation example and
  a simple stored-source versus chronological-slot diagram, including a partial block.
- Grids MAP/EUCLID, FILL and OFFSET, plus NEW FIRST / NO CHANGE and Undo examples.
- TONE/VOICE/EXTRA for both 303s, all Morph landmarks and a rejected second Morph request.
- Current TB-3PO SCALE page, MPEN default, generated motif and restrained mutation.
- Browser storage/reset versus GLO reset, with clear SAVE and reload steps.

P7.2/P7.3 browser captures and existing host Grids/LCD captures under `build/host/` can guide
new captures. They were not added as publication images during P8.1.

## Suggested P8.3 video demonstrations

1. First five minutes: switch on the browser, make a kick/clap/hat beat, generate a 303
   line, shape TONE, add BREAK, mute and save. Introduce pointer/keyboard combinations.
2. Remix: distinguish project pattern from mapping, compare A1 and a variation, demonstrate
   APPLY boundaries, independent parts and mapped motion; leave BREAK playing unchanged.
3. Grids: compare MAP and EUCLID, adjust FILL/CHAOS/OFFSET, NEW/MUTATE, manual edits that
   cause NEW FIRST, and Undo/Redo. Explain session-only generator knobs.
4. 303 sound: walk through TONE/VOICE/EXTRA, compare Morph 0/64/127, transfer Morph ownership
   between A/B and demonstrate ACCDEC, DRIVE/DRVTYPE and TUNE.
5. TB-3PO and persistence: select scales, generate versus mutate, automatic mutation,
   save/reload, and distinguish browser reset, GLO reset and visual preferences.

Label browser demonstrations as emulator captures. Do not present them as physical FM-1
performance or an installable firmware release. No new images or videos were created here.

## Authorized finalization with P7.4

The owner-supplied Remix provenance above replaces the earlier missing-creator question.
It credits transcription/compilation without asserting Future Retro IP ownership or a
new license grant. BB Gen remains inherited from original X0X with its existing permission
statement; the correspondence has not been supplied or independently verified here.

P7.4 adds only two exterior palette entries and extends the browser regression. Cream &
Lilac and Classic Dark, LCD palettes, geometry, control mappings, favicon and audio behavior
remain intact. Finalization repeats documentation generation/link/Pages checks and
Chromium tests before the authorized commits, normal pushes and package-free deployment.

Final local validation passed: all four Chromium variants, exact Cream & Lilac/Classic Dark
state and geometry parity with P7.3, label/knob contrast, mouse/touch/keyboard and latch
operation, mobile/persistence, live audio (peak 0.318), both EXTRA pages, Grids/Remix and
independent LCD/exterior selection. No page errors occurred. Existing real-WASM Grids,
EXTRA, six LCD schemes and host audio/Remix parity checks passed. The rebuilt WASM is
byte-identical to P7.3 (the SHA-256 above); no firmware or memory-resource change is present.
