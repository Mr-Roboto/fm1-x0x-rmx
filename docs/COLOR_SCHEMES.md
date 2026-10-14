# P5 LCD color schemes

GLOBAL has one SCHEME selector with six stable IDs. The browser uses the same LCD renderer. P5 did not recolor the device SVG; P7.3 now
provides separate Cream & Lilac / Classic Dark device palettes (see [Pages](PAGES.md)).
P7.4 adds Sunset Orange and Pink Lilac exterior variants inspired by these LCD palettes.
GLO > SCHEME affects the LCD, not the device illustration; neither selector changes the other. All schemes use a black background. Cream refers to the foreground highlights of WHITE LILAC and WHITE BLUE. Red recording and warning indicators retain their original colors.

The tuples below are the exact RGB inputs to `RGB()` in `firmware/src/gfx.c`. The macro truncates these to RGB565. The packed part words are also listed to identify the exact stored colors.

## Factory default and saved-project compatibility (P8.4)

New projects, fresh startup without a valid saved project, and factory reset select
**WHITE LILAC (ID 1)**. The firmware startup splash and UI default descriptor use the
same scheme. This does not couple the browser exterior selector to GLO > SCHEME.

Valid saved selections retain all six existing IDs (0–5). **DEFAULT remains ID 0**
and preserves the original appearance. Historical untagged bytes, invalid scheme
IDs/tags, and short settings records that omit the scheme byte decode as DEFAULT,
with their saved legacy palette. Loading does not replace these with WHITE LILAC.
The tagged-byte encoding, project/storage layouts and format versions are unchanged.

Regression coverage includes `sh tests/host/run_scheme_ui.sh`,
`node tests/host/scheme_emu_test.mjs build/emu/x0x.wasm`, and
`node tests/host/factory_scheme_browser_test.mjs SITE_ROOT OUTPUT_DIR` (Chromium,
with the existing `X0X_PLAYWRIGHT` / `X0X_CHROMIUM` runtime overrides where needed).
The last check exercises real browser startup, SAVE/reload, both reset controls,
independent exterior storage and saved DEFAULT/untagged/invalid/short project fixtures.

## Part colors

| ID / Scheme | 909 | 808 | 303A | 303B | BREAK |
|---|---|---|---|---|---|
| 0 DEFAULT | (255, 150, 40) | (255, 72, 64) | (130, 240, 90) | (70, 205, 255) | (214, 120, 255) |
| 1 WHITE LILAC | (187, 143, 180) | (232, 152, 192) | (184, 160, 240) | (160, 184, 248) | (216, 128, 240) |
| 2 PINK LILAC | (248, 176, 152) | (248, 112, 152) | (192, 160, 248) | (224, 144, 208) | (248, 208, 224) |
| 3 SUNSET ORANGE | (248, 192, 88) | (232, 120, 96) | (248, 216, 168) | (208, 152, 104) | (224, 144, 168) |
| 4 GRAPHITE MINT | (152, 240, 192) | (80, 192, 168) | (184, 216, 136) | (96, 208, 232) | (168, 184, 216) |
| 5 WHITE BLUE | (240, 224, 184) | (152, 176, 208) | (120, 192, 224) | (104, 144, 200) | (192, 200, 224) |

| Scheme | 909 RGB565 | 808 RGB565 | 303A RGB565 | 303B RGB565 | BREAK RGB565 |
|---|---|---|---|---|---|
| DEFAULT | 0xFCA5 | 0xFA48 | 0x878B | 0x467F | 0xD3DF |
| WHITE LILAC | 0xBC76 | 0xECD8 | 0xBD1E | 0xA5DF | 0xDC1E |
| PINK LILAC | 0xFD93 | 0xFB93 | 0xC51F | 0xE49A | 0xFE9C |
| SUNSET ORANGE | 0xFE0B | 0xEBCC | 0xFED5 | 0xD4CD | 0xE495 |
| GRAPHITE MINT | 0x9F98 | 0x5615 | 0xBED1 | 0x669D | 0xADDB |
| WHITE BLUE | 0xF717 | 0x9D9A | 0x7E1C | 0x6C99 | 0xC65C |

## LCD colors

| Scheme | Rules | Inactive | Labels | Secondary | Values | Accent |
|---|---|---|---|---|---|---|
| DEFAULT | (34, 34, 40) | (74, 74, 82) | (150, 150, 158) | (232, 136, 30) | (255, 184, 70) | (255, 255, 255) |
| WHITE LILAC | (40, 36, 48) | (88, 80, 100) | (184, 176, 200) | (208, 184, 232) | (248, 232, 208) | (248, 244, 232) |
| PINK LILAC | (48, 32, 40) | (100, 76, 92) | (200, 168, 188) | (224, 160, 208) | (248, 208, 224) | (248, 240, 240) |
| SUNSET ORANGE | (48, 36, 32) | (100, 80, 68) | (200, 176, 152) | (232, 152, 96) | (248, 212, 152) | (248, 240, 216) |
| GRAPHITE MINT | (32, 44, 44) | (72, 96, 96) | (168, 192, 184) | (96, 200, 184) | (192, 240, 216) | (232, 248, 240) |
| WHITE BLUE | (32, 40, 48) | (76, 88, 104) | (176, 188, 200) | (136, 176, 216) | (240, 232, 208) | (248, 244, 232) |

