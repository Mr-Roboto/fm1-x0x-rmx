/* SPDX-License-Identifier: GPL-3.0-only */
/* Host-only original table/arithmetic comparisons and offline edit invariants.
 * Include the core to test MAP interpolation without exporting test APIs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/drum_gen.c"
#include "../../firmware/src/app/x0x.h"
#include "../../firmware/src/dsp/drum909.h"
#include "../../firmware/src/dsp/drum808.h"
#include "x0x_grids_reference.h"

static const int lengths[] = {1, 8, 12, 16, 24, 32, 48, 64};
/* Original PatternGenerator::drum_map, independent of the preprocessing code. */
static const uint8_t layout[5][5] = {
    {10, 8, 0, 9, 11}, {15, 7, 13, 12, 6}, {18, 14, 4, 5, 3},
    {23, 16, 21, 1, 2}, {24, 19, 17, 20, 22}
};

static unsigned ref_mix(unsigned a, unsigned b, uint8_t balance)
{
    return ((255u - balance) * a + balance * b) / 256u;
}

static unsigned ref_level(int x, int y, int ch, int step)
{
    int i = x / 64, j = y / 64, pos = ch * 32 + step * 2;
    return ref_mix(ref_mix(dg_ref_node[layout[i][j]][pos], dg_ref_node[layout[i + 1][j]][pos], (uint8_t)(x * 4)),
                   ref_mix(dg_ref_node[layout[i][j + 1]][pos], dg_ref_node[layout[i + 1][j + 1]][pos], (uint8_t)(x * 4)),
                   (uint8_t)(y * 4));
}

static int count_bits(uint32_t mask)
{
    int count = 0;
    while (mask) { count += mask & 1u; mask >>= 1; }
    return count;
}

static uint32_t rotate(uint32_t mask, int offset, int length)
{
    uint32_t result = 0;
    int i;
    for (i = 0; i < length; i++)
        if ((mask >> i) & 1u) result |= 1u << ((i + offset) % length);
    return result;
}

static void apply_new(dpart_t *d, drum_gen_cfg_t *g, int kit)
{
    drum_gen_edit_t edit;
    dpart_t before = *d;
    drum_gen_cfg_t config = *g;
    int result = drum_gen_new(d, g, kit, &edit);
    assert(result == DG_CHANGED || result == DG_UNCHANGED);
    assert(!memcmp(d, &before, sizeof before)); /* preparation is read-only */
    assert(!memcmp(g, &config, sizeof config));
    drum_gen_apply(d, g, kit, &edit);
    assert((memcmp(d, &before, sizeof before) != 0) == (result == DG_CHANGED));
}

static void assert_preserved(const dpart_t *before, const dpart_t *after, int kit)
{
    int v, s;
    assert(before->len == after->len && before->rate == after->rate);
    assert(!memcmp(before->rsv, after->rsv, sizeof before->rsv));
    for (v = 0; v < NDRUM; v++) {
        if (v != 0 && v != 1 && v != (kit ? 10 : 7))
            assert(!memcmp(before->hit[v], after->hit[v], sizeof before->hit[v]));
        for (s = before->len; s < NSTEPS; s++)
            assert(sm_get(before->hit[v], s) == sm_get(after->hit[v], s));
    }
    for (s = 0; s < NSTEPS; s++) {
        int unrelated = 0;
        for (v = 0; v < NDRUM; v++)
            if (v != 0 && v != 1 && v != (kit ? 10 : 7)) unrelated |= sm_get(before->hit[v], s);
        if (s >= before->len) assert(sm_get(before->accent, s) == sm_get(after->accent, s));
        if (unrelated && sm_get(before->accent, s)) assert(sm_get(after->accent, s));
    }
}

