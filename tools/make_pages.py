#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate X0X-RMX Pages locally, without installing or publishing anything.

Default/package-free: tools/make_pages.py --preview build/pages
Future approved release: tools/make_pages.py --validated-release PKG VERSION OUT_DIR
The latter retains the original package identity and loader checks. Use it only
following physical FM-1 validation and explicit release approval.
"""
import argparse
import hashlib
import html
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SRC / "web"))
from make_site import product_of, strip_module  # noqa: E402  (Felucca's: the package format)

REPO = "https://github.com/Mr-Roboto/fm1-x0x-rmx"
UPSTREAM = "https://github.com/charlesvestal/fm1-x0x"


def is_release_identity(product):
    # Retain historical package support; neither development identity is publishable.
    return (re.fullmatch(r"FM-1_(?:987[0-9]{6}|9(?:[0-9]{2}|[0-9]{6}))", product) is not None
            and product not in ("FM-1_900", "FM-1_987"))


LANDING = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>X0X-RMX for the FM-1</title>
<meta name="description" content="Independent X0X fork: Orb Remix, Grids MAP/EUCLID, extended TB-3PO and 303 Morph.">
<link rel="icon" href="emu/favicon.svg" type="image/svg+xml">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Barlow:wght@400;600&family=Barlow+Semi+Condensed:wght@600;700&display=swap">
<style>
:root { --ground: #f4f5f6; --ink: #1c1f23; --muted: #5b626a; --rule: #d9dde1; --accent: #c8700a; --card: #fff; color-scheme: light; }
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) { --ground: #131518; --ink: #e6e8eb; --muted: #9aa1a9; --rule: #2d3137; --accent: #f0a043; --card: #1a1d21; color-scheme: dark; }
}
:root[data-theme="dark"] { --ground: #131518; --ink: #e6e8eb; --muted: #9aa1a9; --rule: #2d3137; --accent: #f0a043; --card: #1a1d21; color-scheme: dark; }
body { background: var(--ground); color: var(--ink); margin: 0; padding-inline: 16px; padding-block: 48px 64px;
       font: 400 17px/1.6 "Barlow", "Helvetica Neue", Arial, sans-serif; }
main { max-width: 40rem; margin: 0 auto; }
.branding { margin-bottom: 1.6rem; }
.branding img { display: block; width: 100%; height: auto; }
h1 { font: 700 2.6rem/1.1 "Barlow Semi Condensed", "Barlow", sans-serif; margin: 0 0 .4rem; }
.lede { color: var(--muted); margin: 0 0 1.6rem; max-width: 34rem; }
.status { border: 1px solid var(--rule); background: var(--card); border-radius: 8px; padding: 12px 16px; margin: 0 0 2rem; }
.ways { display: grid; gap: 12px; margin: 0 0 2rem; }
.ways a { display: block; text-decoration: none; color: var(--ink); background: var(--card); border: 1px solid var(--rule);
          border-radius: 10px; padding: 14px 18px; }
.ways a:hover, .ways a:focus-visible { border-color: var(--accent); outline: none; }
.ways .card { background: var(--card); border: 1px solid var(--rule); border-radius: 10px; padding: 14px 18px; }
.ways .card a { color: var(--accent); text-decoration: none; border: 0; padding: 0; background: none; display: inline; }
.ways .card a:hover, .ways .card a:focus-visible { text-decoration: underline; }
.ways .card a.new { font: 400 .95rem "Barlow", sans-serif; margin-left: .5rem; }
.ways strong { font: 600 1.25rem "Barlow Semi Condensed", "Barlow", sans-serif; color: var(--accent); display: block; }
.ways span { color: var(--muted); font-size: .95rem; }
figure { margin: 0 0 1.6rem; }
figure img { display: block; width: 100%; height: auto; border-radius: 8px; }
figcaption { color: var(--muted); font-size: .95rem; margin-top: .6rem; }
.gallery { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 20px; }
.gallery figure img { image-rendering: pixelated; }
.gallery strong { color: var(--ink); }
@media (max-width: 520px) { .gallery { grid-template-columns: 1fr; } }
p.small { color: var(--muted); font-size: .9rem; }
.video { position: relative; aspect-ratio: 16 / 9; max-width: 100%; margin: 1.5rem 0; border-radius: 6px; overflow: hidden; background: #000; }
.performance-video { display: block; width: 100%; height: auto; aspect-ratio: 16 / 9; border-radius: 8px; background: #211d29; }
.video iframe { position: absolute; inset: 0; width: 100%; height: 100%; border: 0; }
a { color: var(--accent); }
</style>
</head>
<body>
<main>
<h1 class="branding"><img src="manual/img/x0x-rmx-banner-1280x320.png" width="1280" height="320"
  alt="X0X-RMX — Orb-inspired Remix for the FM-1" fetchpriority="high"></h1>
<p class="lede">Groovebox firmware for the M-VAVE FM-1. A 909, an 808, two 303s with the TB-3PO
line generator, and a breakbeat player, all running at once, with patterns, a song mode and
recorded knob moves. This independent fork adds Orb-inspired Remix for 909, 808, 303A and 303B,
Grids MAP/EUCLID drum generation with NEW/MUTATE, FILL, OFFSET, CHAOS and Undo,
extended TB-3PO, a 303 Morph waveform, and additional color schemes and UI improvements.</p>
<figure class="hero">
  <a href="emu/"><img src="manual/img/p82/01-overview-cream.png" width="3120" height="2175"
    alt="X0X-RMX browser emulator in Cream &amp; Lilac, showing a 909 pattern and the Device color selector" loading="lazy"></a>
  <figcaption>The default Cream &amp; Lilac exterior, shown with the WHITE LILAC LCD scheme. Play it with mouse, touch or keyboard.</figcaption>
</figure>
<div class="status">__STATUS__</div>
<nav class="ways" aria-label="Explore X0X-RMX">
  <a href="emu/"><strong>Try it in the browser</strong><span>The current X0X-RMX sources compiled for the browser, with sound. Mouse, touch or keyboard; no FM-1 needed.</span></a>
  __INSTALL__
  <a href="new/"><strong>Version history</strong><span>Fork development changes and clearly labelled upstream history.</span></a>
  <a href="manual/"><strong>Manual</strong><span>Getting started in ten steps, then everything else.</span></a>
  <a href="breaks/"><strong>Your own breaks</strong><span>Drop in loops and send them to the BREAK part, from Chrome or Edge.</span></a>
  __DOWNLOAD__
  <a href="__REPO__"><strong>Source</strong><span>X0X-RMX on GitHub, GPL-3.0.</span></a>
</nav>
<section aria-labelledby="demo">
<h2 id="demo">X0X-RMX musical demonstration</h2>
<figure>
  <video class="performance-video" controls playsinline preload="metadata" width="1280" height="720"
    poster="media/video/x0x-rmx-demo-poster.png" src="media/video/x0x-rmx-demo.mp4"
    aria-label="X0X-RMX musical browser-emulator demonstration">
    <a href="media/video/x0x-rmx-demo.mp4">Watch the X0X-RMX demonstration (MP4)</a>
  </video>
  <figcaption>84 seconds of Remix, Grids, TB-3PO and Morph with real stereo browser-emulator audio.
    Earlier browser-emulator recording, not physical FM-1 footage. The outro was updated to confirm successful v1.0.0 real-device validation.
    <a href="media/video/x0x-rmx-demo.mp4">Open the video (MP4, 6.38 MB)</a>.</figcaption>
</figure>
</section>
<section aria-labelledby="features">
<h2 id="features">Inside X0X-RMX</h2>
<div class="gallery">
  <figure><a href="manual/#remix"><img src="manual/img/p82/06-remix-909-on-b3.png" width="627" height="627" loading="lazy"
    alt="909 Remix with Bank B, Pattern 3, Remix ON and APPLY BAR"></a>
    <figcaption><strong>Remix.</strong> Reorder playback while keeping the stored steps.</figcaption></figure>
  <figure><a href="manual/#grids-drum-generator"><img src="manual/img/p82/08-grids-map-909.png" width="627" height="627" loading="lazy"
    alt="909 Grids MAP page with X, Y, CHAOS and MODE controls"></a>
    <figcaption><strong>Grids.</strong> Generate and mutate kick, snare and closed-hat rhythms.</figcaption></figure>
  <figure><a href="manual/#s7"><img src="manual/img/p82/15-tb3po-generate-303a.png" width="627" height="627" loading="lazy"
    alt="303A TB-3PO GENERATE page with density, accent, slide and octave controls"></a>
    <figcaption><strong>TB-3PO.</strong> Generate bass lines with 22 scales and phrasing controls.</figcaption></figure>
  <figure><a href="manual/#s6"><img src="manual/img/p82/22-303a-extra-morph64.png" width="627" height="627" loading="lazy"
    alt="303A EXTRA page with MORPH 64, ACCDEC, DRVTYPE and TUNE"></a>
    <figcaption><strong>Morph.</strong> Shape the experimental waveform on the EXTRA page. One Morph voice at a time.</figcaption></figure>
</div>
</section>
<section aria-labelledby="device-colors">
<h2 id="device-colors">Four emulator device colors</h2>
<p>Choose Cream &amp; Lilac (default), Classic Dark, Sunset Orange or Pink Lilac with the
browser's Device color selector. Switching is immediate and the visual preference is remembered
separately from your project. GLO → SCHEME controls LCD colors independently.</p>
<figure><a href="manual/img/p82/01-device-comparison.png"><img src="manual/img/p82/01-device-comparison.png" width="2140" height="1712" loading="lazy"
  alt="The same 909 pattern with matching device and LCD colors: Cream &amp; Lilac / WHITE LILAC, Classic Dark / DEFAULT, Sunset Orange / SUNSET ORANGE and Pink Lilac / PINK LILAC"></a>
  <figcaption>Four browser finishes with coordinated LCD schemes, selected separately in GLO → SCHEME. Visual interpretations, not color-matched hardware photographs. Open the image for a closer look.</figcaption></figure>
</section>
<section aria-labelledby="upstream-demo">
<h2 id="upstream-demo">Original X0X – upstream demonstration</h2>
<p class="small">Original X0X demonstration by Charles Vestal; it does not demonstrate the fork additions.</p>
<div class="video"><iframe src="https://www.youtube-nocookie.com/embed/cZLYrZaLDUk" title="Original X0X by Charles Vestal running on the FM-1"
  allow="accelerometer; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen loading="lazy"></iframe></div>
</section>
<p class="small">Based on <a href="__UPSTREAM__">Charles Vestal's original X0X</a> and
<a href="https://github.com/hugelton/Felucca">Felucca</a> by Leo Kuroshita and contributors,
Hügelton Instruments. Grids by Emilie Gillet / Mutable Instruments; MAP arithmetic cross-checked
against DEADACTIVE's Hortator. See the <a href="manual/#s19">complete credits</a> and
<a href="__REPO__/blob/orb-remix/LICENSING.md">licensing notices</a>.</p>
<p class="small">An independent fork without implied endorsement by the original author.
M-VAVE, FM-1, TR-808, TR-909 and TB-303 are trademarks of their owners; no affiliation or endorsement.</p>
</main>
</body>
</html>
"""


