/* SPDX-License-Identifier: GPL-3.0-only */
/* TB-3PO generator; see tb3po.h. Integer and float maths only, no libm. */
#include "tb3po.h"

typedef struct { int8_t deg[7]; uint8_t len; } scale_t;

/* Preserve the historical invalid-index fallback independently of new defaults. */
#define TB3PO_FALLBACK_SCALE 4 /* Natural Minor (MIN) */

/* Display order; indices deliberately differ from schwung-tb3po. */
static const scale_t SCALES[TB3PO_NSCALES] = {
    {{0, 3, 5, 7, 10, 0, 0}, 5}, /* MinPent */
    {{0, 2, 4, 7, 9, 0, 0}, 5}, /* Major Pentatonic */
    {{0, 2, 5, 7, 10, 0, 0}, 5}, /* Egyptian Pentatonic */
    {{0, 2, 3, 7, 8, 0, 0}, 5}, /* Hirajoshi */
    {{0, 2, 3, 5, 7, 8, 10}, 7}, /* Minor */
    {{0, 2, 4, 5, 7, 9, 11}, 7}, /* Major */
    {{0, 2, 3, 5, 7, 9, 10}, 7}, /* Dorian */
    {{0, 1, 3, 5, 7, 8, 10}, 7}, /* Phrygian */
    {{0, 2, 4, 6, 7, 9, 11}, 7}, /* Lydian */
    {{0, 2, 4, 5, 7, 9, 10}, 7}, /* Mixolydian */
    {{0, 1, 3, 5, 6, 8, 10}, 7}, /* Locrian */
    {{0, 2, 3, 5, 7, 8, 11}, 7}, /* HarmMinor */
    {{0, 2, 3, 5, 7, 9, 11}, 7}, /* Melodic Minor */
    {{0, 2, 4, 5, 7, 8, 11}, 7}, /* Harmonic Major */
    {{0, 1, 3, 4, 6, 8, 10}, 7}, /* Altered */
    {{0, 2, 3, 6, 7, 8, 11}, 7}, /* Hungarian Minor */
    {{0, 2, 3, 6, 7, 9, 10}, 7}, /* Romanian Minor */
    {{0, 1, 4, 5, 7, 8, 11}, 7}, /* Bhairav Thaat */
    {{0, 1, 4, 6, 7, 9, 11}, 7}, /* Marva Thaat */
    {{0, 1, 4, 6, 7, 8, 11}, 7}, /* Purvi Thaat */
    {{0, 1, 3, 6, 7, 8, 11}, 7}, /* Todi Thaat */
    {{0, 2, 4, 5, 6, 8, 10}, 7}, /* Arabic */
};
const char *const TB3PO_SCALE_NAMES[TB3PO_NSCALES] = {
    "MPEN", "PMAJ", "EGYP", "HIRA",
    "MIN", "MAJ", "DOR", "PHRY", "LYD", "MIXO", "LOCR",
    "HARM", "MELM", "HMAJ", "ALTR", "GYPS",
    "ROMN", "BHAI", "MARV", "PURV", "TODI", "ARAB",
};

static uint32_t rng_u32(uint32_t *r)                 /* xorshift32, as schwung-tb3po */
{
    uint32_t x = *r;
    if (x == 0)
        x = 1;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *r = x;
    return x;
}