static void test_state(void)
{
    drum_gen_cfg_t g, keep;
    int mode, length, ch;
    assert(sizeof(drum_gen_map) == 1200 && sizeof(drum_gen_cfg_t) == 16);
    assert(sizeof(drum_gen_edit_t) == 48);
    assert(sizeof(dpart_t) == 100 && sizeof(bpart_t) == 144);
    assert(sizeof(pattern_t) == 528 && sizeof(project_t) == 30944);
    assert(DR_BD == 0 && DR_SD == 1 && DR_CH == 7);
    assert(D8_BD == 0 && D8_SD == 1 && D8_CH == 10);
    drum_gen_init(0x1234);
    keep = drum_gen_session[1];
    drum_gen_session[0].x++;
    assert(!memcmp(&keep, &drum_gen_session[1], sizeof keep));
    drum_gen_init(0x1234);
    assert(drum_gen_session[0].x == 128 && drum_gen_session[0].rng == 0x1234);
    for (mode = DG_MAP; mode <= DG_EUCLID; mode++) {
        drum_gen_defaults(&g, 0);
        assert(g.offset[0] == 0 && g.offset[1] == 4 && g.offset[2] == 2 && g.rng);
        for (length = 1; length <= 64; length++) {
            g.mode = (uint8_t)mode;
            for (ch = 0; ch < DG_CHANNELS; ch++) g.len[ch] = 255, g.offset[ch] = 255, g.fill[ch] = 255;
            g.chaos = 255;
            drum_gen_normalize(&g, length);
            for (ch = 0; ch < DG_CHANNELS; ch++) {
                assert(g.len[ch] == (length < 32 ? length : 32));
                assert(g.offset[ch] < (mode == DG_MAP ? 16 : g.len[ch]));
                assert(g.fill[ch] == 100);
            }
            assert(g.chaos == 100);
        }
    }
    puts("drum generator: transient defaults, bounds, independent machines and persistent sizes passed");
}

static void test_map_reference(void)
{
    drum_gen_cfg_t g;
    int x, y, ch, step, fill, chaos;
    drum_gen_defaults(&g, 0x21);
    for (x = 0; x < 256; x++)
        for (y = 0; y < 256; y++) {
            g.x = (uint8_t)x; g.y = (uint8_t)y;
            for (ch = 0; ch < DG_CHANNELS; ch++)
                for (step = 0; step < 16; step++)
                    assert(dg_level(&g, ch, step) == ref_level(x, y, ch, step));
        }
    for (x = 0; x < 256; x += 51)
        for (y = 0; y < 256; y += 51)
            for (fill = 0; fill <= 100; fill += 25)
                for (chaos = 0; chaos <= 100; chaos += 50) {
                    dpart_t d = {0};
                    drum_gen_edit_t edit;
                    uint16_t rng = 0x21;
                    uint32_t expected_hit[3] = {0}, expected_accent = 0;
                    g.x = (uint8_t)x; g.y = (uint8_t)y; g.chaos = (uint8_t)chaos; g.rng = rng;
                    d.len = 16;
                    for (ch = 0; ch < DG_CHANNELS; ch++) {
                        unsigned perturb = 0, threshold = 255u - (fill * 255u + 50u) / 100u;
                        g.fill[ch] = (uint8_t)fill;
                        if (chaos) {
                            rng = (uint16_t)((rng >> 1) ^ ((rng & 1u) ? 0xB400u : 0u));
                            perturb = ((rng >> 8) * (((chaos * 255u + 50u) / 100u) >> 2)) / 256u;
                        }
                        for (step = 0; step < 16; step++) {
                            unsigned level = ref_level(x, y, ch, step) + perturb;
                            int destination = (step + g.offset[ch]) % 16;
                            if (level > 255) level = 255;
                            if (level > threshold) {
                                expected_hit[ch] |= 1u << destination;
                                if (level > 192) expected_accent |= 1u << destination;
                            }
                        }
                    }
                    assert(drum_gen_new(&d, &g, 0, &edit) != DG_INVALID);
                    for (ch = 0; ch < DG_CHANNELS; ch++) assert(edit.hit[ch][0] == expected_hit[ch]);
                    assert(edit.accent[0] == expected_accent && edit.cfg.rng == rng);
                }
    puts("drum generator: all 3,145,728 MAP interpolation cases and 540 trigger/accent cases passed");
}

