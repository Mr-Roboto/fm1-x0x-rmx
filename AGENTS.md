# Repository Guidelines

## Project Identity & Branch Workflow

- This fork is `fm1-x0x-rmx`, derived from `charlesvestal/fm1-x0x`.
- Upstream: https://github.com/charlesvestal/fm1-x0x. `origin`: `Mr-Roboto/fm1-x0x-rmx`.
- Keep `main` as close as possible to `upstream/main`; never put project-specific feature work directly on `main`.
- Current development branch: `orb-remix`. Integrate upstream changes regularly, especially before substantial feature work.

## Current Primary Feature: Remix

The first major feature is a Future Retro Orb-style Remix system. Apply Remix independently to `909`, `808`, `303A`, and `303B`, each with its own independent selection. Leave `BREAK` unaffected. User-facing selection is Bank A–P and Pattern 1–16; `A1` is the neutral sequential mapping.

Once present, `docs/REMIX_DESIGN.md` is the authoritative detailed musical and sequencing specification. Do not invent missing Remix behavior.

## Project Structure & Module Organization

X0X is groovebox firmware for the M-VAVE FM-1. `firmware/src/` contains DSP engines (`dsp/`), sequencing (`seq/`), application logic (`app/`), and device services. Hardware support lives in `firmware/hal/`; the update loader lives in `firmware/loader/`. `host/` runs the application on a computer. `web/` contains browser installers, sample tools, and the WebAssembly emulator. `tools/` holds build and asset generators; `assets/` holds fonts and samples. Tests live in `tests/host/`, `tests/scenarios/`, and top-level `tests/`. Generated files belong in `build/`. Consult `BUILDING.md`, `docs/MANUAL.md`, and `LICENSING.md`.

## Build, Test, and Development Commands

Run commands from the repository root:

- `./build.sh`: generate assets and build firmware, loader, and `build/x0x.fwsc`. Requires Python 3 with Pillow, the JieLi toolchain, and AC79 SDK; non-Linux-x86-64 hosts also need Docker.
- `sh host/build_host.sh`: build `build/host/x0x_host` with a host C compiler.
- `build/host/x0x_host tests/scenarios/getting_started.x0x build/scenarios/getting_started`: run one simulator scenario; create the output directory first.
- `tests/run_tests.sh`: run host, reference-comparison, storage, update, browser, and simulator checks as available.
- `node web/test_breaks.mjs`: check browser sample conversion against Python output.
- `sh web/emu/build.sh`: build the browser emulator with Emscripten.

Set `JIELI_TOOLCHAIN` and `AC79_SDK` to override SDK locations.

## Architecture & Compatibility

- Inspect existing implementations, execution paths, and data structures before modifying code, architecture, or APIs.
- Preserve persistent project/pattern storage sizes whenever practical; prefer existing reserved bytes over enlarging persistent structs. Protect project-format compatibility and migration behavior.
- Static RAM is extremely constrained: the successful baseline firmware build reported `.data+.bss = 97216 B` of `98304 B`. Large immutable Remix lookup tables must live in flash/const data, not writable static RAM; use compact representations where practical.
- Report capacity problems before sacrificing existing firmware functionality to fit a feature. Keep changes modular and localized where practical.

## Development Discipline

- For substantial changes, first inspect and explain the relevant code path and proposed change before implementing, unless explicitly instructed to implement directly.
- Verify functions, fields, hardware/UI behavior, and persistence semantics in the repository or an explicit design document; do not invent them.
- If repository behavior conflicts with a requested design, stop and report the conflict rather than silently choosing an interpretation.
- Make small, focused changes that are easy to review and rebase. Avoid unrelated cleanup or formatting changes.
- Do not commit or push unless explicitly asked. Preserve GPL-3.0 licensing, SPDX headers, and all required attribution.

## Coding Style & Naming Conventions

Follow surrounding code: C99, four-space indentation in C and Python, snake_case functions/files, and uppercase constants. Preserve SPDX headers. Use float arithmetic in DSP; the firmware build rejects linked soft-double routines. Preserve `-ffp-contract=off` for reproducible audio comparisons. No repository-wide formatter or linter configuration is present; respect compiler warnings and avoid unrelated reformatting.

## Testing & Validation

The baseline firmware build is known to succeed with `./build.sh`; the browser emulator is known to build with `sh web/emu/build.sh` and run successfully.

- After firmware-affecting changes, build firmware and report image, RAM, and pool size results.
- After browser-visible or sequencing changes, rebuild the browser emulator as appropriate.
- Run focused tests for the changed subsystem before broader tests. Clearly report tests that could not run because external reference repositories are unavailable.

Use C host tests named `*_test.c` and descriptive `.x0x` simulator scenarios. Add regression coverage for changed audio, sequencing, persistence, or UI behavior. Inspect logs in `build/host/` and scenario audio/screenshots in `build/scenarios/`. Reference comparisons require external source checkouts; report skipped checks. No numeric coverage threshold is documented.

## Commit & Pull Request Guidelines

History uses descriptive subjects such as `Add emulator favicon` and component-prefixed summaries. Keep commits focused. PRs should explain the behavior change, link relevant issues, list checks and skips, and include screenshots for UI changes. Preserve asset attribution and licensing when adding samples or fonts.
