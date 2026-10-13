/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise private const tables without adding a firmware API or RAM. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/tb3po.c"

static const scale_t expected[] = {
    {{0, 3, 5, 7, 10, 0, 0}, 5}, /* MPEN */
    {{0, 2, 4, 7, 9, 0, 0}, 5}, /* PMAJ */
    {{0, 2, 5, 7, 10, 0, 0}, 5}, /* EGYP */
    {{0, 2, 3, 7, 8, 0, 0}, 5}, /* HIRA */
    {{0, 2, 3, 5, 7, 8, 10}, 7}, /* MIN */
    {{0, 2, 4, 5, 7, 9, 11}, 7}, /* MAJ */
    {{0, 2, 3, 5, 7, 9, 10}, 7}, /* DOR */
    {{0, 1, 3, 5, 7, 8, 10}, 7}, /* PHRY */
    {{0, 2, 4, 6, 7, 9, 11}, 7}, /* LYD */
    {{0, 2, 4, 5, 7, 9, 10}, 7}, /* MIXO */
    {{0, 1, 3, 5, 6, 8, 10}, 7}, /* LOCR */
    {{0, 2, 3, 5, 7, 8, 11}, 7}, /* HARM */
    {{0, 2, 3, 5, 7, 9, 11}, 7}, /* MELM */
    {{0, 2, 4, 5, 7, 8, 11}, 7}, /* HMAJ */
    {{0, 1, 3, 4, 6, 8, 10}, 7}, /* ALTR */
    {{0, 2, 3, 6, 7, 8, 11}, 7}, /* GYPS */
    {{0, 2, 3, 6, 7, 9, 10}, 7}, /* ROMN */
    {{0, 1, 4, 5, 7, 8, 11}, 7}, /* BHAI */
    {{0, 1, 4, 6, 7, 9, 11}, 7}, /* MARV */
    {{0, 1, 4, 6, 7, 8, 11}, 7}, /* PURV */
    {{0, 1, 3, 6, 7, 8, 11}, 7}, /* TODI */
    {{0, 2, 4, 5, 6, 8, 10}, 7}, /* ARAB */
};
static const char *const names[] = {
    "MPEN", "PMAJ", "EGYP", "HIRA", "MIN", "MAJ", "DOR", "PHRY", "LYD", "MIXO", "LOCR", "HARM", "MELM", "HMAJ", "ALTR", "GYPS", "ROMN", "BHAI", "MARV", "PURV", "TODI", "ARAB"
};

static void check_notes(const bpart_t *b, int s, int root)
{
    for (int i = 0; i < b->len; i++) {
        if (bstep_gate(&b->step[i]) == G_REST) continue;
        int pc = (b->step[i].note - root) % 12, found = 0;
        for (int d = 0; d < expected[s].len; d++) found |= pc == expected[s].deg[d];
        assert(found);
    }
}

int main(void)
{
    unsigned masks[22] = {0};
    int pent = 0, seven = 0;
    assert(TB3PO_NSCALES == 22);
    tb3po_cfg_t defaults;
    tb3po_defaults(&defaults, 123);
    assert(defaults.scale == 0 && !strcmp(TB3PO_SCALE_NAMES[defaults.scale], "MPEN"));
    for (int s = 0; s < TB3PO_NSCALES; s++) {
        assert(!strcmp(TB3PO_SCALE_NAMES[s], names[s]));
        assert(SCALES[s].len == expected[s].len);
        assert(!memcmp(SCALES[s].deg, expected[s].deg, sizeof expected[s].deg));
        pent += SCALES[s].len == 5;
        seven += SCALES[s].len == 7;
        for (int d = 0; d < SCALES[s].len; d++) masks[s] |= 1u << SCALES[s].deg[d];
        for (int p = 0; p < s; p++) assert(masks[s] != masks[p]);
        for (int seed = 1; seed <= 32; seed++) {
            bpart_t c;
            memset(&c, 0, sizeof c);
            c.len = NSTEPS;
            tb3po_defaults(&c.gen, seed * 7919u);
            c.gen.scale = s;
            c.gen.root = 0;
            c.gen.density = 100;
            tb3po_generate(&c);
            for (int root = 0; root < 12; root++) {
                bpart_t b = c;
                b.gen.root = root;
                tb3po_generate(&b);
                check_notes(&b, s, root);
                for (int i = 0; i < NSTEPS; i++) {
                    assert(b.step[i].note == c.step[i].note + root);
                    assert(b.step[i].flags == c.step[i].flags);
                }
                bpart_t cm = c;
                uint32_t br = 0, cr = 0;
                for (int m = 0; m < 16; m++) {
                    tb3po_mutate(&b, &br);
                    tb3po_mutate(&cm, &cr);
                    assert(br == cr);
                    check_notes(&b, s, root);
                    for (int i = 0; i < NSTEPS; i++) {
                        assert(b.step[i].note == cm.step[i].note + root);
                        assert(b.step[i].flags == cm.step[i].flags);
                    }
                }
            }
        }
    }
    assert(pent == 4 && seven == 18);
    /* Invalid selections retain the existing MIN fallback without changing the stored byte. */
    bpart_t invalid = {0}, minor = {0};
    invalid.len = minor.len = 64;
    tb3po_defaults(&invalid.gen, 456);
    minor.gen = invalid.gen;
    minor.gen.scale = 4; /* MIN remains the fallback, independent of MPEN initialization. */
    invalid.gen.scale = 255;
    tb3po_generate(&invalid);
    tb3po_generate(&minor);
    assert(!memcmp(invalid.step, minor.step, sizeof minor.step));
    uint32_t ir = 0, mr = 0;
    tb3po_mutate(&invalid, &ir);
    tb3po_mutate(&minor, &mr);
    assert(ir == mr && invalid.gen.scale == 255);
    assert(!memcmp(invalid.step, minor.step, sizeof minor.step));

    puts("TB-3PO: 22 unique scales, 4 pentatonic/18 seven-note, exact tables, all roots, generate/mutate and repeatability passed");
    return 0;
}
