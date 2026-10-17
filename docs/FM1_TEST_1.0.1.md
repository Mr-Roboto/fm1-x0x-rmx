# X0X-RMX v1.0.1 — hardware validation record

Report received from the maintainer on **18 October 2026**. Device: a real
**M-VAVE FM-1**. Installation and testing succeeded. This records reported
hardware observations, not an additional test performed by the coding agent.
The maintainer explicitly approved **v1.0.1 Stable public publication** on
18 October 2026, accepting the unperformed checks below. No additional hardware
tests or recovery experiments are required for this authorized release.

## Stable release artifact

- Implementation commit: `45310f6c5a20eae1951c034783e75dca7a1fd55d`.
- Development branch: `dev/independent-firmware-identity`.
- Existing local package: `build/x0x-1.0.1.fwsc`, 609,680 B.
- Package and running firmware identity: **`FM-1_987010001`**.
- Firmware SHA-256:

```text
42190c2dffbda6d89807e40b68dd8d2426ef502fa3848dc47bb1af37e34a2fd3
```

The successfully hardware-tested firmware was not rebuilt, replaced or modified
while recording these results. Only documentation is changed in this follow-up.

## Maintainer-reported nine-item checklist

| Item | Check | Result |
| --- | --- | --- |
| 1 | Independent firmware identity `FM-1_987010001` | Passed |
| 2 | Five playback directions including MREV | Passed |
| 3 | Correct Musical Reverse traversal | Passed |
| 4 | Remix bypass and restoration | Passed |
| 5 | `MORPH IN USE` notification through the knob | Passed |
| 6 | `MORPH IN USE` notification through SEL | Passed |
| 7 | Save/restart persistence | Passed |
| 8 | Existing project data and user samples | Passed |
| 9 | External MIDI clock and dedicated extended performance test | **Not performed** — external MIDI equipment was not connected |

The FM-1 also ran stably without audible dropouts or unexpected interruptions.
No run duration was supplied. This observation does **not** establish a pass
for item 9's dedicated extended performance test.

**Interrupted-update recovery on physical hardware was not tested.**
Successful regular installation does not establish recovery after an interrupted
update. The existing automated loader/OTA simulations remain separate evidence.
A physical Firmware Hub installation was not reported either.

Do not infer more detailed outcomes than those supplied: the report does not
separately specify every pattern length, timing measurement, connection/OS,
or performance workload. Automated length/timing coverage is recorded in the
[release notes](RELEASE_1.0.1.md), separately from hardware observations.

## Existing artifact verification — no rebuild

The following read-only checks were completed on 18 October 2026:

1. The development branch points to implementation commit
   `45310f6c5a20eae1951c034783e75dca7a1fd55d` before this documentation update.
   Firmware, build-tool and asset inputs have no source differences from it.
2. The package SHA-256 equals the value already recorded in that commit's
   `docs/RELEASE_1.0.1.md`. The existing build log reports the same package name,
   identity, app size and resource figures as the committed build record.
3. `python3 tests/identity_test.py build/x0x-1.0.1.fwsc build/x0x.bin` passes:
   package markers decode to `FM-1_987010001`, the app contains that exact
   null-terminated USB identity, the loader marker is present, and the decrypted
   packaged app equals the existing 580,284-byte `build/x0x.bin`.
4. The app SHA-256 is
   `0626260a688d3ae381382891aeeb8ba8e8cc336b1b029f98393b67b780d3e678`.
   The existing ELF has a five-entry direction-name table, the MREV-capable
   sequencer and the user-wave rejection counter. Its RAM and pool symbols
   agree with the committed resource report.

This verifies the existing artifact against the committed checksum/build record
and checks its internal package/app consistency. No fresh source-to-binary
reproducibility build or device-flash readback is claimed. The successfully
tested bytes remain unchanged. If a firmware rebuild becomes necessary, record
its new checksum and require new hardware validation.

## Compatibility and release boundary

The accepted direction change remains explicit: FWD=0, REV=1, MREV=2, PING=3,
RND=4. Without migration, v1.0.0 PING (2) becomes MREV and RND (3) becomes PING.
The [release notes](RELEASE_1.0.1.md) retain the full compatibility explanation.
Data/sample preservation does not imply unchanged interpretation of those bytes.

The maintainer explicitly authorized the v1.0.1 Stable GitHub release/tag, the
unchanged hardware-tested package, Pages, the online manual and Web MIDI
installer. The unperformed checks are accepted release limitations, not passes.
The publication-day registry recheck found no competing `FM-1_987` package
identity among 57 listed firmwares; X0X-RMX family recognition remains present.

`main`, the v1.0.0 release/tag and its historical hardware records remain
unchanged. The original v1.0.0 package still hashes to
`eccacfa995efec6b9c819583e7bce43b3aa7a126033c8e668ae89c6a988b754c`.