static void test_euclid(void)
{
    drum_gen_cfg_t g;
    int length, fill, offset, ch, s, density, exact = 0, rotations = 0;
    drum_gen_defaults(&g, 1);
    g.mode = DG_EUCLID;
    for (length = 1; length <= 32; length++)
        for (fill = 0; fill <= 100; fill++) {
            uint32_t canonical = 0;
            int count = (length * fill + 50) / 100;
            dpart_t d = {0};
            d.len = (uint8_t)length;
            for (ch = 0; ch < DG_CHANNELS; ch++) g.len[ch] = (uint8_t)length, g.fill[ch] = (uint8_t)fill;
            for (offset = 0; offset < length; offset++) {
                drum_gen_edit_t edit;
                for (ch = 0; ch < DG_CHANNELS; ch++) g.offset[ch] = (uint8_t)offset;
                drum_gen_new(&d, &g, 0, &edit);
                if (!offset) canonical = edit.hit[0][0];
                for (ch = 0; ch < DG_CHANNELS; ch++) {
                    assert(count_bits(edit.hit[ch][0]) == count);
                    assert(edit.hit[ch][0] == rotate(canonical, offset, length));
                    assert(!edit.hit[ch][1]);
                }
                assert(edit.accent[0] == (count ? 1u << offset : 0));
                assert(!edit.accent[1]);
            }
            if (count) {
                int previous = -1, first = -1;
                for (s = 0; s < length; s++) if ((canonical >> s) & 1u) {
                    if (previous >= 0) assert(s - previous == length / count || s - previous == (length + count - 1) / count);
                    else first = s;
                    previous = s;
                }
                assert(first + length - previous == length / count || first + length - previous == (length + count - 1) / count);
            }
        }
    /* Original density chooses hit count differently. Compare shape at its
     * actual count, not equal knob values; allow the documented phase change. */
    for (length = 1; length <= 32; length++)
        for (density = 0; density < 32; density++) {
            uint32_t reference = dg_ref_euclid[(length - 1) * 32 + density], mask;
            uint32_t generated[NSW], accents[NSW];
            int count = count_bits(reference), same = 0;
            g.len[0] = (uint8_t)length;
            g.fill[0] = (uint8_t)((count * 100 + length / 2) / length);
            g.offset[0] = 0;
            dg_euclid(&g, 0, length, generated, accents);
            mask = generated[0];
            assert(count_bits(mask) == count);
            for (offset = 0; offset < length; offset++) if (rotate(mask, offset, length) == reference) same = 1;
            assert(same);
            if (mask == reference) exact++; else rotations++;
        }
    assert(exact == 646 && rotations == 378);
    assert(count_bits(dg_ref_euclid[31 * 32 + 16]) == 17); /* new 32/50% is 16 */
    puts("drum generator: EUCLID all LEN/FILL/OFFSET, exact counts/gaps; reference 646 exact + 378 rotations passed");
}

static void test_patterns(void)
{
    int mode, kit, index, ch, s;
    for (mode = DG_MAP; mode <= DG_EUCLID; mode++)
        for (kit = 0; kit < NKIT; kit++)
            for (index = 0; index < (int)(sizeof lengths / sizeof lengths[0]); index++) {
                pattern_t pattern, before;
                drum_gen_cfg_t g;
                dpart_t *d = &pattern.drum[kit];
                memset(&pattern, 0xA5, sizeof pattern);
                d->len = (uint8_t)lengths[index];
                before = pattern;
                drum_gen_defaults(&g, 0x4567);
                g.mode = (uint8_t)mode;
                g.len[0] = 16; g.len[1] = 12; g.len[2] = 8;
                drum_gen_normalize(&g, d->len);
                apply_new(d, &g, kit);
                assert_preserved(&before.drum[kit], d, kit);
                before.drum[kit] = *d;
                assert(!memcmp(&before, &pattern, sizeof before));
                if (mode == DG_MAP) {
                    for (ch = 0; ch < DG_CHANNELS; ch++)
                        for (s = 16; s < d->len; s++)
                            assert(sm_get(d->hit[dg_row(kit, ch)], s) == sm_get(d->hit[dg_row(kit, ch)], s % 16));
                } else {
                    for (ch = 0; ch < DG_CHANNELS; ch++)
                        for (s = g.len[ch]; s < d->len; s++)
                            assert(sm_get(d->hit[dg_row(kit, ch)], s) == sm_get(d->hit[dg_row(kit, ch)], s % g.len[ch]));
                }
                {
                    drum_gen_edit_t edit;
                    dpart_t saved = *d;
                    assert(drum_gen_new(d, &g, kit, &edit) == DG_UNCHANGED);
                    drum_gen_apply(d, &g, kit, &edit);
                    assert(!memcmp(&saved, d, sizeof saved));
                }
            }
    puts("drum generator: both kits/modes, eight lengths, partial bars, hidden steps, unrelated parts and deterministic NEW passed");
}

