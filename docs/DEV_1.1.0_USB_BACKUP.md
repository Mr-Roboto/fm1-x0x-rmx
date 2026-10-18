# Experimental X0X-RMX v1.1.0 USB project backup

This is an experimental integration on `dev/v1.1.0-usb-backup`, based on RC1
`1e78336dc7d1948a47ea95b1df0032aff70528ea`. It is **not an approved release
feature** and is not merged into the release-candidate branch. No device is
flashed and no release, tag, installer or Pages deployment is published.

RC1's branch, validation records and read-only package remain unchanged:
`build/v1.1.0-rc1/x0x-1.1.0-rc1-1124707f702e.fwsc`, SHA-256
`1124707f702e57ba97ec0e89d2fdde6751b2efcc2af8250cef69a292c1cee366`.
This integration produces a different experimental binary. RC1 hardware
results cannot validate it.

## Selective port and compatibility review

Charles Vestal's GPL-3.0-only implementations are selectively adapted from:

- Firmware `53cdab9034059e8323d557cc72ad067eee906d0c`.
- CLI `04194ba33ef179923b5ce27c5c3638f618e4d70e`.
- Browser `c0cd54157c3c154bc9bec51e7c7fed5197e1b9ab`.
- Site wiring `726f727c974fc1df667df2f89b09e4cb9964f694`.
- Manual `9a3dce4f94fdce959c1afefafa4090f49cb2c448`.

There is no upstream merge and no accessibility, voltage, core-failover or
other unrelated feature port. Existing firmware identity generation is unchanged.
The clients accept only complete numeric RMX identities `FM-1_987` followed
by six ASCII digits. RC1/v1.0.1 do not implement this protocol even though
individual identities pass the family check; they time out without a transfer.

The six objects are settings, sound and the four packed pattern/song/motion
objects (IDs 0, 1, 2, 3, 5, 6). Their payloads, including format words, are
backed up exactly as saved. Persistent structures and serialization are unchanged.

| RMX data | Backup behavior |
| --- | --- |
| Remix banks, selections and enable flags | Pattern reserved bytes retained exactly |
| Remix APPLY and manual startup patterns | Settings bytes retained exactly |
| MREV and all direction numbers | Retained, no direction migration |
| 303 Wave and three Morph shapes | Whole sound retained; existing ownership validation on load |
| TB3PO | Pattern generator configuration/seeds and written notes retained |
| Grids | Written patterns retained; session-only generator controls are not persistent and are not backed up |
| BREAK FILTER | Sound revision 5 retained; earlier supported sound revisions use normal migrations, including OFF |
| Song and motion | Entire existing packed stream retained |
| Uploaded user loops | Not included, not erased; loop references remain in patterns |
| Panel calibration | Not included, not erased; belongs to the physical unit |
| Undo history, unsaved live edits during playback | Not included |

The historical v1.0.0 direction-number caveat remains: old PING=2 becomes
MREV, old RND=3 becomes PING. This feature adds no further reinterpretation.

## Explicit backup-file policy

The container keeps `X0XB`, six ordered records, and upstream framing/CRCs.
Its **RMX-specific profile/version word is `0x524D`**, not upstream version 1.
This is a schema discriminator, not a digital signature or an automatic
cross-firmware converter. Standard X0X, Polly and other variants are refused,
even when object IDs and all CRCs match. Renaming a file does not convert it.
Do not relabel another firmware's file with the RMX marker.

Only complete RMX format-2 packed projects (current raw size 29,572 B), with
supported settings/sound layouts and sound revisions 0–5, are accepted; an
entirely absent, never-saved project represents defaults. Incomplete projects,
format-1 legacy backup streams, unknown revisions/layouts, unknown flags,
invalid sizes/IDs, malformed LZ, and inconsistent CRCs are rejected. Format-1
projects can still load normally through existing project migration; save them
on this firmware before making a restorable backup. Revision-4 compatibility
uses the actual v1.0.1 layout. Future schema changes must explicitly review or
revise this profile rather than assuming compatibility.

