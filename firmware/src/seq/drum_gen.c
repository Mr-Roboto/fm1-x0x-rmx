/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2011-2012 Emilie Gillet (Mutable Instruments Grids).
 * Offline adaptation for X0X, 2026. Interpolation/threshold/perturbation after
 * grids/pattern_generator.cc; C arithmetic cross-checked against DEADACTIVE's
 * Hortator grids.c (2026, GPL-3.0-or-later). See assets/grids/README.md. */
#include "drum_gen.h"
#include "x0x_grids_map.h"

#define DG_DEFAULT(seed) {seed, DG_MAP, 128, 128, 0, {50, 35, 65}, {16, 16, 16}, {0, 4, 2}, 0}
drum_gen_cfg_t drum_gen_session[NKIT] = {DG_DEFAULT(0x21), DG_DEFAULT(0xBEEF)};
typedef char dg_cfg_size[(sizeof(drum_gen_cfg_t) == 16) ? 1 : -1];
typedef char dg_table_size[(sizeof(drum_gen_map) == 1200) ? 1 : -1];

static int dg_row(int kit, int ch) { return ch < 2 ? ch : kit ? 10 : 7; }
static unsigned dg_byte(unsigned percent) { return (percent * 255u + 50u) / 100u; }
static unsigned dg_random(drum_gen_cfg_t *g)
{
    unsigned r = g->rng ? g->rng : 0x21u;
    r = (r >> 1) ^ ((r & 1u) ? 0xB400u : 0u);
    g->rng = (uint16_t)r;
    return r >> 8;
}

void drum_gen_defaults(drum_gen_cfg_t *g, uint16_t seed)
{
    const drum_gen_cfg_t def = DG_DEFAULT(0x21);
    *g = def;
    g->rng = seed ? seed : 0x21;
}

void drum_gen_init(uint16_t seed)
{
    drum_gen_defaults(&drum_gen_session[0], seed);
    drum_gen_defaults(&drum_gen_session[1], (uint16_t)(seed ^ 0xBEEFu));
}

void drum_gen_normalize(drum_gen_cfg_t *g, int pattern_len)
{
    int ch, limit = pattern_len < 1 ? 1 : pattern_len > 32 ? 32 : pattern_len;
    if (g->mode > DG_EUCLID) g->mode = DG_MAP;
    if (g->chaos > 100) g->chaos = 100;
    for (ch = 0; ch < DG_CHANNELS; ch++) {
        if (g->fill[ch] > 100) g->fill[ch] = 100;
        if (!g->len[ch]) g->len[ch] = 1;
        if (g->len[ch] > limit) g->len[ch] = (uint8_t)limit;
        g->offset[ch] %= g->mode == DG_MAP ? 16 : g->len[ch];
    }
}

/* Original U8Mix uses 255 rather than 256 and truncates at each stage. */
static unsigned dg_mix(unsigned a, unsigned b, unsigned balance)
{
    return (a * (255u - balance) + b * balance) >> 8;
}

static unsigned dg_level(const drum_gen_cfg_t *g, int ch, int step)
{
    int x = g->x >> 6, y = g->y >> 6;
    unsigned xb = (g->x << 2) & 255u, yb = (g->y << 2) & 255u;
    return dg_mix(dg_mix(drum_gen_map[x][y][ch][step], drum_gen_map[x + 1][y][ch][step], xb),
                  dg_mix(drum_gen_map[x][y + 1][ch][step], drum_gen_map[x + 1][y + 1][ch][step], xb), yb);
}

static unsigned dg_perturbed(unsigned level, unsigned perturb)
{
    return level < 255u - perturb ? level + perturb : 255u;
}

static void dg_euclid(const drum_gen_cfg_t *g, int ch, int length, uint32_t *hit, uint32_t *accent)
{
    int s;
    unsigned cycle = g->len[ch], count = (cycle * g->fill[ch] + 50u) / 100u;
    for (s = 0; s < NSW; s++) hit[s] = accent[s] = 0;
    for (s = 0; s < length; s++) {
        unsigned source = ((unsigned)s + cycle - g->offset[ch]) % cycle;
        if ((source * count) % cycle < count) {
            sm_set(hit, s);
            if (!source) sm_set(accent, s);
        }
    }
}

static void dg_begin(const dpart_t *d, const drum_gen_cfg_t *g, int kit, drum_gen_edit_t *e)
{
    int ch, w;
    e->cfg = *g;
    for (w = 0; w < NSW; w++) {
        e->accent[w] = d->accent[w];
        for (ch = 0; ch < DG_CHANNELS; ch++) e->hit[ch][w] = d->hit[dg_row(kit, ch)][w];
    }
}

static uint32_t dg_range(int length, int word)
{
    int n = length - word * 32;
    return n <= 0 ? 0 : n >= 32 ? ~0u : (1u << n) - 1u;
}

