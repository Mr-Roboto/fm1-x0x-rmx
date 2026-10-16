# X0X-RMX v1.0.0 — hardware test record and checklist

The maintainer explicitly approved publication of v1.0.0 Stable on 16 October 2026.
The tested package is `build/x0x-1.0.0.fwsc`, product `FM-1_9010000`, SHA-256:

```text
eccacfa995efec6b9c819583e7bce43b3aa7a126033c8e668ae89c6a988b754c
```

## Reported results

| Check | Maintainer-reported outcome |
|---|---|
| Regular firmware update from FM-1_900 | Successful |
| Existing patterns and user breaks after update | Preserved |
| Five manually chosen startup patterns after restart | Restored correctly |
| Pattern-chain playback | Successful |
| Twelve-minute musical run | No reported problems |
| USB audio with iPad and AudioShare | Successful; described as working perfectly |
| Sequencer, drums, bass synths, effects and breakbeat engine on the real FM1 | Successful |
| Separate FULL and FOLLOWS KNOB hardware tests | Not separately reported; not a release condition |

Do not infer unreported individual test outcomes from the musical run. Specific device/OS
versions, exhaustive Morph timing measurements, and separate results for every checklist
item below were not provided. The list is retained for reproducible future checks, not as
an additional gate on the explicitly approved release. Automated results are recorded in
[P9_REPORT.md](P9_REPORT.md); public notes are in [RELEASE_1.0.0.md](RELEASE_1.0.0.md).

## Compact reproducible checklist

1. **Back up and record.** Back up the project and user samples. Record the current identity,
   five manual patterns, USB AUDIO choice and all three user slots before updating. Do not
   reset before checking retention of the existing settings and samples.
2. **Regular update.** Check the old identity with `python3 tools/fm1_install.py --info`.
   Install the checksum-verified package through the web installer or
   `python3 tools/fm1_install.py build/x0x-1.0.0.fwsc`. Keep USB connected while writing.
   Verify product `FM-1_9010000` and display `X0X 1.0.0` afterwards.
3. **Basic operation and audio.** Check display, parts, encoders, buttons, headphones/line/
   speaker and local volume. Play a dense musical project for at least twelve minutes;
   watch for crashes, audible dropouts and obvious control errors. Record PERF observations.
4. **iPad/AudioShare.** Connect the FM1 and record USB audio in AudioShare. For separate mode
   checks, verify that FULL stays independent of the hardware volume knob and FOLLOWS KNOB
   follows it; save/restart and check retention. These two separate checks were not reported
   for this release and are not an additional approval condition.
5. **Factory Reset.** Only after backups and retention checks, reset and confirm USB AUDIO
   defaults to FULL. Factory Reset replaces project sounds and patterns. Do not confuse it
   with an ordinary firmware update.
6. **Manual startup combination.** Use PRESETS on individual part screens, for example
   909=P2, 808=P5, 303A=P9, 303B=P13, BREAK=P17. SAVE, switch off/on and compare all five.
   Select a HOME pattern key, save and restart: all five should follow that manual choice.
7. **AUTOSAVE and SAVE while playing.** Choose another combination, stop and wait for
   AUTOSAVED (four seconds idle, with up to a twenty-second gap after the previous autosave).
   Restart and check. During PLAY choose new patterns and press SAVE: SAVES WHEN STOPPED;
   stop, wait for SAVED, restart and check the latest manual combination.
8. **Song and chains.** Save a manual reference combination, then play a song with different
   patterns, stop/save/restart and check the manual reference. Define a HOME chain: its first
   manual choice becomes the startup combination. Automatic advances must not replace it.
9. **Older projects.** Load an old project without startup fields: all parts start at P1;
   sounds, notes and stored Remix/APPLY remain. Save a new combination and restart. Do not
   assume lossless downgrade of new settings to older firmware.
10. **Built-in breaks and samples.** Listen to 909 GR and 808 GR at 96 BPM for several loops:
    two bars, distinct grooves and no disturbing seam/distortion. Check other tempos/slices.
    Load recognizable samples into all three user slots and play them. Under Windows, test
    the `FM-1 Midi 2` uploader if available. Repeat a regular firmware update and verify the
    samples remain intact. Explicit uploader erase/replace operations intentionally write slots.
11. **303 and Remix controls.** On both 303s verify SEQUENCE → REMIX → KEYS, with only FREE
    PLAY on KEYS. On 909/808/303A/303B verify APPLY/BANK/PATTERN/REMIX left to right, saved
    values and STEP/BEAT/BAR/LOOP timing. A1 is neutral; BREAK has no Remix.
12. **Record outcomes.** Record exact package hash, date, device/iPad/OS and observed results.
    Distinguish reported passes, untested cases and actual failures. Stop a future release
    if a failure is found rather than silently substituting an untested firmware build.
