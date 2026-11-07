# X0X-HARDER hardware checkpoint

Validated on 2026-11-07. This checkpoint follows the RMX/upstream integration and
captures the locally developed features installed on the connected FM-1:

- X0X-HARDER branding.
- Independent 909/808 CH/OH velocity, step locks and recorded motion.
- 909 open-hat HOLD, including choking, retriggering and velocity behavior.
- Stock-X0X revision 6 migration distinguished from HARDER revision 6.
- Independent 303 HPFs and 303B pattern mirroring with signed transpose.
- Fourth 303 MIX page: HPF, PAN, REV and DLY, with locks and recorded throws.
- Third HOME screen, Live: ACID1/2, FEEDBACK and drum/break-only DJ filtering.
- Live transpose keys (-7..+8 semitones, key 8 = unison), immediate/bar toggle,
  steady active key and flashing queued key, with internal/external clock tests.

Live controls are session offsets. They do not overwrite saved sound values,
patterns or automation. Persistent sound/pattern sizes remain unchanged; sound
revision 8 initializes the added 303 controls safely in older projects.

## Hardware artifact

The installed development package is generated locally as
`build/x0x-harder-dev-live.fwsc`; generated tools, firmware and personal device
backups are excluded from Git by the existing `build/` rule.

| Check | Result |
|---|---|
| Product / USB identity | X0X-HARDER DEV / FM-1_987 |
| Application image | 489,352 bytes |
| Static RAM | 97,632 / 98,304 bytes |
| Pool | 334,936 / 344,064 bytes |
| Package | 609,682 bytes |
| Package SHA-256 | `5f60ad33c6d11265bdf7456e0e644b6f49f85c4bd50a581da90db1386a58e042` |

The packaged application was decoded and compared byte-for-byte with the built
image. Installation completed, the FM-1 returned with its expected identity and
product name, and pre/post-install backups contained identical saved project
objects. Hardware operation has also been exercised by the user, including dual
303 playback. This is a development checkpoint, not a published stable release.

## Verification

- Full available host suite: 86 checks, including 40 scenarios, passed.
- Nine browser/WASM checks passed, including audio parity with the host.
- Original DSP golden renders and 264 cymbal reference cases passed.
- Focused Live tests cover held-note retuning, four-beat quantization independent
  of pattern lengths/rates, 96-tick external bars, mirrored B, MIDI note-zero and
  clipping/release behavior, macro layering, mute/LED behavior and unchanged saves.
- Both drum filter directions leave 303-only audio bit-identical. Tests separately
  verify that 909, 808 and BREAK signals are affected.
- Target image/RAM/pool, RAM-code, single-precision and HAL checks passed.
- The update-loader simulation passed in Linux, including preservation of project,
  settings and sample areas.
- External 909/808/Open303 source-reference comparisons were unavailable because
  their separate reference checkouts were absent.

## Core allocation

The main core renders the 909 and both 303s, runs sequencing and command handling,
mixes all parts, applies the Live drum filter, processes effects and the master,
and runs the foreground UI. The second core renders the 808 and BREAK in parallel.
The main core then waits for that job before finishing the mix.

The second core therefore has fewer assigned tasks, but actual headroom depends
on the playing project. No independent live per-core utilization measurement was
made for this checkpoint. In two-core operation the existing 808 performance stage
counts the main core's wait for the second core, rather than the second core's
complete render duration; it should not be interpreted as its utilization.