/* No accent provenance exists. Preserve every old accent coincident with
 * unrelated hits, even if generated earlier. Cleanup can only use old hits
 * in affected target rows/columns; it cannot identify orphaned old accents. */
static void dg_accents(const dpart_t *d, int kit, drum_gen_edit_t *e, const uint32_t *old,
                       const uint32_t *positions, const uint32_t *generated)
{
    int w, v;
    for (w = 0; w < NSW; w++) {
        uint32_t unrelated = 0;
        for (v = 0; v < NDRUM; v++)
            if (v != dg_row(kit, DG_KICK) && v != dg_row(kit, DG_SNARE) && v != dg_row(kit, DG_HAT))
                unrelated |= d->hit[v][w];
        e->accent[w] = (d->accent[w] & ~(old[w] & positions[w] & ~unrelated)) |
                       (generated[w] & positions[w]);
    }
}

static int dg_changed(const dpart_t *d, int kit, const drum_gen_edit_t *e)
{
    int ch, w;
    for (w = 0; w < NSW; w++) {
        if (d->accent[w] != e->accent[w]) return DG_CHANGED;
        for (ch = 0; ch < DG_CHANNELS; ch++)
            if (d->hit[dg_row(kit, ch)][w] != e->hit[ch][w]) return DG_CHANGED;
    }
    return DG_UNCHANGED;
}

static int dg_valid(const dpart_t *d, int kit) { return (unsigned)kit < NKIT && d->len >= 1 && d->len <= NSTEPS; }

int drum_gen_new(const dpart_t *d, const drum_gen_cfg_t *g, int kit, drum_gen_edit_t *e)
{
    uint32_t generated[NSW] = {0}, positions[NSW], old[NSW] = {0};
    int ch, w, s, bar;
    if (!dg_valid(d, kit)) return DG_INVALID;
    dg_begin(d, g, kit, e);
    drum_gen_normalize(&e->cfg, d->len);
    for (w = 0; w < NSW; w++) {
        positions[w] = dg_range(d->len, w);
        for (ch = 0; ch < DG_CHANNELS; ch++) {
            old[w] |= e->hit[ch][w];
            e->hit[ch][w] &= ~positions[w];
        }
    }
    if (e->cfg.mode == DG_EUCLID) {
        for (ch = 0; ch < DG_CHANNELS; ch++) {
            uint32_t hits[NSW], accent[NSW];
            dg_euclid(&e->cfg, ch, d->len, hits, accent);
            for (w = 0; w < NSW; w++) {
                e->hit[ch][w] |= hits[w];
                generated[w] |= accent[w];
            }
        }
    } else {
        for (bar = 0; bar < d->len; bar += 16) {
            for (ch = 0; ch < DG_CHANNELS; ch++) {
                unsigned perturb = e->cfg.chaos ?
                    (dg_random(&e->cfg) * (dg_byte(e->cfg.chaos) >> 2)) >> 8 : 0;
                unsigned threshold = 255u - dg_byte(e->cfg.fill[ch]);
                for (s = 0; s < 16; s++) {
                    int dest = bar + ((s + e->cfg.offset[ch]) & 15);
                    unsigned level = dg_perturbed(dg_level(&e->cfg, ch, s), perturb);
                    if (dest < d->len && level > threshold) {
                        sm_set(e->hit[ch], dest);
                        if (level > 192) sm_set(generated, dest);
                    }
                }
            }
        }
    }
    dg_accents(d, kit, e, old, positions, generated);
    return dg_changed(d, kit, e);
}