RESERVED = """<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>X0X-RMX release status</title></head><body><main>
<a href="../">X0X-RMX</a><h1>Documentation-only preview</h1>
<p>X0X-RMX v1.0.0 Stable has completed successful real M-VAVE FM-1 testing and received release approval. This package-free preview intentionally excludes installation controls.
No firmware package, download or installation controls are available here.</p>
<p>You can <a href="../emu/">play the browser emulator</a> or
<a href="../manual/">read the manual</a>. Browser testing does not validate hardware timing.</p>
</main></body></html>"""


def site_doc_links(page, firmware_name=None):
    """Resolve repository Markdown links without confusing them with Pages routes."""
    def link(match):
        url = match.group(1)
        site = "https://mr-roboto.github.io/fm1-x0x-rmx/"
        if url.startswith(site):
            # Historical download links stay public when that package is absent
            # from a documentation preview or a different future release.
            if url.startswith(site + "firmware/") and url[len(site + "firmware/"):] not in (
                    firmware_name, (firmware_name + ".sha256") if firmware_name else None):
                return match.group(0)
            return 'href="../' + url[len(site):] + '"'
        if ":" in url or url.startswith("#"):
            return match.group(0)
        path, _, fragment = url.partition("#")
        if not path.endswith(".md"):
            return match.group(0)
        if path in ("MANUAL.md", "NEW.md"):
            target = "../manual/" if path == "MANUAL.md" else "../new/"
            if fragment:
                fragment = re.sub(r"^(\d+)-.*", r"s\1", fragment)
                target += "#" + fragment
        else:
            source = (Path("docs") / path).as_posix()
            if path.startswith("../"):
                source = path[3:]
            target = REPO + "/blob/orb-remix/" + source
            if fragment:
                target += "#" + fragment
        return 'href="' + target + '"'
    return re.sub(r'href="([^"<>]+)"', link, page)


