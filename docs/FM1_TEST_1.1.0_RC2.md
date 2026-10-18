# X0X-RMX v1.1.0 RC2 — one FM-1 validation session

**Completed: maintainer-reported physical RC2 validation passed.** The maintainer
explicitly approved v1.1.0 Stable publication, including USB backup/restore,
on 18 October 2026. The session instructions below are retained for provenance;
results are user-reported hardware observations, not automated test results.
RC1 and its original pending record remain unchanged.

## Exact candidate

- Branch: `dev/v1.1.0-rc2`; firmware source: `77fc6692e6d875fe6970e85b4c057b43f4056515`.
- Displayed version **1.1.0**; USB/package identity **`FM-1_987010100`**.
- File: `build/v1.1.0-rc2/x0x-1.1.0-rc2-b343c135779c.fwsc` (**609,680 B**).
- SHA-256:

```text
b343c135779c1de933d655236b14628103816036e170d65faed706aba620dcaa
```

These are exactly the existing experimental USB-backup package's bytes.
The RC2 filename does not change its embedded version or identity. Retain this
read-only file for eventual publication, without rebuilding or repackaging.
See [Stable release notes and automated validation](RELEASE_1.1.0.md).

Windows Explorer can open the containing folder:

```text
\\wsl.localhost\Ubuntu\home\falke\fm1-x0x-rmx\build\v1.1.0-rc2
```

Alternatively run `explorer.exe .` from that folder in WSL, or use
`wslpath -w build/v1.1.0-rc2/x0x-1.1.0-rc2-b343c135779c.fwsc` from the repository
root to obtain the path for this WSL distribution.

## Local Chrome/Edge preview from WSL

The preview is prepared locally; nothing is deployed. In a WSL terminal run:

```sh
cd /home/falke/fm1-x0x-rmx
python3 -m http.server 8000 --bind 127.0.0.1 --directory build/v1.1.0-rc2/pages
```

Keep that terminal running and open **http://localhost:8000/backup/** in
Windows Chrome or Edge. If port 8000 is occupied, use 8001 in both the command
and URL. Connect the FM-1 by USB, close other MIDI clients, click **Back up**
when ready and allow MIDI/SysEx access. Connection is automatic when an
operation starts. The page accepts the X0X-RMX identity family.
The local preview's installer is intentionally disabled; use the exact RC2
file through your normal file-based installation procedure. The public
installer still offers v1.0.1 and must not be used to select this candidate.
Stop the preview with Ctrl+C when finished.

If the ignored local preview folder is missing, regenerate it from this branch
(after the emulator build has completed):

```sh
sh web/emu/build.sh
python3 tools/make_pages.py --preview build/v1.1.0-rc2/pages --experimental-backup
python3 tests/pages_test.py build/v1.1.0-rc2/pages
```

The generator requires a fresh output directory; an already prepared preview
does not need regenerating. These commands do not rebuild the firmware package.

## One short session

1. **Identity and preservation:** normally install the exact package above.
   Confirm **1.1.0** in GLO > ABOUT X0X, and **FM-1_987010100** using the
   read-only command `python3 tools/fm1_install.py --info` (close it before
   using the browser). Check a familiar project and uploaded
   user break. Before editing anything, note one BREAK Filter value and a few
   recognizable pattern/sound settings.
2. **Protect the original first:** with playback stopped, open the local
   page and click **Back up**. Keep its downloaded `.x0b` file unchanged. Select
   that file in the page's restore-file selector: **Ready to restore** means
   the container/profile, object sizes and CRC checks passed. This selection
   alone does not send a restore. Optional file-only verification:
   `python3 tools/x0x_backup.py info /path/to/downloaded-file.x0b`
   (Windows Downloads is under `/mnt/c/Users/YOUR_WINDOWS_USER/Downloads/`).
   Full decoded-content validation also runs on the FM-1 before confirmation.
