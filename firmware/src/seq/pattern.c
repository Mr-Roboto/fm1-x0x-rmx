/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X pattern defaults. */
#include "pattern.h"
#include "tb3po.h"

static const uint8_t BRK_DEFAULTS[BRK_NSET] = {
    25, 60, 20, 50,                   /* complexity, anchor, roll, fill */
    10, 0, 5, 0,                      /* retrig 2x 3x 4x 8x */
    2, 0, 2, 2,                       /* phrase 4 bars, B chance 0, A length 1 bar, B length 1 bar */
};

void pattern_init(pattern_t *p, uint32_t seed_a, uint32_t seed_b)
{
    uint32_t i, b;
    uint8_t *z = (uint8_t *)p;
    for (i = 0; i < sizeof *p; i++)
        z[i] = 0;
    for (i = 0; i < NKIT; i++) {
        p->drum[i].len = 16;
        p->drum[i].rate = RATE_16;
    }
    for (b = 0; b < NBASS; b++) {
        bpart_t *bp = &p->bass[b];
        bp->len = 16;
        bp->rate = RATE_16;
        bp->dir = DIR_FWD;
        bp->transpose = 24;
        tb3po_defaults(&bp->gen, b ? seed_b : seed_a);
        if (b)
            bp->gen.base_oct = 2;     /* 303 B sits an octave above A by default */
        for (i = 0; i < NSTEPS; i++) {
            bp->step[i].note = (uint8_t)(12 * (bp->gen.base_oct + 1) + bp->gen.root);
            bp->step[i].flags = G_REST;
        }
    }
    p->brk.steps = 0;                 /* the break is silent until switched on */
    for (i = 0; i < BRK_NSET; i++)
        p->brk.set[i] = BRK_DEFAULTS[i];
    p->brk.slot_a = 0;                /* the built-in bank's first two: the groove, and the fill for B */
    p->brk.slot_b = 1;
}

#define REMIX_MARKER 0xA4u

static int remix_valid(const pattern_t *p, int part)
{
    if ((unsigned)part >= NKIT + NBASS)
        return 0;
    return part < NKIT ? (p->drum[part].rsv[1] & 0xFEu) == REMIX_MARKER
                       : (p->rsv[2] & 0xFCu) == REMIX_MARKER;
}

uint8_t pattern_remix_selection(const pattern_t *p, int part)
{
    if (!remix_valid(p, part))
        return 0;
    return part < NKIT ? p->drum[part].rsv[0] : p->rsv[part - NKIT];
}

int pattern_remix_enabled(const pattern_t *p, int part)
{
    if (!remix_valid(p, part))
        return 0;
    return part < NKIT ? p->drum[part].rsv[1] & 1u
                       : (p->rsv[2] >> (part - NKIT)) & 1u;
}

/* Establishing a shared bass marker must reset BOTH legacy selections. */
static void remix_prepare(pattern_t *p, int part)
{
    if (remix_valid(p, part))
        return;
    if (part < NKIT) {
        p->drum[part].rsv[0] = 0;
        p->drum[part].rsv[1] = REMIX_MARKER;
    } else {
        p->rsv[0] = p->rsv[1] = 0;
        p->rsv[2] = REMIX_MARKER;
    }
}

void pattern_set_remix_selection(pattern_t *p, int part, int bank, int pattern)
{
    if ((unsigned)part >= NKIT + NBASS || (unsigned)bank >= 16u || (unsigned)pattern >= 16u)
        return;
    remix_prepare(p, part);
    if (part < NKIT)
        p->drum[part].rsv[0] = (uint8_t)((bank << 4) | pattern);
    else
        p->rsv[part - NKIT] = (uint8_t)((bank << 4) | pattern);
}

void pattern_set_remix_enabled(pattern_t *p, int part, int enabled)
{
    uint8_t *state, bit;
    if ((unsigned)part >= NKIT + NBASS)
        return;
    remix_prepare(p, part);
    state = part < NKIT ? &p->drum[part].rsv[1] : &p->rsv[2];
    bit = (uint8_t)(1u << (part < NKIT ? 0 : part - NKIT));
    *state = (uint8_t)((*state & ~bit) | (enabled ? bit : 0));
}
