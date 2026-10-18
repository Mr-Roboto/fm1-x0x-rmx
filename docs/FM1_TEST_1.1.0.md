# X0X-RMX v1.1.0 RC1 — short FM-1 validation record

**Pending: no v1.1.0 physical results have been reported.** One normal
installation and the checks below are sufficient for this candidate's planned
hardware review. Do not deliberately interrupt an update. No cycle-counter
measurement or external MIDI equipment is required.

## Exact candidate

- Version: **1.1.0**; USB/package identity: **`FM-1_987010100`**.
- File: `build/v1.1.0-rc1/x0x-1.1.0-rc1-1124707f702e.fwsc` (609,680 B).
- SHA-256:

```text
1124707f702e57ba97ec0e89d2fdde6751b2efcc2af8250cef69a292c1cee366
```

Keep this exact file. Record the checksum with the hardware results; eventual
release must use these tested bytes without another build. Full provenance and
automated checks are in the [draft release notes](RELEASE_1.1.0.md).

## One short session

1. After normal installation of the exact candidate, confirm **1.1.0** in
   GLO > ABOUT X0X and **FM-1_987010100** in the installer's read-only
   device query (`python3 tools/fm1_install.py --info` on the connected host).
2. Open **GLO > PERFORMANCE**, confirm **2 CORES**, then press SEL and choose
   **RUN PERF TEST**. Let the approximately 15-second test finish; photograph
   its three results and note dropouts/guard activity. The test restores the
   project. Record mean and peak CPU percentages below. **NO CYCLE COUNTER is
   expected on this device; ignore the unavailable cycles/sample figures.**
3. Play one familiar BREAK pattern. In BREAK > EDIT > SOUND try Filter at
   **64/OFF**, left for **LP**, then right for **HP**, keeping the same pattern,
   tempo and other settings. Listen for the expected tonal change and return
   to 64 for bypass. Check live **GLO > PERFORMANCE** after each setting; note
   load/peak, dropouts and whether **2 CORES** remains displayed.
4. Briefly check **ENV + KNOB** reset (for example BREAK Filter returns to
   64/OFF) and listen to the 909 hi-hat, ride and crash in normal playback.
5. Confirm a familiar saved project and uploaded user break still work, then
   save/restart once to confirm persistence. Report any unexpected change.

| Built-in workload | v1.0.0 mean | v1.0.0 peak | RC1 mean | RC1 peak | Mean / peak difference (percentage points) |
| --- | ---: | ---: | --- | --- | --- |
| FACTORY LOOP | 41.1% | 49% | Pending | Pending | Pending |
| ALL FIVE | 56.6% | 69% | Pending | Pending | Pending |
| WORST CASE | 70.7% | 90% | Pending | Pending | Pending |

These are the maintainer-supplied v1.0.0 baseline figures, not v1.1.0 results.
Subtract baseline from candidate for each difference: 42.1% versus 41.1% is
**+1.0 percentage point**, not +1.0% relative CPU usage. Record observations
rather than inventing a new numerical pass threshold. The built-in test and
the same-pattern filter comparison are separate workloads.

| Observation | Result |
| --- | --- |
| Installed version / identity / package checksum | Pending |
| 2 CORES during built-in test and BREAK playback | Pending |
| BREAK OFF / LP / HP: sound, live load/peak, dropouts | Pending |
| ENV reset and basic 909 cymbal audio | Pending |
| Existing project/user samples and save/restart | Pending |

Send the completed values or photos and any audible/visible problems. No
v1.1.0 publication is authorized by completion of this document alone.