void tb3po_defaults(tb3po_cfg_t *g, uint32_t seed)
{
    g->density = 70;                                 /* schwung-tb3po: 0.7 / 0.4 / 0.25, 2 octaves; root A */
    g->accent = 40;
    g->slide = 25;
    g->oct_range = 2;
    g->root = 9;
    g->scale = TB3PO_DEFAULT_SCALE;
    g->base_oct = 1;                                 /* note 24 + root: schwung-tb3po's base */
    g->mutate_bars = 0;
    g->seed = seed ? seed : 0xBEEF;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static uint8_t note_of(const tb3po_cfg_t *g, int degree, int oct)
{
    const scale_t *sc = &SCALES[g->scale < TB3PO_NSCALES ? g->scale : TB3PO_FALLBACK_SCALE];
    return (uint8_t)clampi(12 * (g->base_oct + 1) + g->root + sc->deg[degree] + 12 * oct, 0, 127);
}

static int distance(int a, int b) { return a > b ? a - b : b - a; }

/* Use a fifth only if the scale contains it (Locrian/Altered/Arabic do not). */
static int preferred_degree(uint32_t *r, const scale_t *sc, int colour)
{
    int fifth = 0, d;
    for (d = 1; d < sc->len; d++)
        if (sc->deg[d] == 7) fifth = d;
    d = (int)(rng_u32(r) % 100u);
    if (d < 50) return 0;
    if (d < 85 - colour) return fifth;
    return (int)(rng_u32(r) % sc->len);
}

/* Pick the nearest register of the motif tone. If it would leap more than a
 * fifth, use a nearby scale tone instead. No clipping to an out-of-scale 127. */
static uint8_t voice_note(const tb3po_cfg_t *g, const scale_t *sc, int degree, int previous, int neighbour)
{
    int range = clampi(g->oct_range, 1, 3), best = -1, cost = 10000;
    int d, o;
    for (o = 0; o < range; o++) {
        for (d = 0; d < sc->len; d++) {
            int n = 12 * (g->base_oct + 1) + g->root + sc->deg[d] + 12 * o;
            int delta = distance(n, previous), c;
            if (n > 127) continue;
            c = delta + (d == degree ? 0 : 24) + (delta <= 7 && distance(n, neighbour) <= 7 ? 0 : 100);
            if (c < cost) { best = n; cost = c; }
        }
    }
    return best < 0 ? note_of(g, 0, 0) : (uint8_t)best;
}

static uint8_t articulation(uint32_t *r, const bpart_t *b, int i)
{
    const tb3po_cfg_t *g = &b->gen;
    int nx = (i + 1) % clampi(b->len, 1, NSTEPS);
    int accent = clampi(g->accent, 0, 100), slide = clampi(g->slide, 0, 100);
    uint8_t f = G_NOTE;
    /* Absolute beat grid, plus anticipations into beats; never phrase-relative. */
    if (accent < 100) {
        int weight = i % 4 == 0 ? 100 : i % 4 == 3 ? 80 : 55;
        accent = accent * weight / 100;
    }
    if ((int)(rng_u32(r) % 100u) < accent) f |= BS_ACCENT;
    if (slide < 100) {
        int delta = distance(b->step[i].note, b->step[nx].note);
        int root = (b->step[nx].note - g->root + 120) % 12 == 0;
        int weight = delta > 0 && delta <= 7 ? (nx % 4 == 0 || root ? 100 : 55) : 15;
        slide = slide * weight / 100;
        /* Leave space between gestures rather than chaining random legato. */
        if (i > 0 && (b->step[i - 1].flags & BS_SLIDE)) slide /= 4;
    }
    if ((int)(rng_u32(r) % 100u) < slide && nx != i &&
        bstep_gate(&b->step[nx]) == G_NOTE)
        f |= BS_SLIDE;
    return f;
}

static void clean_slides(bpart_t *b, int len)
{
    int i;
    for (i = 0; i < len; i++)
        if (bstep_gate(&b->step[(i + 1) % len]) == G_REST)
            b->step[i].flags &= (uint8_t)~BS_SLIDE;
}

void tb3po_generate(bpart_t *b)
{
    const tb3po_cfg_t *g = &b->gen;
    const scale_t *sc = &SCALES[g->scale < TB3PO_NSCALES ? g->scale : TB3PO_FALLBACK_SCALE];
    uint8_t melody[2][8], priority[2][8];
    int len = clampi(b->len, 1, NSTEPS), i, start, previous;
    uint32_t r = g->seed ? g->seed : 1;
    int range = clampi(g->oct_range, 1, 3), register_roll = (int)(rng_u32(&r) % 100u);
    int anchor = note_of(g, 0, range > 1 && register_roll < 25 ?
                         (range > 2 && register_roll < 5 ? 2 : 1) : 0);
    int motif_len = rng_u32(&r) % 2u ? 4 : 8;
    previous = anchor;
    for (int m = 0; m < 2; m++) {
        int offbeat = 2 + rng_u32(&r) % 2u;
        for (i = 0; i < 8; i++) {
            int degree = i % 4 == 0 ? 0 : preferred_degree(&r, sc, i == 7 ? 25 : 0);
            melody[m][i] = i >= motif_len ? melody[m][i % motif_len] :
                voice_note(g, sc, degree, i ? melody[m][i - 1] : anchor, anchor);
            /* Seeded beat/offbeat motif with deliberate low-priority gaps. */
            int weight = i == 0 ? 0 : i == 4 ? 24 :
                         i % 4 == offbeat ? 40 : i % 4 == 1 ? 96 : 72;
            priority[m][i] = weight + rng_u32(&r) % 16u;
        }
        melody[m][0] = anchor;
    }
    for (i = 0; i < NSTEPS; i++) {
        b->step[i].flags = G_REST;
        b->step[i].note = note_of(g, 0, 0);
    }
    for (start = 0; start < len; start += 8) {
        int n = len - start < 8 ? len - start : 8;
        int m = (start / 16) % 2, answer = (start / 8) % 2;
        int count = (n * clampi(g->density, 0, 100) + 50) / 100;
        uint8_t rhythm[8];
        for (i = 0; i < 8; i++) rhythm[i] = priority[m][i];
        /* The answer exchanges two weak positions, retaining beat anchors.
         * Later repetitions develop that answer without moving the beat grid. */
        if (answer || (start >= 32 && rng_u32(&r) % 2u)) {
            int p = 6 + rng_u32(&r) % 2u;
            uint8_t tmp = rhythm[p];
            rhythm[p] = rhythm[5];
            rhythm[5] = tmp;
        }
        /* Preserve the existing playable root even at DENS 0. */
        if (start == 0 && count == 0) count = 1;
        for (i = 0; i < n; i++) {
            int j, rank = 0;
            for (j = 0; j < n; j++)
                rank += rhythm[j] < rhythm[i] || (rhythm[j] == rhythm[i] && j < i);
            if (rank >= count) continue;
            int note = melody[m][i];
            if (answer && i >= 6)
                note = voice_note(g, sc, i == 7 ? 0 : preferred_degree(&r, sc, 25), previous, anchor);
            /* Each short motif connects to its predecessor and the loop anchor. */
            if (distance(note, previous) > 7)
                note = voice_note(g, sc, 0, previous, anchor);
            b->step[start + i].note = note;
            b->step[start + i].flags = G_NOTE;
            previous = note;
        }
    }
    for (i = 0; i < len; i++)
        if (bstep_gate(&b->step[i]) != G_REST)
            b->step[i].flags = articulation(&r, b, i);
}

void tb3po_mutate(bpart_t *b, uint32_t *rng)
{
    const tb3po_cfg_t *g = &b->gen;
    const scale_t *sc = &SCALES[g->scale < TB3PO_NSCALES ? g->scale : TB3PO_FALLBACK_SCALE];
    int len = clampi(b->len, 1, NSTEPS), edits = (len + 7) / 8, k;
    uint32_t r = *rng ? *rng : (g->seed ? g->seed : 1);
    for (k = 0; k < edits; k++) {
        int start = k * 8, nsteps = len - start < 8 ? len - start : 8;
        int i = start + (int)(rng_u32(&r) % nsteps), prev, next, j;
        int action = (int)(rng_u32(&r) % 100u);
        uint8_t n;
        /* Articulation may vary on anchors, but their pitch and rhythm stay. */
        if (i % 4 < 2 || (action >= 45 && action < 90)) {
            if (bstep_gate(&b->step[i]) == G_NOTE)
                b->step[i].flags = articulation(&r, b, i);
            continue;
        }
        prev = next = i;
        for (j = 1; j < len; j++) {
            int p = (i + len - j) % len;
            if (bstep_gate(&b->step[p]) != G_REST) { prev = p; break; }
        }
        for (j = 1; j < len; j++) {
            int p = (i + j) % len;
            if (bstep_gate(&b->step[p]) != G_REST) { next = p; break; }
        }
        /* Rhythm additions echo a neighbour; pitch edits are rare scale steps. */
        int degree = preferred_degree(&r, sc, 25);
        if (action < 45) {
            for (j = 0; j < sc->len; j++)
                if ((b->step[prev].note - g->root + 120) % 12 == sc->deg[j]) degree = j;
        }
        n = voice_note(g, sc, degree, b->step[prev].note, b->step[next].note);
        if (distance(n, b->step[prev].note) > 7 || distance(n, b->step[next].note) > 7)
            continue;
        if (action < 45) {
            if ((int)(rng_u32(&r) % 100u) < clampi(g->density, 0, 100)) {
                if (bstep_gate(&b->step[i]) == G_REST) {
                    b->step[i].note = n;
                    b->step[i].flags = G_NOTE;
                }
            } else if (prev != i && distance(b->step[prev].note, b->step[next].note) <= 7) {
                b->step[i].flags = G_REST;
            }
        } else if (bstep_gate(&b->step[i]) == G_NOTE && distance(n, b->step[i].note) <= 3) {
            b->step[i].note = n;
        }
        if (bstep_gate(&b->step[i]) == G_NOTE)
            b->step[i].flags = articulation(&r, b, i);
    }
    clean_slides(b, len);
    *rng = r;
}

uint32_t tb3po_new_seed(uint32_t entropy)
{
    uint32_t r = entropy * 2654435761u + 0x9E3779B9u;
    rng_u32(&r);
    rng_u32(&r);
    return r ? r : 1;
}
