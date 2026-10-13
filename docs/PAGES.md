# X0X-RMX website preparation (P7.1)

The expected URL is `https://mr-roboto.github.io/fm1-x0x-rmx/`, subject to configuration,
deployment and verification. This document does not claim it is live. Source:
[Mr-Roboto/fm1-x0x-rmx](https://github.com/Mr-Roboto/fm1-x0x-rmx), development branch `orb-remix`.
Keep `main` aligned with [original X0X](https://github.com/charlesvestal/fm1-x0x).

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

The emulator compiles current working-tree sources, including Remix and Grids, with the
existing `web/emu/build.sh`; no prebuilt upstream binary is downloaded. Its browser interface,
AudioWorklet, controls, keyboard mapping, LCD, device graphic and favicon are preserved.
The emulator build records the source revision, working-tree changes and WASM SHA-256 in
`emu/build-info.json`. An uncommitted preview correctly reports that its sources are dirty.
This metadata documents provenance, not hardware validation.

## Generated structure

- `index.html`: X0X-RMX landing page, credits and release status.
- `emu/`: `index.html`, `worklet.js`, `x0x.wasm`, unchanged `favicon.svg`, `build-info.json`.
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

## Deferred installation and P7.2

Real FM-1 timing/listening/update validation and explicit approval are required before any
release package, download, public installer or release version. The future
`--validated-release PKG VERSION OUT_DIR` generator mode retains package identity and loader
checks; it is not a validation test and must not be used to bypass the release gate.
Installer libraries, package names, protocol and hardware identifiers are retained.

P7.2 can address the device illustration, favicon and visual branding after approval.
P7.1 changes page identity and text only; it does not recolor or replace those assets.

## P7.1 local verification record

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

No browser executable or browser automation tool was available here. Therefore full HTML/CSS
visual rendering, browser AudioWorklet startup, audible playback and live mouse/keyboard
interaction remain manual preview checks; the successful tests execute the real WASM directly
and compare its LCD/audio output. External links were inspected for correct destinations,
not exhaustively checked for remote availability. GitHub repository Pages settings and public
deployment were not verified or changed.

Firmware sources and build configuration were not modified, and no firmware build/package was
created. The existing P6.4 capacity figures remain the recorded baseline (579,788-byte image,
1,776 bytes free Flash, 97,324 bytes static RAM, 334,348-byte pool); these are not a new P7.1
hardware build measurement. Firmware branding, device illustration and favicon are untouched.
No branch switch, commit, push, release or public deployment was performed.
