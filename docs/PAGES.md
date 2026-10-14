# X0X-RMX website, local previews and deployment records

The browser-only website is live at [X0X-RMX](https://mr-roboto.github.io/fm1-x0x-rmx/),
with the [emulator](https://mr-roboto.github.io/fm1-x0x-rmx/emu/) and documentation.
Firmware installation and downloads remain disabled. Source:
[Mr-Roboto/fm1-x0x-rmx](https://github.com/Mr-Roboto/fm1-x0x-rmx), development branch `orb-remix`.
Keep `main` aligned with [original X0X](https://github.com/charlesvestal/fm1-x0x).

## Current public state

The public emulator includes Remix, Grids MAP/EUCLID, 22 TB-3PO scales, Morph and the
TONE/VOICE/EXTRA pages for both 303 voices. Its Device color selector defaults to Cream &
Lilac and also offers Classic Dark, Sunset Orange and Pink Lilac; this visual preference is independent of GLO > SCHEME.
Firmware installation and public downloads remain unavailable.

P7.3 source `c5d828379a329d4b67dd4547d94429ee07e1c116` was pushed to `orb-remix` and
site commit `ed341db8a52741da490debac6c392303f6e0551b` to `gh-pages`, both without force.
The [P7.3 deployment](https://github.com/Mr-Roboto/fm1-x0x-rmx/actions/runs/38526717747)
succeeded. Public Chromium checks verified both palettes, audio during switching,
both EXTRA pages, storage/mobile/favicon behavior and the disabled installer/download routes.
The public WASM matched P7.2 byte-for-byte. `main` was unchanged.

Documentation-only previews can reuse an existing verified `build/emu/` without compiling
firmware or WASM. Its `build-info.json` continues to identify the binary's source revision;
it does not claim that uncommitted documentation has been published. Choose a fresh output
and run the generator and Pages test below. Publication requires separate authorization.

## Build and preview without a release

From the repository root, with Emscripten and the normal asset build prerequisites installed:

```
sh web/emu/build.sh
python3 tools/make_pages.py --preview build/preview/fm1-x0x-rmx
python3 tests/pages_test.py build/preview/fm1-x0x-rmx
python3 -m http.server 8000 --directory build/preview --bind 127.0.0.1
```

Open `http://localhost:8000/fm1-x0x-rmx/`. This tests the project subpath, with relative links
and assets. Use a fresh output directory on each generation. The generator accepts only
subdirectories of `build/` and refuses to replace nonempty output, including stale firmware.
The output must be separate from `build/emu`, which is its emulator input.
`tools/publish_pages.sh build/OUTPUT_DIR` is an alternative local build/preparation wrapper;
it performs no Git mutations, network writes or deployment.

The emulator compiles current working-tree sources, including Remix, Grids and both 303 EXTRA
pages, with the existing `web/emu/build.sh`; no prebuilt upstream binary is downloaded. Its browser interface,
AudioWorklet, controls, keyboard mapping, LCD and device geometry are preserved; P7.3 adds
selectable device palettes and a project favicon.
The emulator build records the source revision, working-tree changes and WASM SHA-256 in
`emu/build-info.json`. An uncommitted preview correctly reports that its sources are dirty.
This metadata documents provenance, not hardware validation.

## Generated structure

- `index.html`: X0X-RMX landing page, credits and release status.
- `emu/`: `index.html`, `worklet.js`, `x0x.wasm`, shared project `favicon.svg`, `build-info.json`.
- `manual/index.html`, `manual/img/`: current manual and images.
- `new/index.html`, `new/img/new/`: fork development notes and labelled original X0X history.
- `breaks/index.html`: existing Web MIDI sample uploader; hardware connection requires Chrome/Edge.
- `install/index.html`: reserved status page, without scripts, package links or installation controls.
- `.nojekyll`: static Pages output.

Preview mode contains no `firmware/` directory or `.fwsc` file. Additional technical Markdown
links resolve to the fork source on `orb-remix`; manual and version history links resolve to
rendered site pages. Historical upstream links are retained and labelled as upstream history.

## Review before any publication

Run the static site test and emulator tests. In a browser, switch on, press PLAY, confirm sound
and LCD/LED updates, use keyboard and mouse, and select 909/808 MAP and EUCLID pages. Review
manual navigation, credit links, break page and reserved installer. Browser output does not
prove physical FM-1 timing, sound or installation compatibility.

## GitHub settings and later deployment

The initial P7.1 preparation did not deploy. The user subsequently authorized a browser-only
public deployment, committing P7.1 to `orb-remix` and creating/updating `gh-pages`.
Firmware publication remains excluded. The old publisher recreated a Git repository and force-pushed an orphan
`gh-pages` branch, discarding remote site history. That behavior has been removed.

Recommended later procedure: review generated package-free output, then use a normal
fast-forward commit on `gh-pages` in a separate checkout. `orb-remix` remains the development
branch and the original workspace never switches branches. Do this only after publication
approval; it can immediately update an existing public Pages site.

1. Review repository Settings → Pages and existing `gh-pages` history first. Before this
   authorized deployment, GitHub reported `has_pages: false`, and the remote had no
   `gh-pages` branch. The available credentials have repository admin access.
2. After approval, regenerate into a fresh directory and run the tests above. Record the source
   revision and WASM hash. If a remote `gh-pages` branch exists, clone that branch into a separate
   temporary directory using `git clone --single-branch --branch gh-pages
   https://github.com/Mr-Roboto/fm1-x0x-rmx.git CHECKOUT`. This retains all existing site history.
3. If `gh-pages` does not exist, use a separate normal clone and create an orphan `gh-pages`
   branch there. Remove tracked development files from that separate index/checkout only.
   Never do this in the development workspace, and never replace an existing remote branch
   with an orphan history.
4. In the separate checkout, replace generated site files with the reviewed output while
   preserving `.git`. Inspect `git diff --stat`, staged changes and absence of firmware files.
   Commit only the site output, then `git push origin gh-pages` without force. If rejected,
   fetch the new remote history and reapply/review the site update; do not force-push.
5. Under Settings → Pages → Build and deployment, choose **Deploy from a branch**, branch
   **gh-pages**, folder **/(root)**, then Save. A repository admin/maintainer must perform this
   setting. The source branch must already exist. Enable HTTPS if the setting is available;
   no custom domain or CNAME is needed.
6. Check the Pages deployment in Actions, then verify the expected URL and all six sections,
   including sound and MAP/EUCLID. Only then describe the public website as live.

GitHub's official [publishing source instructions](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site)
confirm the branch/root setup. If automation is preferred later, select **GitHub Actions** as
source and use checkout/build, `upload-pages-artifact`, and `deploy-pages`, with `pages: write`
and `id-token: write` on the deployment job. Configure the `github-pages` environment to permit
`orb-remix` and require review, and build exactly the reviewed revision. Start with manual
activation after approval; do not add automatic deployment to upstream-aligned `main`.
No active deployment workflow is added in P7.1.

## Deferred hardware installation

Real FM-1 timing/listening/update validation and explicit approval are required before any
release package, download, public installer or release version. The future
`--validated-release PKG VERSION OUT_DIR` generator mode retains package identity and loader
checks; it is not a validation test and must not be used to bypass the release gate.
Installer libraries, package names, protocol and hardware identifiers are retained.

P7.1 changed page identity and text only. P7.2 added EXTRA EDIT for both 303 voices;
P7.3 added the device palettes and favicon. Their completed deployments are recorded below.

## Initial P7.1 preparation verification record

Prepared from `orb-remix` at `44c37e7caf1d20081e2865440a17cf854a022d6c` with uncommitted
website/documentation changes. Final preview: `build/preview-final/fm1-x0x-rmx/`.

- Emscripten 3.1.69 build passed; WASM size 669,669 bytes.
- WASM SHA-256: `616f98599225eb2fc3e45b16ce99068e0e05ffacd459bbd8640c75434a1404bd`.
- Host audio comparison passed: 168,021 samples, peak 0.261, maximum difference
  `3.05e-5` (16-bit rounding), LCD drawn and key lights active.
- Grids emulator test passed: MAP/EUCLID, 909/808 pages, host renderer parity, session
  isolation, OFFSET mutation, NEW FIRST, Undo behavior and restart defaults.
- Color scheme emulator regression passed, including 42 LCD screenshot checks.
- Six generated pages passed relative asset/link/anchor and project-subpath checks;
  preview release gate, provenance, preserved favicon and AudioWorklet checks passed.
- Local HTTP returned 200 for all six sections, WASM, worklet and favicon beneath
  `/fm1-x0x-rmx/`.
- Break uploader byte comparison passed against the Python uploader after retrying outside
  the sandbox, which blocked its Python child process.
- Generator refused existing output, destinations outside `build/` and overlap with
  `build/emu`, without changing the existing preview.
- Python compilation, shell syntax and `git diff --check` passed.

During the initial preparation, no browser executable or browser automation tool was available. Therefore full HTML/CSS
visual rendering, browser AudioWorklet startup, audible playback and live mouse/keyboard
interaction remain manual preview checks; the successful tests execute the real WASM directly
and compare its LCD/audio output. External links were inspected for correct destinations,
not exhaustively checked for remote availability. GitHub repository Pages settings and public
deployment were not verified or changed.

Firmware sources and build configuration were not modified, and no firmware build/package was
created. The existing P6.4 capacity figures remain the recorded baseline (579,788-byte image,
1,776 bytes free Flash, 97,324 bytes static RAM, 334,348-byte pool); these are not a new P7.1
hardware build measurement. Firmware branding, device illustration and favicon are untouched.
No branch switch, commit, push, release or public deployment was performed during the initial
preparation. The subsequent authorized publication is recorded below.

## Authorized public deployment verification

The user explicitly authorized public browser-only publication, commits and normal pushes.
P7.1 source preparation was committed as `e618f22` and pushed to `origin/orb-remix`.
The first site commit was `c30e956` on a newly created `gh-pages` branch; no remote site history
existed to replace. Subsequent site updates are normal fast-forward commits on that history.
The workspace remains on `orb-remix`; `main` remains at `47f8e4c4ad602591b81c21bc5480db7a94f6d6af`.

GitHub Pages is configured as **Deploy from a branch → gh-pages → /(root)** with HTTPS enforced.
The initial [Pages deployment](https://github.com/Mr-Roboto/fm1-x0x-rmx/actions/runs/38474050384)
succeeded. No manual settings action remains. Future publication still requires authorization;
`tools/publish_pages.sh` remains a local preparation wrapper. The branch/root procedure above
is the procedure used for this deployment.

Verified public routes: `/`, `/emu/`, `/manual/`, `/new/`, `/breaks/`, `/install/`.
The public WASM matched the locally built WASM byte-for-byte and passed the real Grids and host
audio comparison tests after downloading. Public `emu/build-info.json` records the exact
committed source revision and an empty working-tree change list. The WASM SHA-256 remains
`616f98599225eb2fc3e45b16ce99068e0e05ffacd459bbd8640c75434a1404bd`.

A temporary Playwright/Chromium installation enabled real-browser checks against the public
HTTPS site. The checks passed:

- All six pages rendered with X0X-RMX titles; documentation images loaded.
- AudioWorklet boot completed; sound output measured a peak of **0.402**, with AudioContext
  running and lights active. This measured browser audio output, not physical speaker listening.
- Mouse buttons and wheel controls reached 909 MAP, EUCLID, FILL and OFFSET and 808 MAP pages;
  the LCD header and controls/footer pixels matched host references.
- NEW generated ordinary drum steps; keyboard HOME, pattern selection and PLAY worked.
- No JavaScript page errors or failed site resource responses occurred.
- `/install/` contains no script, input or button; `/firmware/` returns HTTP 404.

Browser screenshots and results are in `build/host/p71-browser/`; build/test artifacts are
local and are not added to the public site. The final documentation update replaces planned
website links with verified public links and preserves local preview navigation.
No installable package, firmware release, firmware source change, force-push or change to
`main` was made. At the time of that P7.1 deployment, hardware validation and later visual work were deferred.

## P7.3 device palettes and favicon

P7.3 starts after P7.2's `990543c` source revision was finalized and deployed successfully
(Pages commit `562505c`). The source workspace was clean and matched `origin/orb-remix`.
P7.2 provides EXTRA EDIT for both 303 voices; earlier references above to planned P7.2
visual work describe the original P7.1 plan. Device branding is implemented in P7.3.

The emulator's **Device color** selector defaults to **Cream & Lilac**, a visual interpretation
rather than a photograph-matched finish. **Classic Dark** preserves the original SVG colors,
including illuminated, dim, held and latched states. Palettes are defined in one configuration
in `web/emu/index.html`, applied through CSS variables without rebuilding the SVG or controls.
The display surround stays dark. The separate localStorage key `x0x-emu-device-color` remembers
only the visual choice; unavailable storage and unknown palette IDs fall back to cream.
The selector does not send worklet messages or change project/sound storage.

The project SVG favicon uses cream/lilac X0X marks and keys, without fonts or external artwork.
The Pages landing page links to the same `emu/favicon.svg` asset.

Browser regression command (Playwright and Chromium must be installed separately):

```
# Serve a freshly generated preview beneath /fm1-x0x-rmx/ as described above.
# Optional ORIGINAL_HTML is the pre-P7.3 emulator page for exact Classic Dark comparison.
node tests/host/device_palette_browser_test.mjs \
  http://127.0.0.1:8000/fm1-x0x-rmx/ build/host/p73-browser [ORIGINAL_HTML]
```

`X0X_PLAYWRIGHT` can name an installed Playwright module directory; `X0X_CHROMIUM` can name a
Chromium executable. The test checks geometry, original dark colors/states, button contrast,
pointer/keyboard controls, native selector input isolation, live audio during switching,
both EXTRA pages, independent visual storage, mobile layout, favicon rendering and the
reserved installer. EXTRA comparisons use the fixtures produced by
`node tests/host/bass303_extra_emu_test.mjs build/emu/x0x.wasm` following its host UI fixtures.

Validation artifacts belong in `build/host/p73-browser/` and `build/host/p73-public-browser/`;
they are not published. The P7.3 WASM is byte-identical to deployed P7.2:
SHA-256 `e3951651ecc4363b01f4575ac817ce994e0a26bb0240a00bd4204cc7ddb72dce`.
Firmware/DSP, sequencing, EXTRA, AudioWorklet, mappings, save/load, build flags and memory
configuration are unchanged. No firmware build or new hardware capacity measurement is
required for these browser-only presentation changes. Firmware installation and downloads
remain disabled; publication uses the normal fast-forward `gh-pages` process above.

Local P7.3 verification passed: Emscripten build (669,765-byte WASM), Pages checks,
Chromium palettes/controls/audio/EXTRA/storage/mobile/favicon checks (audio peak 0.318;
no page errors), 303 EXTRA save/reboot regression, host audio parity (168,021 samples,
maximum difference `3.05e-5`), Grids, six LCD schemes/42 screenshots, break slots and
browser sample-conversion parity. The latter passed outside the sandbox because its
Python child process was blocked there. Classic Dark matched the pre-P7.3 page's
computed SVG colors, geometry and hit targets in seven tested control-state combinations.
Button labels pass a 4.5:1 contrast check in both normal and illuminated states.

## P7.4 exterior variants

Sunset Orange and Pink Lilac extend the existing palette configuration with exterior
finishes inspired by the warm amber/sand and rose/violet LCD definitions in `gfx.c`.
Cream & Lilac remains the default and Classic Dark retains its original colors. Existing
`cream`/`dark` selections and the separate visual localStorage key remain compatible.
Device color and GLO > SCHEME are independent: neither selector changes the other.
Firmware palettes, SVG geometry, input mappings, AudioWorklet and favicon are unchanged.

The palette browser regression now exercises all four finishes, existing-palette parity,
label/knob contrast, normal/lit/dim/held/latched controls, mouse/touch/keyboard input,
mobile layouts, reload persistence, live audio, both EXTRA pages, Grids and Remix.
P8.1 documentation finalization is published with these browser presentation changes;
firmware feature freeze and the disabled public installer/downloads remain in effect.
