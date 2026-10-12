/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the actual app copy/clear and project stream paths on simulated flash. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

static void expect_remix(const pattern_t *p, int part, int selection, int enabled)
{
    assert(pattern_remix_selection(p, part) == selection);
    assert(pattern_remix_enabled(p, part) == enabled);
}

static void expect_defaults(void)
{
    for (int n = 0; n < NPAT; n++)
        for (int t = 0; t < 4; t++)
            expect_remix(&proj.pat[n], t, 0, 0);
}

int main(void)
{
    pattern_t p, before;
    assert(sizeof(dpart_t) == 100);
    assert(sizeof(bpart_t) == 144);
    assert(sizeof(pattern_t) == 528);
    assert(sizeof(project_t) == 30944);
    assert(RAW_LEN == 29572);
    project_defaults();
    seq_init(&seq, proj.pat);
    expect_defaults();
    for (int t = 0; t < 4; t++) {
        for (int value = 0; value < 256; value++) {
            pattern_init(&p, 1, 2);
            pattern_set_remix_selection(&p, t, value >> 4, value & 15);
            pattern_set_remix_enabled(&p, t, 1);
            pattern_set_remix_selection(&p, t, value >> 4, value & 15);
            expect_remix(&p, t, value, 1);
            for (int on = 0; on < 2; on++) {
                pattern_set_remix_enabled(&p, t, on);
                expect_remix(&p, t, value, on);
                for (int other = 0; other < 4; other++)
                    if (other != t)
                        expect_remix(&p, other, 0, 0);
            }
        }
        for (int marker = 0; marker < 256; marker++) {
            memset(&p, 0xFF, sizeof p);
            if (t < NKIT)
                p.drum[t].rsv[1] = marker;
            else
                p.rsv[2] = marker;
            int valid = (marker & (t < NKIT ? 0xFE : 0xFC)) == 0xA4;
            expect_remix(&p, t, valid ? 255 : 0, valid ? (marker >> (t < NKIT ? 0 : t - NKIT)) & 1 : 0);
            if (!valid) {
                pattern_set_remix_enabled(&p, t, 1);
                expect_remix(&p, t, 0, 1);
                if (t >= NKIT)
                    expect_remix(&p, t == 2 ? 3 : 2, 0, 0);
            }
        }
    }
    pattern_init(&p, 1, 2);
    before = p;
    pattern_set_remix_selection(&p, -1, 0, 0);
    pattern_set_remix_selection(&p, 4, 0, 0);
    pattern_set_remix_selection(&p, 0, 16, 0);
    pattern_set_remix_selection(&p, 0, 0, -1);
    pattern_set_remix_enabled(&p, 4, 1);
    assert(!memcmp(&p, &before, sizeof p));

    for (int n = 0; n < NPAT; n++)
        for (int t = 0; t < 4; t++) {
            pattern_set_remix_selection(&proj.pat[n], t, n & 15, t + 5);
            pattern_set_remix_enabled(&proj.pat[n], t, (n + t) & 1);
        }
    assert(project_save() == 0);
    project_defaults();
    assert(project_load() == 0);
    for (int n = 0; n < NPAT; n++)
        for (int t = 0; t < 4; t++)
            expect_remix(&proj.pat[n], t, ((n & 15) << 4) | (t + 5), (n + t) & 1);

    for (int t = 0; t < 4; t++) {
        project_defaults();
        for (int k = 0; k < 4; k++) {
            pattern_set_remix_selection(&proj.pat[0], k, k + 1, k + 2);
            pattern_set_remix_enabled(&proj.pat[0], k, k & 1);
            pattern_set_remix_selection(&proj.pat[1], k, 15, 15);
            pattern_set_remix_enabled(&proj.pat[1], k, !(k & 1));
        }
        if (t < NKIT) {
            sm_set(proj.pat[0].drum[t].hit[0], 63);
            sm_set(proj.pat[0].drum[t].accent, 63);
            proj.pat[0].drum[t].len = 40;
            proj.pat[0].drum[t].rate = RATE_32;
        } else {
            proj.pat[0].bass[t - NKIT].step[63].flags = G_NOTE | BS_SLIDE;
            proj.pat[0].bass[t - NKIT].len = 40;
            proj.pat[0].bass[t - NKIT].rate = RATE_32;
            proj.pat[0].bass[t - NKIT].dir = DIR_REV;
        }
        assert(copy_part(1, 0, t) == 0);
        if (t < NKIT)
            assert(!memcmp(&proj.pat[1].drum[t], &proj.pat[0].drum[t], sizeof(dpart_t)));
        else
            assert(!memcmp(&proj.pat[1].bass[t - NKIT], &proj.pat[0].bass[t - NKIT], sizeof(bpart_t)));
        for (int k = 0; k < 4; k++)
            expect_remix(&proj.pat[1], k, k == t ? ((k + 1) << 4) | (k + 2) : 255,
                   k == t ? k & 1 : !(k & 1));
        before = proj.pat[1];
        clear_part(&proj.pat[1], t);
        if (t < NKIT) {
            assert(!sm_any(proj.pat[1].drum[t].hit[0]));
            assert(!sm_any(proj.pat[1].drum[t].accent));
            assert(proj.pat[1].drum[t].len == 40);
            assert(proj.pat[1].drum[t].rate == RATE_32);
        } else {
            assert(proj.pat[1].bass[t - NKIT].step[63].flags == G_REST);
            assert(proj.pat[1].bass[t - NKIT].len == 40);
            assert(proj.pat[1].bass[t - NKIT].rate == RATE_32);
            assert(proj.pat[1].bass[t - NKIT].dir == DIR_REV);
        }
        for (int k = 0; k < 4; k++)
            expect_remix(&proj.pat[1], k, pattern_remix_selection(&before, k), pattern_remix_enabled(&before, k));
        /* Legacy source copying decodes A1/OFF and preserves the sibling. */
        pattern_init(&proj.pat[0], 1, 2);
        assert(copy_part(1, 0, t) == 0);
        expect_remix(&proj.pat[1], t, 0, 0);
    }
    project_defaults();
    for (int n = 0; n < NPAT; n++) {
        memset(proj.pat[n].rsv, 0xFF, 3);
        for (int t = 0; t < NKIT; t++)
            memset(proj.pat[n].drum[t].rsv, 0xFF, 2);
    }
    assert(project_save() == 0);
    project_defaults();
    assert(project_load() == 0);
    expect_defaults();

    /* Actual historical format-1 flash fixture, through project_load(). */
    FILE *f = fopen("tests/fixtures/v1_store.bin", "rb");
    assert(f);
    for (int o = 0; o < OBJ_NOBJ; o++) {
        int32_t len;
        assert(fread(&len, 4, 1, f) == 1);
        assert(fread(store[o], 1, PLAT_STORE_MAX, f) == PLAT_STORE_MAX);
        store_len[o] = len;
    }
    fclose(f);
    assert(project_load() == 0);
    expect_defaults();
    /* Reserved bytes in format 1 must never activate Remix, even matching markers. */
    pattern1_t old;
    memset(&old, 0xA5, sizeof old);
    pattern_init(&p, 1, 2);
    pattern_from1(&p, &old);
    for (int t = 0; t < 4; t++)
        expect_remix(&p, t, 0, 0);
    puts("Remix state: defaults, selections, markers, independence, stream reload, legacy, copy/clear and sizes passed");
    return 0;
}
