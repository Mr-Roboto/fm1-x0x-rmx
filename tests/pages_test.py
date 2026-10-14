#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check package-free Pages output, including links beneath the repository subpath."""
import hashlib
import json
import re
import sys
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urljoin, urlsplit


class Links(HTMLParser):
    def __init__(self, source):
        super().__init__()
        self.links = []
        self.ids = set()
        self.images = []
        self.videos = []
        self.feed(source)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == "img":
            self.images.append(attrs)
        if tag == "video":
            self.videos.append(attrs)
        if "id" in attrs:
            self.ids.add(attrs["id"])
        for attr in ("href", "src", "poster"):
            if attr in attrs:
                self.links.append(attrs[attr])


root = Path(sys.argv[1]).resolve()
base = "http://localhost:8000/fm1-x0x-rmx/"
pages = {p: Links(p.read_text()) for p in root.rglob("*.html")}
for page, parsed in pages.items():
    for link in parsed.links:
        url = urlsplit(urljoin(base + page.relative_to(root).as_posix(), link))
        if url.netloc != "localhost:8000" or url.scheme != "http":
            continue
        assert url.path.startswith("/fm1-x0x-rmx/"), (page, link, "escapes project subpath")
        target = root / unquote(url.path[len("/fm1-x0x-rmx/"):])
        if target.is_dir():
            target /= "index.html"
        assert target.is_file(), (page, link, "missing target")
        if url.fragment and target in pages:
            assert unquote(url.fragment) in pages[target].ids, (page, link, "missing anchor")
for section in ("emu", "manual", "new", "breaks", "install"):
    assert (root / section / "index.html").is_file()
assert not list(root.rglob("*.fwsc")), "preview includes firmware package"
assert not (root / "firmware").exists(), "preview includes firmware directory"
installer = (root / "install/index.html").read_text()
assert not re.search(r"<(script|button|input)\b", installer), "preview enables installation"
assert "validation" in installer and "approval" in installer
for path in ("index.html", "emu/index.html", "manual/index.html", "breaks/index.html"):
    text = (root / path).read_text()
    assert "X0X-RMX" in text
    assert "https://charlesvestal.github.io/fm1-x0x" not in text, (path, "old hosted functionality")
assert "https://github.com/Mr-Roboto/fm1-x0x-rmx" in (root / "index.html").read_text()
assert "https://github.com/charlesvestal/fm1-x0x" in (root / "index.html").read_text()
assert "Original X0X history" in (root / "new/index.html").read_text()
info = json.loads((root / "emu/build-info.json").read_text())
assert info["branch"] == "orb-remix"
assert info["wasm_sha256"] == hashlib.sha256((root / "emu/x0x.wasm").read_bytes()).hexdigest()
assert '<link rel="icon" href="emu/favicon.svg" type="image/svg+xml">' in (root / "index.html").read_text()
assert (root / "emu/favicon.svg").read_bytes() == Path("web/emu/favicon.svg").read_bytes()
assert (root / "emu/worklet.js").read_bytes() == Path("web/emu/worklet.js").read_bytes()
# Publication assets are shared by the landing page and manual, with useful alt text.
landing_path = root / "index.html"
manual_path = root / "manual/index.html"
landing_images = [img for img in pages[landing_path].images if "/p82/" in img.get("src", "")]
manual_images = [img for img in pages[manual_path].images if "/p82/" in img.get("src", "")]
assert len(landing_images) == 6 and len(manual_images) == 17
for img in landing_images + manual_images:
    assert len(img.get("alt", "").strip()) > 20, (img, "missing descriptive alternative text")
assert landing_images[0]["src"].endswith("01-overview-cream.png")
landing = landing_path.read_text()
assert landing.index("01-overview-cream.png") < landing.index('<nav class="ways"') < landing.index("upstream-demo")
assert "Original X0X – upstream demonstration" in landing
assets = root / "manual/img/p82"
assert len(list(assets.glob("*.png"))) == 19
for image in assets.glob("*.png"):
    assert image.read_bytes() == (Path("docs/img/p82") / image.name).read_bytes(), image
assert "Rejection is silent; there is no warning banner." in manual_path.read_text()
readme = Path("README.md").read_text()
readme_images = re.findall(r'!\[[^\]]+\]\((docs/img/[^)]+)\)', readme)
readme_images += [img["src"] for img in Links(readme).images]
assert len(readme_images) == 8
for image in readme_images:
    assert Path(image).is_file(), (image, "missing README image")
assert "Original X0X – upstream demonstration" in readme
banner_name = "x0x-rmx-banner-1280x320.png"
banner = Path("docs/img") / banner_name
assert (root / "manual/img" / banner_name).read_bytes() == banner.read_bytes()
assert hashlib.sha256(banner.read_bytes()).hexdigest() == "349b82e67d76fd3968249f9555c47a7fdc75f3ef3856f2b975a01f76ce85874d"
assert readme.startswith("![X0X-RMX") and readme.splitlines()[0].endswith(banner_name + ")")
assert pages[landing_path].images[0]["src"].endswith(banner_name)
assert 'width="1280" height="320"' in landing
assert '<h1 class="branding">' in landing and '<h1>X0X-RMX</h1>' not in landing
print("Approved banner bytes, source/Pages paths and first-image placement passed")
print("Visual documentation: 19 unchanged publication assets, 6 landing images, 17 manual images and descriptive alt text passed")
print(f"Pages: {len(pages)} pages; relative links/anchors/subpath, identity, credits, release gate and WASM provenance passed")

# Approved musical demonstration: unchanged bytes, accessible native controls,
# conservative loading, separate from upstream hardware demonstration/release gates.
video_assets = Path("docs/media/video")
for name, approved_hash in (
    ("x0x-rmx-demo.mp4", "84adf956774d1678af3b0b8e3303841e968f01ca5533e98a210d1ce27765960c"),
    ("x0x-rmx-demo-poster.png", "e66e897da7736ccfd02d77e20094275324e56e3ce775ca0af2375e24411d2811"),
):
    source = (video_assets / name).read_bytes()
    assert hashlib.sha256(source).hexdigest() == approved_hash, name
    assert (root / "media/video" / name).read_bytes() == source, name
videos = pages[landing_path].videos
assert len(videos) == 1
video = videos[0]
assert "controls" in video and "playsinline" in video
assert "autoplay" not in video and "muted" not in video
assert video["preload"] == "metadata"
assert video["src"] == "media/video/x0x-rmx-demo.mp4"
assert video["poster"] == "media/video/x0x-rmx-demo-poster.png"
assert video["width"] == "1280" and video["height"] == "720"
assert "browser-emulator" in video["aria-label"]
assert landing.index('id="demo"') < landing.index('id="upstream-demo"')
assert "Browser demonstration; physical FM-1 validation remains pending." in landing
assert 'href="https://mr-roboto.github.io/fm1-x0x-rmx/#demo"' in readme
assert 'src="docs/media/video/x0x-rmx-demo-poster.png"' in readme
assert "This is a browser-emulator demonstration" in readme
print("Approved video/poster byte integrity, URLs, player controls and browser-only identification passed")
