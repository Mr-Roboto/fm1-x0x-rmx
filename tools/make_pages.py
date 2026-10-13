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

LANDING = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>X0X-RMX for the FM-1</title>
<meta name="description" content="Independent X0X fork: Orb Remix, Grids MAP/EUCLID, extended TB-3PO and 303 Morph.">
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
p.small { color: var(--muted); font-size: .9rem; }
.video { position: relative; aspect-ratio: 16 / 9; max-width: 100%; margin: 1.5rem 0; border-radius: 6px; overflow: hidden; background: #000; }
.video iframe { position: absolute; inset: 0; width: 100%; height: 100%; border: 0; }
a { color: var(--accent); }
</style>
</head>
<body>
<main>
<h1>X0X-RMX</h1>
<p class="lede">Groovebox firmware for the M-VAVE FM-1. A 909, an 808, two 303s with the TB-3PO
line generator, and a breakbeat player, all running at once, with patterns, a song mode and
recorded knob moves. This independent fork adds Orb-inspired Remix for 909, 808, 303A and 303B,
Grids MAP/EUCLID drum generation with NEW/MUTATE, FILL, OFFSET, CHAOS and Undo,
extended TB-3PO, a 303 Morph waveform, and additional color schemes and UI improvements.</p>
<p class="small">Original X0X demonstration by Charles Vestal; it does not demonstrate the fork additions.</p>
<div class="video"><iframe src="https://www.youtube-nocookie.com/embed/cZLYrZaLDUk" title="X0X running on the FM-1"
  allow="accelerometer; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen loading="lazy"></iframe></div>
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
<a href="../">X0X-RMX</a><h1>Firmware installation is reserved</h1>
<p>Physical FM-1 validation and explicit release approval are pending.
No firmware package, download or installation controls are available here.</p>
<p>You can <a href="../emu/">play the browser emulator</a> or
<a href="../manual/">read the manual</a>. Browser testing does not validate hardware timing.</p>
</main></body></html>"""


def site_doc_links(page):
    """Resolve repository Markdown links without confusing them with Pages routes."""
    def link(match):
        url = match.group(1)
        site = "https://mr-roboto.github.io/fm1-x0x-rmx/"
        if url.startswith(site):
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
        if not re.fullmatch(r"FM-1_9(\d\d|\d{6})", product) or product == "FM-1_900":
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
        doc.write_text(site_doc_links(doc.read_text(encoding="utf-8")), encoding="utf-8")
    status = "Physical FM-1 validation is pending. The browser emulator is available here; firmware installation and downloads are reserved."
    install = '<a href="install/"><strong>Firmware release status</strong><span>Reserved until hardware validation and explicit approval.</span></a>'
    download = ""
    if pkg is not None:
        status = "Validated release " + html.escape(version) + ". Installing firmware is at your own risk."
        install = '<a href="install/"><strong>Install ' + html.escape(version) + '</strong><span>Chrome or Edge, Web MIDI.</span></a>'
        download = '<a href="firmware/' + name + '"><strong>Download ' + name + '</strong></a>'
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