static void test_chaos_mutate(void)
{
    int seed, kit, index, changed_bars = 0, changed_mutations = 0;
    for (kit = 0; kit < NKIT; kit++)
        for (index = 0; index < (int)(sizeof lengths / sizeof lengths[0]); index++)
            for (seed = 1; seed <= 100; seed++) {
                dpart_t d = {0}, before, other;
                drum_gen_cfg_t g, config, repeat;
                drum_gen_edit_t edit, second;
                int ch, s, flips = 0, result;
                d.len = (uint8_t)lengths[index];
                drum_gen_defaults(&g, (uint16_t)seed);
                apply_new(&d, &g, kit);
                before = d; config = g;
                assert(drum_gen_mutate(&d, &g, kit, &edit) == DG_UNCHANGED);
                assert(!memcmp(&d, &before, sizeof d) && !memcmp(&g, &config, sizeof g));
                g.chaos = 100;
                config = g;
                drum_gen_new(&d, &g, kit, &edit);
                drum_gen_new(&d, &g, kit, &second);
                assert(!memcmp(&edit, &second, sizeof edit));
                drum_gen_apply(&d, &g, kit, &edit);
                if (d.len >= 32)
                    for (ch = 0; ch < DG_CHANNELS; ch++)
                        if ((d.hit[dg_row(kit, ch)][0] & 65535u) != (d.hit[dg_row(kit, ch)][0] >> 16)) changed_bars++;
                assert_preserved(&before, &d, kit);
                before = d; config = g; repeat = g;
                result = drum_gen_mutate(&d, &g, kit, &edit);
                assert(result == drum_gen_mutate(&d, &repeat, kit, &second));
                assert(!memcmp(&edit, &second, sizeof edit));
                assert(!memcmp(&d, &before, sizeof d) && !memcmp(&g, &config, sizeof g));
                if (result == DG_CHANGED) {
                    drum_gen_apply(&d, &g, kit, &edit);
                    changed_mutations++;
                    assert_preserved(&before, &d, kit);
                    for (ch = 0; ch < DG_CHANNELS; ch++)
                        for (s = 0; s < d.len; s++) {
                            int row = dg_row(kit, ch);
                            if (sm_get(before.hit[row], s) != sm_get(d.hit[row], s)) {
                                flips++;
                                assert(ref_level(config.x, config.y, ch, (s + 16 - config.offset[ch]) % 16) <= 192);
                            }
                        }
                    assert(flips >= 1 && flips <= 4);
                }
                other = before;
                for (ch = 0; ch < NDRUM; ch++)
                    if (ch != 0 && ch != 1 && ch != (kit ? 10 : 7)) other.hit[ch][0] = other.hit[ch][1] = ~0u;
                other.accent[0] = other.accent[1] = ~0u;
                config = repeat;
                result = drum_gen_mutate(&other, &repeat, kit, &edit);
                if (result == DG_CHANGED) {
                    before = other;
                    drum_gen_apply(&other, &repeat, kit, &edit);
                    assert_preserved(&before, &other, kit);
                    assert(other.accent[0] == ~0u && other.accent[1] == ~0u);
                }
            }
    assert(changed_bars && changed_mutations);
    puts("drum generator: seeded MAP bar variation, zero-CHAOS no-op, bounded mutations and anchor protection passed");
}

static void test_accents(void)
{
    int mode, kit;
    for (mode = DG_MAP; mode <= DG_EUCLID; mode++)
        for (kit = 0; kit < NKIT; kit++) {
            dpart_t d = {0};
            drum_gen_cfg_t g;
            d.len = 16;
            d.hit[0][0] = 7;          /* old target at 0,1,2 */
            d.hit[2][0] = 2;          /* unrelated at 1: its accent must survive */
            d.accent[0] = 15;         /* column 3 has no old target: preserve */
            d.accent[1] = 123;
            drum_gen_defaults(&g, 1); g.mode = (uint8_t)mode;
            g.fill[0] = g.fill[1] = g.fill[2] = 0;
            apply_new(&d, &g, kit);
            assert(d.accent[0] == 10 && d.accent[1] == 123);
            g.fill[0] = g.fill[1] = g.fill[2] = 100;
            apply_new(&d, &g, kit);
            g.fill[0] = g.fill[1] = g.fill[2] = 0;
            apply_new(&d, &g, kit);
            /* The original orphan at 3 may become a generated target column
             * and thus lose its accent later; unrelated column 1 is protected. */
            assert((d.accent[0] & 2u) && !(d.accent[0] & ~10u));
        }
    puts("drum generator: unrelated accents, cleanup, hidden accents and repeated NEW passed");
}

