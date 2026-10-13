# Grids source data

`resources.cc` is the unmodified Mutable Instruments eurorack file at commit
`08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4`, the revision also pinned by
DEADACTIVE Hortator's Grids fidelity tests.

- Source: https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/grids/resources.cc
- Copyright 2012 Emilie Gillet; GPL-3.0-or-later. Original notices are retained;
  the GPLv3 text is in the repository's `LICENSE`.
- SHA-256: `02c4911d1ee940f921d50207c25a39d911c3f6af9ace87869e7e667954c63265`.

`tools/gen_grids.py` verifies this checksum and emits only the even original
positions of all three channels in all 25 nodes: 1,200 bytes, arranged in the
original 5x5 spatial order. The full MAP and Euclidean table remain source/test
inputs only; neither is linked into firmware. Builds need no network.

MAP arithmetic is adapted from Emilie Gillet's `grids/pattern_generator.cc`,
copyright 2011, GPL-3.0-or-later. Its C interpretation was cross-checked against
DEADACTIVE's 2026 GPL-3.0-or-later Hortator port at
`7e3415971c5800b4a3ad0146a7d95615883d7d96`, `firmware/src/grids.c`.
X0X's offline RNG draw schedule intentionally differs from live Grids clocking.
Euclidean exact-percentage FILL, arithmetic phase, and hit-attached accents are
deliberate adaptations, not claims of bit-identical Grids playback.
