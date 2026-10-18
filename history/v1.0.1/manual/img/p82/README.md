# X0X-RMX publication screenshots

P9 refreshed `05-remix-909-off-a1.png`, `06-remix-909-on-b3.png` and
`19-remix-303a-on-c4.png` from the final local 1.0.0 emulator in Chromium. All UI state was
reached through ordinary panel inputs. They show APPLY/BANK/PATTERN/REMIX in the new order,
with 303 Remix on page 2; the 303 example leaves its underlying notes empty. The other
16 images retain their P8.2 provenance below. No displayed pixels were retouched.

These 19 PNGs are used by the README, manual and landing page. P8.2c recaptured
17 images from the real Chromium emulator; the already matching Global Settings
images `32-global-scheme-white-lilac.png` and `33-cream-white-lilac-global.png`
remain byte-identical to the P8.2a originals.

Matching publication pairings: **Cream & Lilac / WHITE LILAC**, **Classic Dark / DEFAULT**,
**Sunset Orange / SUNSET ORANGE**, **Pink Lilac / PINK LILAC**. Each LCD scheme is
selected through the actual GLO → SCHEME control, independently of the browser exterior.
No displayed UI pixels or parameters were recolored or retouched. The comparison
resizes four genuine captures and adds explanatory captions outside the device UI.

Source for the recapture: `orb-remix`, `d5d52e5620f3b65d0a897fb10679575cfdba54eb`;
unchanged emulator binary source: `75c0792919a37bf52d86f0fc4be4787a00c2c6f0`.
Chromium framing stays at 1280 × 1080 CSS pixels, device scale factor 3.
LCD crops are 627 × 627; exterior frames are 3120 × 2175; the comparison is 2140 × 1712.
The original generation and editing workflow is repeated, preserving pages, knob values
and framing. Drum sequences are reproducible. TB-3PO NEW uses time-based seeds and
original per-scene projects were not retained; the recaptures use fresh generated bass
motifs with the same settings. Their seed identifiers and note layouts consequently differ.

The approved [project banner](../x0x-rmx-banner-1280x320.png) is supplied artwork,
copied unchanged at 1280 × 320. SHA-256:
`349b82e67d76fd3968249f9555c47a7fdc75f3ef3856f2b975a01f76ce85874d`.
Existing project/component attribution remains in [LICENSING.md](../../../LICENSING.md).
Capture helpers, source originals and validation artifacts stay under `build/host/`.
The retained upstream illustrations outside this directory are distinct from these captures.

| Image | Contents | Publication use |
|---|---|---|
| [01-device-comparison.png](01-device-comparison.png) | Four exterior variants | README and website device comparison |
| [01-overview-cream.png](01-overview-cream.png) | Cream & Lilac device, generated 909 beat and device selector visible. | README hero; website hero |
| [05-remix-909-off-a1.png](05-remix-909-off-a1.png) | 909 REMIX at neutral A1/OFF with APPLY BEAT. | Manual |
| [06-remix-909-on-b3.png](06-remix-909-on-b3.png) | 909 REMIX B3/ON, APPLY BAR; BANK/PATTERN/REMIX/APPLY readable. | README and website feature gallery; manual |
| [08-grids-map-909.png](08-grids-map-909.png) | 909 MAP: X/Y/CHAOS/MODE, generated rhythm in the grid. | README and website feature gallery; manual |
| [09-grids-euclid-909.png](09-grids-euclid-909.png) | EUCLID mode: LEN K/S/H and MODE, stored result shown. | Manual |
| [10-grids-fill-909.png](10-grids-fill-909.png) | FILL K/S/H page at defaults 50/35/65%. | Manual |
| [12-grids-offset-909.png](12-grids-offset-909.png) | OFFSET K/S/H page at defaults 0/4/2. | Manual |
| [14-grids-result-909.png](14-grids-result-909.png) | Ordinary SEQUENCE view of the generated 909 beat. | Manual |
| [15-tb3po-generate-303a.png](15-tb3po-generate-303a.png) | 303A GENERATE: DENS/ACCENT/SLIDE/OCTS and generated motif. | README and website feature gallery; manual |
| [16-tb3po-scale-mpen.png](16-tb3po-scale-mpen.png) | 303A SCALE page: ROOT A, MPEN, OCTAVE and MUTATE. | Manual |
| [18-tb3po-result-303a.png](18-tb3po-result-303a.png) | 303A SEQUENCE view of the generated Dorian motif. | Manual |
| [19-remix-303a-on-c4.png](19-remix-303a-on-c4.png) | 303A REMIX C4/ON, APPLY LOOP; independent bass selection. | Manual |
| [20-303a-tone.png](20-303a-tone.png) | 303A TONE: CUTOFF/RESO/ENVMOD/DECAY with generated line. | Manual |
| [21-303a-voice-saw.png](21-303a-voice-saw.png) | 303A VOICE: ACCENT/WAVE/DRIVE/SLIDE, initially Saw. | Manual |
| [22-303a-extra-morph64.png](22-303a-extra-morph64.png) | 303A EXTRA: MORPH 64/ACCDEC/DRVTYPE/TUNE. | README and website feature gallery; manual |
| [23-morph-wave-303a.png](23-morph-wave-303a.png) | 303A VOICE with accepted WAVE Morph. | Manual |
| [32-global-scheme-white-lilac.png](32-global-scheme-white-lilac.png) | GLOBAL SCHEME row set to WHITE LILAC. | Manual |
| [33-cream-white-lilac-global.png](33-cream-white-lilac-global.png) | Cream exterior, WHITE LILAC LCD SCHEME and exterior selector in one frame. | Manual |