static void test_euclid_mutate(void)
{
    int kit, length, fill, seed, seen[2][2] = {{0}};
    for (kit = 0; kit < NKIT; kit++)
        for (length = 1; length <= 32; length++)
            for (fill = 0; fill <= 100; fill += 10)
                for (seed = 1; seed <= 40; seed++) {
                    dpart_t d = {0}, before;
                    drum_gen_cfg_t g, config;
                    drum_gen_edit_t edit;
                    int ch, w, effective = 0, result, affected = -1;
                    d.len = 64;
                    drum_gen_defaults(&g, (uint16_t)(seed * 317)); g.mode = DG_EUCLID;
                    for (ch = 0; ch < DG_CHANNELS; ch++) g.len[ch] = (uint8_t)length, g.fill[ch] = (uint8_t)fill;
                    drum_gen_normalize(&g, d.len);
                    /* Shared unrelated accents are protected even during mutation. */
                    d.hit[2][0] = d.hit[2][1] = 0xA55AA55Au;
                    d.accent[0] = d.accent[1] = 0xA55AA55Au;
                    apply_new(&d, &g, kit);
                    before = d; config = g;
                    for (ch = DG_SNARE; ch <= DG_HAT; ch++) {
                        uint32_t cycle = d.hit[dg_row(kit, ch)][0] & dg_range(length, 0);
                        effective |= rotate(cycle, 2 % length, length) != cycle;
                    }
                    result = drum_gen_mutate(&d, &g, kit, &edit);
                    assert(result == (effective ? DG_CHANGED : DG_UNCHANGED));
                    assert(!memcmp(&d, &before, sizeof d) && !memcmp(&g, &config, sizeof g));
                    if (result == DG_UNCHANGED) continue;
                    drum_gen_apply(&d, &g, kit, &edit);
                    assert_preserved(&before, &d, kit);
                    assert(!memcmp(before.hit[0], d.hit[0], sizeof d.hit[0]));
                    assert(g.offset[0] == config.offset[0]);
                    assert(!memcmp(g.fill, config.fill, sizeof g.fill) && !memcmp(g.len, config.len, sizeof g.len));
                    for (ch = DG_SNARE; ch <= DG_HAT; ch++)
                        if (memcmp(before.hit[dg_row(kit, ch)], d.hit[dg_row(kit, ch)], sizeof d.hit[0])) {
                            int delta = (g.offset[ch] + length - config.offset[ch]) % length;
                            assert(affected == -1); affected = ch;
                            assert(delta == 2 % length || delta == (length - 2 % length) % length);
                            seen[ch - 1][delta == 2 % length]++;
                            assert(count_bits(d.hit[dg_row(kit, ch)][0] & dg_range(length, 0)) == (length * fill + 50) / 100);
                            assert(sm_get(d.accent, g.offset[ch]));
                        } else assert(g.offset[ch] == config.offset[ch]);
                    assert(affected != -1);
                    /* An unchanged channel's erased manual accent outside the
                     * mutated rhythm must not be silently reconstructed. */
                    for (w = 0; w < NSW; w++) {
                        uint32_t touched = before.hit[dg_row(kit, affected)][w] | d.hit[dg_row(kit, affected)][w];
                        assert(((before.accent[w] ^ d.accent[w]) & ~touched) == 0);
                    }
                    assert(drum_gen_new(&d, &g, kit, &edit) == DG_UNCHANGED);
                }
    assert(seen[0][0] && seen[0][1] && seen[1][0] && seen[1][1]);
    for (kit = 0; kit < NKIT; kit++) {
        drum_gen_cfg_t g, before_cfg;
        drum_gen_edit_t edit;
        dpart_t d = {0}, before;
        int ch;
        d.len = 32;
        drum_gen_defaults(&g, 7); g.mode = DG_EUCLID;
        g.len[0] = 16; g.len[1] = 12; g.len[2] = 8;
        apply_new(&d, &g, kit);
        /* Kick may have manual edits; mutation must preserve them. */
        sm_flip(d.hit[0], 5);
        before = d; before_cfg = g;
        assert(drum_gen_mutate(&d, &g, kit, &edit) == DG_CHANGED);
        drum_gen_apply(&d, &g, kit, &edit);
        assert(!memcmp(d.hit[0], before.hit[0], sizeof d.hit[0]));
        assert(g.offset[0] == before_cfg.offset[0]);
        for (ch = DG_SNARE; ch <= DG_HAT; ch++) {
            apply_new(&d, &g, kit);
            sm_flip(d.hit[dg_row(kit, ch)], 4);
            before = d; before_cfg = g;
            assert(drum_gen_mutate(&d, &g, kit, &edit) == DG_NEW_FIRST);
            assert(!memcmp(&before, &d, sizeof d) && !memcmp(&before_cfg, &g, sizeof g));
        }
    }
    puts("drum generator: EUCLID eligible +/-2, both channels/directions, symmetry no-op, Kick protection and NEW FIRST passed");
}