def main(out, pkg=None, version=None):
    out = Path(out).resolve()
    # Never recursively remove an arbitrary destination or leave stale firmware in a preview.
    build = (SRC / "build").resolve()
    if build not in out.parents:
        raise SystemExit("output must be a subdirectory of build/")
    if out.exists() and any(out.iterdir()):
        raise SystemExit(f"{out}: output must be empty; choose a fresh build subdirectory")
    emu = SRC / "build" / "emu"
    if out == emu or emu in out.parents:
        raise SystemExit("output must not overlap build/emu")
    if not (emu / "x0x.wasm").exists():
        raise SystemExit("no build/emu/x0x.wasm: run sh web/emu/build.sh")
    info_path = emu / "build-info.json"
    if not info_path.exists():
        raise SystemExit("missing emulator provenance: rebuild with sh web/emu/build.sh")
    info = json.loads(info_path.read_text(encoding="utf-8"))
    if info.get("wasm_sha256") != hashlib.sha256((emu / "x0x.wasm").read_bytes()).hexdigest():
        raise SystemExit("emulator hash mismatch: rebuild with sh web/emu/build.sh")
    raw, product, name = None, None, None
    if pkg is not None:
        pkg = Path(pkg)
        raw = pkg.read_bytes()
        product = product_of(raw)
        if not is_release_identity(product):
            raise SystemExit(f"{pkg}: identity {product!r}: requires a release build")
        if b"FELUCCA-LOADER-1" not in raw:
            raise SystemExit(f"{pkg}: no update loader in it")
        name = f"x0x-{re.sub(r'[^A-Za-z0-9.-]', '-', version)}.fwsc"
    for d in ("install", "manual", "breaks", "new"):
        (out / d).mkdir(parents=True, exist_ok=True)
    if pkg is None:
        (out / "install" / "index.html").write_text(RESERVED, encoding="utf-8")
    else:
        (out / "firmware").mkdir()
        shutil.copy(pkg, out / "firmware" / name)
        (out / "firmware" / (name + ".sha256")).write_text(
            hashlib.sha256(raw).hexdigest() + "  " + name + "\n", encoding="utf-8")
        page = (SRC / "web" / "x0x_installer.html").read_text(encoding="utf-8")
        lib = strip_module((SRC / "web" / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
            strip_module((SRC / "web" / "fm1ota.js").read_text(encoding="utf-8"))
        meta = json.dumps({"version": version, "product": product, "pkg": "../firmware/" + name})
        for mark in ("/*LIB*/", "/*META*/"):
            if page.count(mark) != 1:
                raise SystemExit(f"x0x_installer.html must contain {mark} once")
        (out / "install" / "index.html").write_text(page.replace("/*LIB*/", lib).replace("/*META*/", meta), encoding="utf-8")
    # the break loop uploader
    page = (SRC / "web" / "x0x_breaks.html").read_text(encoding="utf-8")
    if page.count("/*LIB*/") != 1:
        raise SystemExit("x0x_breaks.html must contain /*LIB*/ once")
    lib = strip_module((SRC / "web" / "x0x_breaks.js").read_text(encoding="utf-8"))
    (out / "breaks" / "index.html").write_text(page.replace("/*LIB*/", lib), encoding="utf-8")
    # Approved demonstration assets, including the corrected release outro; copied without transcoding.
    shutil.copytree(SRC / "docs" / "media" / "video", out / "media" / "video")
    # the manual (a complete page; its images beside it)
    subprocess.run([sys.executable, str(SRC / "tools" / "manual_html.py"), str(SRC / "docs" / "MANUAL.md"),
                    str(out / "manual" / "index.html")], check=True, capture_output=True)
    shutil.copytree(SRC / "docs" / "img", out / "manual" / "img")
    # what's new (the same converter; its pictures beside it)
    subprocess.run([sys.executable, str(SRC / "tools" / "manual_html.py"), str(SRC / "docs" / "NEW.md"),
                    str(out / "new" / "index.html")], check=True, capture_output=True)
    shutil.copytree(SRC / "docs" / "img" / "new", out / "new" / "img" / "new")
    for section in ("manual", "new"):
        doc = out / section / "index.html"
        doc.write_text(site_doc_links(doc.read_text(encoding="utf-8"), name), encoding="utf-8")
    status = "Documentation-only preview of X0X-RMX v1.0.0 Stable, successfully tested on real M-VAVE FM-1 hardware. This preview excludes the firmware package and installer; use the official release for installation."
    install = '<a href="install/"><strong>Firmware release status</strong><span>Package-free local preview; use the official Stable release.</span></a>'
    download = ""
    if pkg is not None:
        status = "X0X-RMX " + html.escape(version) + " Stable, successfully tested on a real M-VAVE FM-1. Hardware update, pattern/sample retention, startup patterns, chain playback, a twelve-minute musical run and USB audio with iPad/AudioShare were confirmed successful."
        install = '<a href="install/"><strong>Install ' + html.escape(version) + '</strong><span>Chrome or Edge, Web MIDI.</span></a>'
        download = ('<a href="firmware/' + name + '"><strong>Download ' + name + '</strong></a>'
                    + '<a href="firmware/' + name + '.sha256"><strong>SHA-256 checksum</strong></a>'
                    + '<a href="' + REPO + '/releases/tag/v' + html.escape(version)
                    + '"><strong>GitHub release ' + html.escape(version) + '</strong></a>')
    landing = LANDING.replace("__STATUS__", status).replace("__INSTALL__", install).replace("__DOWNLOAD__", download)
    (out / "index.html").write_text(landing.replace("__REPO__", REPO).replace("__UPSTREAM__", UPSTREAM), encoding="utf-8")
    (out / "emu").mkdir()
    for asset in ("index.html", "worklet.js", "x0x.wasm", "favicon.svg", "build-info.json"):
        shutil.copy(emu / asset, out / "emu" / asset)
    (out / ".nojekyll").write_text("")
    print(f"site: {out}: index.html, emu/, manual/, new/, breaks/, install/; " +
          ("installation reserved, no firmware" if pkg is None else f"validated-release package {name}"))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--preview", metavar="OUT_DIR")
    mode.add_argument("--validated-release", nargs=3, metavar=("PKG", "VERSION", "OUT_DIR"))
    args = parser.parse_args()
    if args.preview:
        main(args.preview)
    else:
        pkg, version, out = args.validated_release
        main(out, pkg, version)