3. **HOME cancellation:** while still stopped and before edits, press Restore,
   accept the browser question, then press **HOME** at **RESTORE FROM USB?**
   on the FM-1. Expect Not restored and unchanged settings/patterns. Keep the
   original backup selected for the later successful restore.
4. **Processor baseline:** GLO > PERFORMANCE must show **2 CORES**. SEL >
   RUN PERF TEST takes about 15 seconds and restores the project afterwards.
   Photograph or record all three mean/peak pairs below and any dropouts.
   **NO CYCLE COUNTER is expected; no cycles/sample measurement is needed.**
5. **Filter, reset and cymbals:** play the same familiar BREAK pattern at the
   same tempo. Try Filter **64/OFF**, left for LP and right for HP; observe
   live GLO > PERFORMANCE load/peak and 2 CORES after each change. Listen for
   expected tone changes and dropouts. Hold ENV and turn Filter's knob to
   reset it to **64/OFF**. Briefly listen to 909 hi-hat, ride and crash.
6. **Restore the saved original:** stop. Make one obvious, reversible Filter
   change from the value noted in step 1 and save it. Select the original
   verified backup, press Restore and accept the browser question; at the
   FM-1 confirmation press **SEL**. Expect successful reload and the original
   Filter/pattern/sound values, including prior Remix/MREV/Morph settings where
   present. Check the user break still plays. Undo history intentionally resets.
7. **Playback and USB coexistence:** play the restored project and click **Back up**
   again, keeping the first backup separately. Listen for dropouts and check
   live performance. This exercises MIDI/SysEx while playing; the file contains
   saved data only. Also record a short USB-audio clip in your usual computer
   recorder while backup runs and check it plays cleanly. No external MIDI
   hardware is needed. Ordinary host MIDI notes/clock may additionally be
   checked with existing software; record unavailable checks as not performed.
8. **Persistence:** stop, save the restored project, restart normally and
   confirm its settings, patterns and uploaded user break remain. Retain both
   backup files and the candidate checksum with your results.

| Built-in workload | v1.0.0 mean | v1.0.0 peak | RC2 mean | RC2 peak | Mean / peak difference (percentage points) |
| --- | ---: | ---: | --- | --- | --- |
| FACTORY LOOP | 41.1% | 49% | 41.5% | 50% | +0.4 / +1 |
| ALL FIVE | 56.6% | 69% | 56.8% | 70% | +0.2 / +1 |
| WORST CASE | 70.7% | 90% | 71.0% | 89% | +0.3 / −1 |

These are recorded v1.0.0 baselines, not v1.1.0 results. Subtract baseline
from RC2: 42.1% versus 41.1% is **+1.0 percentage point**, not +1.0% relative
CPU usage. Record actual observations; no new numerical pass threshold is
introduced. The same-pattern OFF/LP/HP comparison is a separate workload.

| Observation | Maintainer-reported result |
| --- | --- |
| Installation and operation | Successful |
| Two CPU cores / built-in PERFORMANCE benchmark | Operational / completed; values above |
| BREAK FILTER | Successful; no numerical live-filter measurements supplied |
| ENV + KNOB reset / 909 cymbal playback | Successful |
| USB project backup and restore | Successful |
| HOME cancellation and other checklist items | Reported successful |
| Existing project / user samples / persistence | Successful |
| Audio glitches or regressions | None reported |

The built-in benchmark differences are small. The maintainer's general checklist
confirmation does not supply specific USB timings, independent Chrome-versus-Edge
measurements, optional external MIDI details or physical power-cut results; none
are claimed. Interrupted commit recovery remains an automated simulation result.
The exact package checksum is independently verified before and after publication.

Only this successfully tested binary is approved for Stable publication, without
rebuilding or repackaging. The local preview instructions above describe RC2
preparation; the approved public backup page is now `/backup/`, with the public
installer updated to the same tested v1.1.0 bytes.
