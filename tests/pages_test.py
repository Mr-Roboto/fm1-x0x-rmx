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
        self.feed(source)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == "img":
            self.images.append(attrs)
        if "id" in attrs:
            self.ids.add(attrs["id"])
        for attr in ("href", "src"):
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
readme_images = re.findall(r'!\[[^\]]+\]\((docs/img/p82/[^)]+)\)', readme)
readme_images += [img["src"] for img in Links(readme).images]
assert len(readme_images) == 6
for image in readme_images:
    assert Path(image).is_file(), (image, "missing README image")
assert "Original X0X – upstream demonstration" in readme
print("Visual documentation: 19 unchanged publication assets, 6 landing images, 17 manual images and descriptive alt text passed")
print(f"Pages: {len(pages)} pages; relative links/anchors/subpath, identity, credits, release gate and WASM provenance passed")