static int dg_euclid_mutate(const dpart_t *d, int kit, drum_gen_edit_t *e)
{
    uint32_t hits[NSW], accent[NSW], positions[NSW], keep[NSW] = {0};
    uint8_t candidates[4];
    int ch, direction, w, n = 0, pick;
    /* Check both rows first, including channels with no effective rotations. */
    for (ch = DG_SNARE; ch <= DG_HAT; ch++) {
        dg_euclid(&e->cfg, ch, d->len, hits, accent);
        for (w = 0; w < NSW; w++)
            if ((hits[w] ^ d->hit[dg_row(kit, ch)][w]) & dg_range(d->len, w)) return DG_NEW_FIRST;
        for (direction = 0; direction < 2; direction++) {
            drum_gen_cfg_t trial = e->cfg;
            trial.offset[ch] = (uint8_t)((trial.offset[ch] + trial.len[ch] + (direction ? 2 : -2)) % trial.len[ch]);
            dg_euclid(&trial, ch, d->len, hits, accent);
            for (w = 0; w < NSW; w++)
                if ((hits[w] ^ e->hit[ch][w]) & dg_range(d->len, w)) break;
            if (w != NSW) candidates[n++] = (uint8_t)((ch << 1) | direction);
        }
    }
    if (!n) return DG_UNCHANGED;
    pick = candidates[dg_random(&e->cfg) % (unsigned)n];
    ch = pick >> 1;
    e->cfg.offset[ch] = (uint8_t)((e->cfg.offset[ch] + e->cfg.len[ch] + ((pick & 1) ? 2 : -2)) % e->cfg.len[ch]);
    dg_euclid(&e->cfg, ch, d->len, hits, accent);
    for (w = 0; w < NSW; w++) {
        uint32_t range = dg_range(d->len, w);
        positions[w] = (e->hit[ch][w] | hits[w]) & range;
        e->hit[ch][w] = (e->hit[ch][w] & ~range) | hits[w];
    }
    /* Recompute the other generated accents without touching their hit rows.
     * In particular, never recreate Kick hits after manual edits/Undo. */
    for (direction = 0; direction < DG_CHANNELS; direction++) {
        if (direction == ch) continue;
        dg_euclid(&e->cfg, direction, d->len, hits, keep);
        for (w = 0; w < NSW; w++) accent[w] |= keep[w] & e->hit[direction][w];
    }
    dg_accents(d, kit, e, d->hit[dg_row(kit, ch)], positions, accent);
    return DG_CHANGED;
}

static int dg_map_mutate(const dpart_t *d, int kit, drum_gen_edit_t *e)
{
    uint32_t positions[NSW] = {0}, generated[NSW] = {0}, old[NSW] = {0};
    int changes = 0, attempts, budget = (e->cfg.chaos * 4 + 99) / 100;
    if (!budget) return DG_UNCHANGED;
    /* Bounded sampling, at most four flips, never the same column twice.
     * Only salience near FILL is eligible; >192 anchors are protected. */
    for (attempts = 0; attempts < 96 && changes < budget; attempts++) {
        int ch = (int)(dg_random(&e->cfg) % DG_CHANNELS);
        int s = (int)(dg_random(&e->cfg) % d->len);
        int source = (s + 16 - e->cfg.offset[ch]) & 15;
        unsigned level = dg_level(&e->cfg, ch, source);
        unsigned threshold = 255u - dg_byte(e->cfg.fill[ch]);
        int distance = (int)level - (int)threshold;
        if (!level || level > 192 || sm_get(positions, s) ||
            distance < -(int)e->cfg.chaos || distance > (int)e->cfg.chaos) continue;
        if (sm_get(e->hit[ch], s)) sm_set(old, s);
        sm_flip(e->hit[ch], s);
        sm_set(positions, s);
        changes++;
    }
    if (!changes) return DG_UNCHANGED;
    /* Only edited columns are cleaned. Keep strong accents of remaining
     * target hits there; shared/manual ownership cannot be recovered. */
    for (attempts = 0; attempts < d->len; attempts++) {
        int ch;
        if (!sm_get(positions, attempts)) continue;
        for (ch = 0; ch < DG_CHANNELS; ch++)
            if (sm_get(e->hit[ch], attempts) &&
                dg_level(&e->cfg, ch, (attempts + 16 - e->cfg.offset[ch]) & 15) > 192)
                sm_set(generated, attempts);
    }
    dg_accents(d, kit, e, old, positions, generated);
    return DG_CHANGED;
}

int drum_gen_mutate(const dpart_t *d, const drum_gen_cfg_t *g, int kit, drum_gen_edit_t *e)
{
    drum_gen_cfg_t normalized = *g;
    int ch, result;
    if (!dg_valid(d, kit)) return DG_INVALID;
    drum_gen_normalize(&normalized, d->len);
    /* P6.3 must normalize on length/mode changes before MUTATE. Mutation
     * itself never silently changes LEN/FILL or any unrelated OFFSET. */
    if (normalized.mode != g->mode || normalized.chaos != g->chaos) return DG_INVALID;
    for (ch = 0; ch < DG_CHANNELS; ch++)
        if (normalized.fill[ch] != g->fill[ch] || normalized.len[ch] != g->len[ch] ||
            normalized.offset[ch] != g->offset[ch]) return DG_INVALID;
    dg_begin(d, g, kit, e);
    result = g->mode == DG_EUCLID ? dg_euclid_mutate(d, kit, e) : dg_map_mutate(d, kit, e);
    if (result != DG_CHANGED) e->cfg = *g;
    return result;
}

void drum_gen_apply(dpart_t *d, drum_gen_cfg_t *g, int kit, const drum_gen_edit_t *e)
{
    int ch, w;
    for (w = 0; w < NSW; w++) {
        for (ch = 0; ch < DG_CHANNELS; ch++) d->hit[dg_row(kit, ch)][w] = e->hit[ch][w];
        d->accent[w] = e->accent[w];
    }
    *g = e->cfg;
}
