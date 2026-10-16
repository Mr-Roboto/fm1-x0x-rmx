# X0X-RMX demonstration video provenance

This is an **84-second browser-emulator recording**, not physical M-VAVE FM-1 footage.
The v1.0.0 release has separately passed real-device testing; see
[the confirmed hardware record](../../P9_FM1_TEST.md).

For release, only the obsolete outro validation line was replaced with:

> v1.0.0 Stable — tested on a real M-VAVE FM-1

The explicit “BROWSER EMULATOR DEMONSTRATION” label, project URL, credit and approved banner
remain. The text changes with the actual closing card at 82.64 seconds. The preceding video
keyframe is at 81.8 seconds, so only that final 2.2-second encoding group was re-encoded.
The first 2,045 decoded video frames are identical, all 2,100 frames retain the 25 fps timing,
and the complete AAC audio bitstream is unchanged. The final first/last outro frames were
visually reviewed. The rest of the production, original performance audio and poster remain.

| Asset | SHA-256 |
|---|---|
| Corrected public MP4 | `0bd9721ce02a5f842c1504e3f450dc09fe4f02a796d2866bb5e690296abe43e2` |
| Previous approved MP4 (historical) | `84adf956774d1678af3b0b8e3303841e968f01ca5533e98a210d1ce27765960c` |
| Unchanged poster | `e66e897da7736ccfd02d77e20094275324e56e3ce775ca0af2375e24411d2811` |
| Unchanged 1280 × 320 banner | `349b82e67d76fd3968249f9555c47a7fdc75f3ef3856f2b975a01f76ce85874d` |
| Unchanged AAC bitstream | `eaf7b293c58129dcf3a6afe089148df463f1765145e05b6a3017a0e68c4905ac` |

The final MP4 is 1280 × 720, H.264 High, 25 fps, with stereo 48 kHz AAC audio.
`tests/release_video_test.py` verifies the public file, timing and audio; supplying the original
MP4 additionally checks frame identity before the final encoding group. Local work files and
before/after frames are under `build/host/v1-release/video/`. Media changes do not alter or
rebuild the hardware-tested firmware package or the tested WASM binary.
