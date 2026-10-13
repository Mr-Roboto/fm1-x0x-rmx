/* SPDX-License-Identifier: GPL-3.0-only */
/* TB-3PO: generative 303 lines. A port of schwung-tb3po's generator (itself a port
 * of the Phazerville Hemisphere Suite TB_3PO applet, (c) djphazer and contributors,
 * GPL-3.0). This fork uses a scale-aware motif generator with restrained
 * mutation; seed and settings are deterministic, but historical port output
 * is no longer identical. */
#pragma once
#include "pattern.h"

#define TB3PO_NSCALES 22
#define TB3PO_DEFAULT_SCALE 0 /* Minor Pentatonic (MPEN), new configurations only */
extern const char *const TB3PO_SCALE_NAMES[TB3PO_NSCALES];

void tb3po_defaults(tb3po_cfg_t *g, uint32_t seed);
/* write a whole line (len steps) from g (g->seed) */
void tb3po_generate(bpart_t *b);
/* Attempt one small rhythm/articulation/pitch edit per eight-step segment.
 * Anchor pitch/gates stay intact; articulation may vary. Continue
 * the line's random stream from *rng
 * (pass the part's running state; 0 = start from the seed) */
void tb3po_mutate(bpart_t *b, uint32_t *rng);
/* a new seed from any entropy (time, key presses) */
uint32_t tb3po_new_seed(uint32_t entropy);