The firmware checks header, each payload, total image and decoded raw-project
CRCs, plus format/magic, lengths and LZ distances/output bounds, before asking
or committing. The streaming validator reuses existing `io_buf` and `lz_head`:
512 B of token checkpoints and a 3,584 B decoded ring, with bounded iterative
lookback for longer distances. No second project buffer, heap allocation or
recursive decoder is introduced. A two-million-read work bound safely rejects
pathological streams; unusual valid streams exceeding it are also refused,
leaving the active project intact. Such rejection is a compatibility limitation,
not permission to bypass validation.

## Transfer, storage map and recovery

Stopped backup first attempts to save dirty edits. During playback it streams
saved data only and flags omitted unsaved changes. Saves wait while a backup
pins object copies. One 128-byte data chunk per main-loop pass produces at most
161 wire bytes / 54 USB-MIDI packets, fitting the existing 64-packet SysEx ring.
Timeout/unplug frees the save hold. OTA cancels an active backup before entry.
Normal MIDI, USB audio, sample upload and firmware update handlers remain.

Restore refuses playback, stages in 512-byte reads and programs at most 256
bytes per write. Starting playback during transfer aborts before commit. The
FM-1 requires a fresh **SEL** press; **HOME**, playback or 60 seconds cancels.
The prior project and unsaved working edits remain on refusal/cancellation.
After confirmation, commit admission atomically rechecks stopped state and masks
only ALNK0 rendering until reload completes. USB IRQs continue, MIDI queues,
and no new per-block DSP check is introduced. This closes the MIDI START race
between the last playback check and activation. Successful restore loads the project through the normal migrations, updates
sound, I/O/startup patterns and loops, and resets recording/Undo to the new base. Armed song recording is cleared
before queued MIDI can run on the restored project.

| Flash region | Use |
| --- | --- |
| `0x97000..0x9EFFF` | A/B sound, PAT0, PAT1 and song objects |
| `0xA0000..0xDBFFF` | User sample slots; untouched |
| `0xDC000..0xDDFFF` | Panel calibration; untouched |
| `0xDE000..0xDFFFF` | A/B motion objects |
| `0xE0000..0xE4FFF` | Existing OTA staging area |
| `0xE1000..0xE1FFF` | RMX restore journal, sharing inactive OTA staging |
| `0xE4F00` | OTA resume record, distinct from restore journal |
| `0xFC000..0xFDFFF` | A/B settings objects |

Staging writes spare copies without storage headers, so it cannot activate
any object. After confirmation a verified `XMR1` journal lists all six new
headers. Every staged payload is verified again before headers are written,
including during boot recovery. Recovery runs **before** project loading and
OTA cleanup. Every journal erase is read back; an uncertain journal write must be safely
cleared or require recovery. Journal/header or final journal-erase failure stops normal
operation with RESTORE NEEDS RESTART: saves, another restore and OTA cannot
invalidate pending recovery. Restart retries the journal; persistent flash
failure remains an error rather than claiming a successful restore. No new
safe-mode shortcut or deliberate physical power-cut test is introduced.

A necessary upstream dependency is included: a zero-length committed object
is a tombstone and loads as absent without falling back to an older copy.
Initial upstream tests exposed its omission; adding the behavior makes the
original absent-object and all-write power-cut assertions pass unchanged.
Saved-object hash caches are invalidated specifically after restore so later saves cannot
skip writing objects based on stale pre-restore hashes. The first complete run
found that upstream's unconditional invalidation on ordinary loads caused three
extra autosave writes. Invalidation is now restore-only; the original autosave
scenario and its exact write-count assertions are preserved.

## Build, capacity and audio implications

Build: `./build.sh --release 1.1.0`. Both USB and package retain
`FM-1_987010100`; the version remains 1.1.0. This is development, not Stable.
The initial port passed the intermediate gate at 474,876 B Flash, 97,336 B
static RAM and 335,332 B pool, before client/site work.

| Resource | RC1 | Final experimental build | Additional | Free after |
| --- | ---: | ---: | ---: | ---: |
| Application Flash (581,564 B) | 471,244 B | 476,940 B | +5,696 B | 104,624 B |
| Static RAM (98,304 B) | 97,336 B | 97,336 B | 0 B | 968 B |
| Pool (344,064 B) | 334,360 B | 335,332 B | +972 B | 8,732 B |

