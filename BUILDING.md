# Building X0X

The build makes three files in `build/`:

| File | What |
| --- | --- |
| `x0x.bin` | the firmware app |
| `loader/ota.bin` | the update loader |
| `x0x.fwsc` | the installable package (app + loader) |

## Prerequisites (macOS)

- Python 3 with Pillow: `pip3 install Pillow`
- Docker Desktop. The JieLi toolchain is Linux x86-64 only; the build runs each tool in a
  `linux/amd64` `debian:bookworm-slim` container (Rosetta on Apple silicon). Keep the source
  tree in a folder Docker can share, e.g. under `/Users`.
- The JieLi Linux toolchain (clang 4.0.1 for pi32v2, from JieLi's package server):

  ```
  tools/get_toolchain.sh            # installs to ~/.jieli/toolchain
  ```

- The JieLi AC79 SDK (Apache-2.0). The package uses three of its files
  (`cpu/wl82/tools/uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`); they are not part of this tree.

  ```
  git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
      https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
  ```

- Node.js (optional, for the web tests).

On Linux x86-64 the toolchain runs natively and Docker is not needed.

## Build

```
./build.sh
```

`JIELI_TOOLCHAIN` and `AC79_SDK` override the default locations
(`~/.jieli/toolchain`, `~/fw-AC79_AIoT_SDK`).

`./build.sh --release 0.1-beta` makes a release build; the package is `build/x0x-0.1-beta.fwsc`.

On macOS with podman instead of Docker, put a `docker` script that runs `exec podman "$@"` first
on your PATH. `X0X_JOBS` (default 4) limits parallel compiles: a podman machine drops
connections when many containers start at once.

Build option: `X0X_CDC=1` adds Felucca's USB serial function (off by default: one plain
MIDI interface).

The build also generates `build/gen/`: the font, the 909's samples and tables
(`tools/gen_drum_samples.py`), and the built-in break loops, rendered by the 909 port on the
host (`tools/gen_builtin_break.sh`, needs a host `cc`). Opt-in, `X0X_BREAK_BANK=DIR` builds
the break bank from one-bar WAVs instead (`tools/import_breaks.py` makes such a folder from
BB Gen's samples, in `build/breaks/`); see README.md, Breaks. It fails if a soft-double routine is
linked (a `double` crept in) and checks the image, RAM and pool sizes.

## Tests

```
tests/run_tests.sh
```

Runs, on the build machine: the maths library against libm; the sequencer and TB-3PO
(local scale, phrasing, mutation and UI regressions); each engine against its
original (9W9, 8W8, schwung-303, BB Gen — the scripts in `tests/host/` say where they look
for those sources); flash storage; Felucca's update-path tests against `build/x0x.fwsc`
(the loader test needs `AC79_SDK`); and the whole app in the simulator
(`tests/scenarios/*.x0x`), with its screenshots and audio in `build/scenarios/`.

The experimental Wave 3 integration also has focused checks:

```
sh tests/host/run_bass303_waveform.sh   # cubic prototype identity, legacy bits, Morph and isolation
sh tests/host/run_bass303_ui.sh         # UI, command/motion exclusion and project persistence
```

These use the local host waveform prototype; the UI check also uses the generated assets in
`build/gen/`. They are included in `tests/run_tests.sh` and do not generate WAV collections or
run spectral matrices. Passing them, or building firmware and the emulator, does not validate
FM1 CPU deadlines or device listening. Those checks remain pending; see the
[waveform specification](docs/TB303_WAVEFORMS_DESIGN.md).

`host/build_host.sh` builds the simulator alone; `build/host/x0x_host SCRIPT OUTDIR` runs one
script (the command list is at the top of `host/x0x_host.c`).

## Installation tooling

Physical FM-1 validation of X0X-RMX is pending. No public fork installer, firmware download
or tested release is enabled. Charles Vestal's installer installs original X0X, not this fork.
The existing package format, hardware identities, loader and command-line tools are retained.
For a future validated and approved package, the command-line installer accepts a local file
(needs `pip3 install mido python-rtmidi`):

```
python3 tools/fm1_install.py --info          # identity of the connected FM-1
python3 tools/fm1_install.py PATH_TO_APPROVED_PACKAGE.fwsc
```

If the FM-1 no longer starts but reaches the chip's update mode (4C4A:8057 on USB),
`tools/fm1_rescue.sh` restores stock V15 from a Mac; see the manual's Troubleshooting section.
Otherwise recovery needs [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter).

## Local website preview and future publication

The site no longer needs a firmware package for preview or emulator-only publication:

```
sh web/emu/build.sh
python3 tools/make_pages.py --preview build/preview/fm1-x0x-rmx
python3 -m http.server 8000 --directory build/preview --bind 127.0.0.1
# http://localhost:8000/fm1-x0x-rmx/
```

Use a fresh output directory; the generator refuses to overwrite existing files.
`tools/publish_pages.sh build/OUTPUT_DIR` now only builds the emulator and prepares a local
package-free site. It does not push, deploy, create a release or include a firmware package.
See [docs/PAGES.md](docs/PAGES.md) for review, GitHub settings and the future procedure that
preserves `gh-pages` history without force-pushing. Keep `main` aligned with upstream and
prepare the fork from `orb-remix`.

The future package-enabled generator command is deliberately explicit:

```
python3 tools/make_pages.py --validated-release PATH_TO_APPROVED_PACKAGE.fwsc VERSION build/release-site
```

Run that only after real-device validation and explicit release approval. It retains the
original release product and loader checks and installer protocol. No package-enabled site
or release is created as part of P7.1.
