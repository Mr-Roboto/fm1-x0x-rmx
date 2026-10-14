# X0X-RMX licensing and component credits

X0X-RMX is an independent fork of [Charles Vestal's X0X](https://github.com/charlesvestal/fm1-x0x).
It retains the upstream platform, instrument ports and attribution notices.

X0X-RMX is free software under the GNU General Public License, version 3 only
(`GPL-3.0-only`, full text in `LICENSE`). It is a fork of Felucca and keeps Felucca's
licence. If you distribute X0X, or firmware derived from it, you must give your
recipients its complete corresponding source under the same licence. Component-specific notices below
remain applicable; the project licence does not replace third-party attribution.

## Where the code comes from

| What | Origin | Licence |
| --- | --- | --- |
| Platform: `firmware/hal/`, `firmware/loader/`, `firmware/src/{libc,lcd,gfx,usb,storage,ota}.c`, `tools/` (build, package, install, upload), `web/` (installer) | [Felucca](https://github.com/hugelton/Felucca), Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments; `usb.c` and `storage.c` changed for X0X | GPL-3.0-only |
| `firmware/src/app/{slots,editor,panel,main_fm1}.c` | adapted from Felucca's `eng_sample.c`, `editor.c`, `panel.c`, `main.c` / `audio.c` | GPL-3.0-only |
| 909 (`dsp/drum909*`, `dsp/fxbus*`) | ported from [9W9](https://github.com/athousanddetails/schwung-9W9) (athousanddetails), itself after [ER-99](https://github.com/matthewcieplak/er-99) (Matthew Cieplak) | GPL-3.0 |
| 909 hi-hat, ride and crash samples (`assets/909/`) | ER-99 via 9W9 | GPL-3.0 |
| 808 (`dsp/drum808*`) | ported from [8W8](https://github.com/athousanddetails/schwung-8W8) (athousanddetails); circuit models after the TR-808 service notes and Werner / Abel / Smith; rim shot after sc808 (Yoshinosuke Horiuchi / Sam Aaron) | GPL-3.0 |
| 303 (`dsp/bass303*`) | ported from [schwung-303](https://github.com/charlesvestal/schwung-303) by Charles Vestal: [Open303](https://github.com/RobinSchmidt/Open303) by Robin Schmidt (MIT), Devilfish extensions after [jc303](https://github.com/midilab/jc303) (midilab), RAT drive after [dm-Rat](https://github.com/davemollen/dm-Rat) (Dave Mollen); TeeBee-filter coefficients credit mystran & kunn | GPL-3.0 (Open303 parts MIT) |
| TB-3PO (`seq/tb3po*`) | ported from [schwung-tb3po](https://github.com/charlesvestal/schwung-tb3po), by Charles Vestal, itself a port of the Phazerville Hemisphere Suite `TB_3PO` applet (djphazer and contributors) | GPL-3.0 |
| Offline drum generator MAP (`seq/drum_gen.c`, generated MAP data, `assets/grids/resources.cc`) | Mutable Instruments Grids by Emilie Gillet; C arithmetic cross-checked against DEADACTIVE's Hortator port; pinned source and adaptation details in `assets/grids/README.md` | GPL-3.0-or-later, combined here under GPLv3 |
| Break generator (`dsp/breaks*`) | ported from [schwung-breakbeat](https://github.com/mestela/schwung-breakbeat) (BB Gen) by mestela, **used with the author's permission** (that repository carries no licence file) | GPL-3.0-only here, by permission |
| The idea of a browser emulator for FM-1 firmware (`web/emu/`) | [Groove OS](https://www.groove-os.com/) by Peter Gombos, whose site runs its firmware in the browser; no code from it | credit only |
| Remix lookup data (`assets/remix/remixes.txt`, generated nibble table) | Table transcribed, analyzed and manually compiled by project owner Mr-Roboto from MIDI recordings of all 256 physical Future Retro Orb patterns; historical Kontakt/Luminary source; import details and digest in [assets/remix/README.md](assets/remix/README.md) | Existing local source notice: GPL-3.0-only; provenance clarified by owner, without resolving underlying rights (below) |
| Everything else in `firmware/src/{app,dsp,seq}`, `host/`, `tests/` | Charles Vestal's X0X and X0X-RMX modifications, retaining existing notices | GPL-3.0-only |

No recorded breaks are in this tree or in the default firmware: the built-in break loops are
generated at build time by X0X's own 909 code. `tools/import_breaks.py`,
`tools/gen_break_bank.py` (`X0X_BREAK_BANK`) and `tools/upload_breaks.py` can put recordings
of your choosing into a build or onto a device; those recordings keep their own copyright.

## Felucca Assets

Felucca's non-GPL assets — the icon atlas, the panel photo and the Hügelton drum pack —
are **not** part of this fork and are not used by it. Felucca's section 7 additional
permission concerns those assets only, so it is not needed here.

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| Barlow Semi Condensed (The Barlow Project Authors), the default firmware UI face; Barlow and Barlow Semi Condensed for web typography | SIL OFL 1.1 | `assets/fonts/BarlowSemiCondensed-*.ttf`, `assets/fonts/Barlow-OFL.txt` |
| Terminus (Dimitar Toshkov Zhekov), the `terminus` font set | SIL OFL 1.1 | `assets/fonts/ter-u*.bdf`, `assets/fonts/Terminus-LICENSE.txt` |
| Fukiai icon font (Hügelton Instruments), web pages only | MIT | `web/fukiai.ttf`, `web/FUKIAI-LICENSE.txt` |
| JieLi AC79 SDK: `uboot.boot`, `cfg_tool.bin`, `eq_cfg_hw.bin` are read from your SDK checkout at build time and placed in the package; no SDK files are in this tree | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |

## Provenance records to confirm

The notices above preserve the repository's existing license statements; they do not
create a new permission grant.

- The project owner **Mr-Roboto** states that he personally recorded all 256 patterns
  from a physical Future Retro Orb into Cubase, analyzed the MIDI sequences and manually
  compiled the complete step-mapping table. It became the historical Kontakt/Luminary
  source used here. Credit for his transcription/compilation is separate from Future
  Retro's original patterns and hardware. This clarification does not imply ownership
  of Future Retro's underlying intellectual property or legally resolve all licensing
  questions; the existing local GPL-3.0-only notice is retained.
- BB Gen's permission is recorded in the existing X0X notices and source header. The
  permission correspondence is not packaged here; retain that record with the project.
  Its repository's lack of a license file is not a substitute for the recorded permission.

No third-party code or sample has been added during documentation finalization.

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments. "M-VAVE" and
"FM-1" are trademarks of their respective owners. TR-808, TR-909 and TB-303 are
trademarks of Roland Corporation, used only to describe what the parts emulate. X0X-RMX is
independent firmware; it is not affiliated with, endorsed by or supported by any of them.

Future Retro Orb is a musical reference for Remix. No affiliation with or endorsement by
Future Retro is implied, and this project does not claim to include Orb firmware.

## Radio

X0X never enables the Bluetooth / Wi-Fi radio of the hardware.