The existing >=8 KiB pool gate passes by 540 B; no gate or assertion is relaxed.
Static changes fit existing linker/compiler padding. New persistent buffers are
only the bounded transfer buffers/state in the pool, not project-sized buffers.
Target disassembly shows a 456-byte `fm1_main` local frame with inlined transfer
and validator work, against the existing 24,320-byte user-stack region. The
separate `rs_heads` helper has 96 B locals plus saved registers. This is a
source/disassembly review, not a physical stack high-water measurement.
The 512-byte restore buffer is pool-resident, not on the stack.

All twelve DSP/engine/sequencer/motion object files are byte-identical to the
preserved matching baseline (whose inputs remain unchanged through RC1).
There is no new render/ISR path or audio DSP workload. Idle cost is a small
main-loop service check; backup adds main-loop flash reads/CRCs and USB traffic.
Flash reads retain existing <=256-byte IRQ-off windows. Restore/validation is
stopped-only; flash erases already silence the DMA buffer. No FM-1 CPU-percent,
USB-audio coexistence or live-backup dropout result is claimed without hardware.

Experimental package (609,680 B):
`build/v1.1.0-usb-backup/x0x-1.1.0-usb-backup-b343c135779c.fwsc`.
SHA-256: `b343c135779c1de933d655236b14628103816036e170d65faed706aba620dcaa`.
Logs, identity verification, object comparisons and SHA256SUMS are alongside it.

## Clients and local preview

```sh
python3 tools/x0x_backup.py backup project.x0b
python3 tools/x0x_backup.py info project.x0b
python3 tools/x0x_backup.py restore project.x0b
# --yes skips the CLI question only; the FM-1 still requires SEL.
python3 tools/make_pages.py --preview build/v1.1.0-usb-backup/pages --experimental-backup
python3 -m http.server 8000 --bind 127.0.0.1 --directory build/v1.1.0-usb-backup/pages
```

Open `http://localhost:8000/backup/` in Chrome/Edge. The page uses `backup/` with Web MIDI/SysEx, download a file, or select one
for restore. Stop first to include dirty edits. Close other MIDI clients.
Keep original uploaded-loop WAVs separately. Restoring a project on another
unit does not transfer loops or calibration. A project's loop references only
sound the same if that unit has the corresponding loops in its slots.

The site generator includes the page/link only with the explicit experimental
flag. The route/link is absent by default; the flag is restricted to local previews.
The installer source, emulator implementation and published site remain unchanged. No stock-firmware installer changes from upstream are imported.

## Validation and recommendation

Targeted firmware, actual-project, CLI and browser protocol tests pass. They
cover saved RMX bytes, revision-4 OFF migration, nested CRC/LZ rejection,
playback refusal/mid-transfer start, every-read disconnect, HOME, bad images,
strict identity/profile rejection, sample/calibration sentinels, save holding,
post-restore save/Undo, and simulated cuts at all 54 flash writes (47 old,
7 new after recovery). Added runtime header/cleanup failures require recovery
and cannot reload mixed data. CLI/browser conversations match byte-for-byte.
The final complete suite passes **82 groups, 0 failures, 2 skips**. Only the
unavailable external 808/Open303 reference comparisons are skipped; their
local tests pass. Installer/update preservation, identity, emulator, all
scenarios (including unchanged autosave assertions), µ-law, BREAK FILTER,
ENV reset, Remix, MREV, TB3PO, Grids and Morph regressions pass. The local
experimental Pages preview passes relative links/anchors, project-subpath,
assets, WASM provenance and release-gate checks. Focused AddressSanitizer and
UndefinedBehaviorSanitizer checks pass without suppressions. Real Chrome/Edge
USB operation remains a physical validation item, not a claimed pass.

**Technically feasible; keep experimental until the maintainer decides.**
Recommend retaining RC1 for v1.1.0 unless the extra USB feature and its new
binary receive a short normal-device validation. Estimate 10–15 minutes:
backup stopped and while playing (listen for glitches), HOME cancellation,
SEL round-trip of a recognisable project, save/restart, original user loops,
and an ordinary MIDI/USB-audio check. No power-cut experiment is requested.
Actual Chrome/Edge and CLI USB latency, coexistence and stack high-water remain
unmeasured on FM-1. Automated recovery is strong evidence, not hardware proof.
Do not merge this branch into RC1 or publish it without the maintainer's decision.