DEFAULT uses the saved legacy palette for its five LCD colors; the row above shows the factory AMBER palette. GREEN, CYAN, RED and MONO remain unchanged:

| Legacy palette | Secondary | Values |
|---|---|---|
| GREEN | (80, 220, 120) | (150, 255, 175) |
| AMBER | (232, 136, 30) | (255, 184, 70) |
| CYAN | (60, 180, 235) | (150, 225, 255) |
| RED | (232, 76, 60) | (255, 132, 112) |
| MONO | (196, 196, 202) | (236, 236, 240) |

All legacy palettes retain rules (34, 34, 40), inactive (74, 74, 82), and labels (150, 150, 158). DEFAULT retains white accent (255, 255, 255).

## Recording and warning colors

| Role | RGB input |
|---|---|
| Song recording text | (255, 90, 80) |
| Recording dot | (255, 50, 50) |
| Recording play icon | (255, 80, 70) |
| Warning / confirmation | (255, 80, 60) |
| Active COMP text / gain reduction meter | (255, 60, 60) |
| CPU warning | (255, 60, 60) |
| Crash background | (160, 0, 0) |

These are named semantic colors in the central renderer and stay fixed across schemes. Inactive COMP uses the original label color. DEFAULT changes only active COMP text and the GR meter fill.

## Persistence and compatibility

`settings_t.palette` is unchanged. The formerly reserved brightness byte at offset 17 holds `0xD0` through `0xD5`. Historical brightness values 0–8, other untagged values and unsupported IDs decode as DEFAULT. The settings object remains 28 bytes and the project remains 30,944 bytes, with unchanged field offsets and format. Factory reset restores DEFAULT and AMBER. Startup/safe-mode screens use DEFAULT before project loading.

## Footer refinement

Ordinary instructional footers use a dedicated `C_FOOT` role. For the five new schemes only, it adds 16 to each dim RGB channel in RGB565; inactive cells, units and other UI elements retain `C_DIM`. DEFAULT returns the original `C_DIM` unchanged. The already brighter startup hint and TB-3PO secondary/stale prompts retain their existing emphasis.

| Scheme | Footer RGB input | Solid contrast against black, before → after |
|---|---|---|
| WHITE LILAC | (104, 96, 116) | 2.79:1 → 3.59:1 |
| PINK LILAC | (116, 92, 108) | 2.74:1 → 3.51:1 |
| SUNSET ORANGE | (116, 96, 84) | 2.78:1 → 3.57:1 |
| GRAPHITE MINT | (88, 112, 112) | 3.19:1 → 4.06:1 |
| WHITE BLUE | (92, 104, 120) | 2.93:1 → 3.76:1 |

WHITE LILAC 303B shifts from (144, 192, 240) to (160, 184, 248), a cooler blue-violet distinct from 303A's (184, 160, 240). Their quantized solid contrasts against black are 11.07:1 and 9.67:1 respectively. P5.2 also changes WHITE LILAC 909 to (187, 143, 180); every other part color remains unchanged.

## Historical P5 measured capacity

Initial P5 implementation compared with the current local uncommitted P4 baseline, compiled with the same firmware toolchain and flags:

| Metric | P4 | P5 | Change |
|---|---:|---:|---:|
| Application image | 575,424 B | 576,092 B | +668 B |
| Linked function code | 178,532 B | 178,962 B | +430 B |
| Unity-object read-only data | 36,463 B | 36,678 B | +215 B |
| Linked initialized data | 360 B | 376 B | +16 B |
| Remaining image alignment | — | — | +7 B |
| Static RAM (.data + .bss region) | 97,244 B | 97,244 B | 0 B |
| Pool | 334,348 B | 334,348 B | 0 B |

The six const records occupy 144 B in flash, and their name pointer table occupies 24 B. The active scheme pointer is 4 B of initialized RAM; the linker absorbs initialized-data growth in existing alignment space before .bss. App flash headroom is 5,472 B of the 581,564 B slot. Static RAM headroom is 1,060 B. Pool headroom is 9,716 B. Package size is 609,680 B; browser WASM is 661,028 B.

The final refinements build to an application image of **576,268 B** (+176 B from initial P5), static RAM of **97,244 / 98,304 B**, and pool usage of **334,348 / 344,064 B**. RAM and pool usage are unchanged. Final app flash headroom is 5,296 B; package size remains 609,680 B. Final browser WASM is 661,058 B. No persistent structure or scheme ID/name changed.

## Readability and validation

The host LCD contact sheet covers all six schemes and all five parts; the GLOBAL sheet checks complete selector names in the existing list. Parts differ in hue and/or brightness. A numerical check of the quantized solid part colors against black gives:

| Scheme | Minimum part contrast | Label contrast |
|---|---:|---:|
| DEFAULT | 6.28:1 | 7.09:1 |
| WHITE LILAC | 7.69:1 | 10.43:1 |
| PINK LILAC | 8.11:1 | 10.1:1 |
| SUNSET ORANGE | 7.59:1 | 10.47:1 |
| GRAPHITE MINT | 9.7:1 | 11.25:1 |
| WHITE BLUE | 6.61:1 | 11.24:1 |

These ratios describe solid colors, not a hardware-panel calibration or a claim about every antialiased/inactive glyph. Screenshots were visually inspected for legibility and part distinction.

- `sh tests/host/run_scheme_ui.sh`: 200 original P4 LCD scene hashes, 30 scheme/legacy-palette round trips, 45 historical brightness/palette projects, short settings, invalid tags, real GLOBAL input, SAVE, confirmed reset, incremental redraws and 54 LCD screenshots.
- `node tests/host/scheme_emu_test.mjs build/emu/x0x.wasm`: real browser input for all six choices, host part-color parity, saved reboot, factory reset, old short settings and 36 LCD screenshots.
- `./build.sh` and `sh web/emu/build.sh`: passed firmware checks and browser build.
- `X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L AC79_SDK=/home/falke/fw-AC79_AIoT_SDK sh tests/run_tests.sh`: all available UI, sequencing, persistence, update, host/browser parity and simulator checks passed. The runner exits nonzero because the external 9W9, 8W8 and schwung-303 reference repositories are unavailable.
- `node web/test_breaks.mjs`: passed on an authorized retry outside the sandbox; the initial run was blocked by `spawnSync python3 EPERM`.

Generated screenshots and machine-readable colors are in `build/host/schemes/`. The baseline was captured in `/tmp/p5-six-baseline` before P5 edits. No musical or sequencing source was changed by P5.

Final refinement validation rebuilt firmware and the browser emulator, regenerated 54 host and 36 browser LCD screenshots plus contact sheets, and reran the scheme UI/persistence, Remix state/UI/motion, TB-3PO, bass303 UI and storage regressions. All 200 original DEFAULT hashes still match. A before/after pixel comparison confirms all DEFAULT screenshots are unchanged and changes outside the footer occur only on WHITE LILAC screens displaying 303B. `build/host/schemes/refinement-detail.png` shows the final 303A/303B colors and before/after footer crops; `refinement-pixel-diff.json` records changed bounds.

## P5.2 color and GLOBAL navigation refinements

WHITE LILAC 909 now uses muted mauve (187, 143, 180), packed as 0xBC76. Its quantized solid contrast against black is 7.69:1, keeping black text on the part chip readable. All other part and LCD colors remain unchanged.

Active COMP and the GR meter fill share `C_GAIN`, RGB(255, 60, 60), in all six schemes including DEFAULT. Inactive COMP, meter backgrounds, recording and other warning colors retain their previous colors. The 200 original DEFAULT scene hashes remain untouched; five active-compressor references at 12 dB were added. Before/after pixel comparisons for all five legacy palettes confirm changes only inside COMP text and GR fill bounds (`build/host/schemes/p52-pixel-diff.json`).

GLOBAL stores selection and scroll in two session-only UI bytes before changing views, and restores a valid, visible cursor when reopened. SCHEME survives HOME/GLOBAL exits, other views and their menus. Action screens and confirmation cancellation also preserve the position. UI initialization resets this memory; other lists still open at the top. Saved structures and scheme IDs are unchanged.

P5.2 firmware: application flash **576,348 / 581,564 B** (5,216 B free), static RAM **97,244 / 98,304 B** (1,060 B free), pool **334,348 / 344,064 B** (9,716 B free). RAM/pool usage is unchanged. Loader binary: 6,524 B; package: 609,680 B; browser WASM: 660,847 B.

Validation passed: scheme UI/persistence tests, inactive/threshold/active/clamped COMP and GR pixels in all six schemes, GLOBAL cursor and scroll tests across views/menus/actions, 200 original DEFAULT hashes, five active DEFAULT references, and browser selection/storage/color parity/navigation checks. Regenerated 71 host and 42 browser screenshots, plus part, GLOBAL and compressor contact sheets. Firmware and browser builds passed. The full suite passed all available UI, Remix, TB-3PO, sequencing, storage, update and simulator checks; external 9W9, 8W8 and schwung-303 comparisons were unavailable. The browser sample-conversion check passed on an authorized retry after sandbox `spawnSync python3 EPERM`.

## P8.4 measured capacity

The WHITE LILAC factory-default build uses an application image of **579,820 B**
in the **581,564 B** application slot (**1,744 B free**). Static RAM is
**97,324 / 98,304 B** (**980 B free**); pool usage is **334,348 / 344,064 B**
(**9,716 B free**). The development package is **609,680 B**, identity `FM-1_900`;
the browser WASM is **669,773 B**. These are local build results, not a hardware
release or physical validation. Firmware installation/downloads remain disabled.