static void test_mutation_edges(void)
{
    int kit, mode, index, strength, s, ch;
    for (kit = 0; kit < NKIT; kit++)
        for (mode = DG_MAP; mode <= DG_EUCLID; mode++)
            for (index = 0; index < (int)(sizeof lengths / sizeof lengths[0]); index++)
                for (strength = 1; strength <= 100; strength += 11) {
                    dpart_t d, before;
                    drum_gen_cfg_t g, config;
                    drum_gen_edit_t edit;
                    int result, flips = 0;
                    memset(&d, 0x5A, sizeof d);
                    d.len = (uint8_t)lengths[index];
                    drum_gen_defaults(&g, (uint16_t)(strength * 921));
                    g.mode = (uint8_t)mode;
                    g.len[0] = 16; g.len[1] = 12; g.len[2] = 8;
                    drum_gen_normalize(&g, d.len);
                    apply_new(&d, &g, kit);
                    g.chaos = (uint8_t)strength;
                    before = d; config = g;
                    result = drum_gen_mutate(&d, &g, kit, &edit);
                    assert(result == DG_CHANGED || result == DG_UNCHANGED);
                    if (result == DG_CHANGED) {
                        drum_gen_apply(&d, &g, kit, &edit);
                        assert_preserved(&before, &d, kit);
                        for (ch = 0; ch < DG_CHANNELS; ch++)
                            for (s = 0; s < d.len; s++)
                                flips += sm_get(d.hit[dg_row(kit, ch)], s) != sm_get(before.hit[dg_row(kit, ch)], s);
                        if (mode == DG_MAP) assert(flips > 0 && flips <= (strength * 4 + 99) / 100);
                        else assert(!memcmp(before.hit[0], d.hit[0], sizeof d.hit[0]));
                    } else assert(!memcmp(&edit.cfg, &config, sizeof config));
                }
    {
        dpart_t d = {0}, before;
        drum_gen_cfg_t g, config;
        drum_gen_edit_t edit;
        d.len = 16;
        drum_gen_defaults(&g, 123); g.mode = DG_EUCLID;
        apply_new(&d, &g, 0);
        before = d; config = g;
        assert(drum_gen_mutate(&d, &g, 0, &edit) == DG_CHANGED);
        drum_gen_apply(&d, &g, 0, &edit);
        d = before; /* Undo persistent bytes only: offsets remain current. */
        before = d; config = g;
        assert(drum_gen_mutate(&d, &g, 0, &edit) == DG_NEW_FIRST);
        assert(!memcmp(&before, &d, sizeof d) && !memcmp(&config, &g, sizeof g));
        /* A fully symmetric row still participates in NEW FIRST checking. */
        g.fill[1] = g.fill[2] = 0;
        apply_new(&d, &g, 0);
        sm_set(d.hit[1], 0);
        assert(drum_gen_mutate(&d, &g, 0, &edit) == DG_NEW_FIRST);
        d.len = 0;
        assert(drum_gen_new(&d, &g, 0, &edit) == DG_INVALID);
        d.len = 65;
        assert(drum_gen_mutate(&d, &g, 0, &edit) == DG_INVALID);
        d.len = 16;
        assert(drum_gen_new(&d, &g, -1, &edit) == DG_INVALID);
        assert(drum_gen_new(&d, &g, 2, &edit) == DG_INVALID);
        g.len[1] = 32;
        assert(drum_gen_mutate(&d, &g, 0, &edit) == DG_INVALID);
        /* NEW clamps session lengths; MUTATE never silently changes them. */
        assert(drum_gen_new(&d, &g, 0, &edit) == DG_CHANGED);
        assert(edit.cfg.len[1] == 16);
    }
    puts("drum generator: mutation lengths/hidden data, CHAOS budgets, Undo mismatch and invalid input passed");
}

int main(void)
{
    test_state();
    test_map_reference();
    test_euclid();
    test_patterns();
    test_chaos_mutate();
    test_accents();
    test_euclid_mutate();
    test_mutation_edges();
    puts("drum generator: all tests passed");
    return 0;
}
