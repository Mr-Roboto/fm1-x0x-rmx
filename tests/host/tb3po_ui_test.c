/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual scale page, selection labels, rendering and numeric storage round trips. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

int main(void)
{
    static const char *const names[] = {
        "MPEN", "PMAJ", "EGYP", "HIRA", "MIN", "MAJ", "DOR", "PHRY", "LYD", "MIXO", "LOCR",
        "HARM", "MELM", "HMAJ", "ALTR", "GYPS", "ROMN", "BHAI", "MARV", "PURV", "TODI", "ARAB"
    };
    project_defaults();
    seq_init(&seq, proj.pat);
    ui_init();
    assert(sizeof(tb3po_cfg_t) == 12 && sizeof(project_t) == 30944);
    for (int b = 0; b < 2; b++) {
        ui.part = PART_303A + b;
        ui.view = V_GEN;
        build_pages();
        assert(pg.n == 2 && !strcmp(pg.title[1], "SCALE"));
        pref_t r = pg.r[1][1];
        assert(r.kind == R_GEN && r.a == b && r.c == G_SCALE);
        assert(pref_desc(r)->max == 21 && pref_desc(r)->def == 0);
        assert(pref_get(r) == 0);
        *page_of(V_GEN) = 1;
        bstep_t before[NSTEPS];
        memcpy(before, cur_pat()->bass[b].step, sizeof before);
        for (int s = 0; s < 22; s++) {
            char num[24], unit[24];
            pref_set(r, s);
            assert(pref_get(r) == s && cur_pat()->bass[b].gen.scale == s);
            pref_value_of(r, pref_get(r), num, unit);
            assert(!strcmp(num, names[s]) && !unit[0]);
            assert(!memcmp(before, cur_pat()->bass[b].step, sizeof before));
            draw_header(); draw_main(); draw_knobs();
            if (s == 0 || s == 4 || s == 21) {
                char path[80];
                snprintf(path, sizeof path, "build/host/tb3po_%d_%s.png", b, names[s]);
                png_write(path);
            }
        }
        pref_set(r, -1); assert(pref_get(r) == 0);
        pref_set(r, 22); assert(pref_get(r) == 21);
    }
    /* Older numeric selections are retained verbatim, with the new meanings.
     * All 22 valid values survive save/load; already written note lines stay intact. */
    for (int s = 0; s < 22; s++) {
        proj.pat[0].bass[0].gen.scale = s;
        proj.pat[0].bass[1].gen.scale = 21 - s;
        proj.pat[0].bass[0].step[0].note = 65;
        proj.pat[0].bass[0].step[0].flags = G_NOTE | BS_ACCENT;
        assert(project_save() == 0);
        project_defaults();
        assert(proj.pat[0].bass[0].gen.scale == 0);
        assert(proj.pat[0].bass[1].gen.scale == 0);
        assert(project_load() == 0);
        assert(proj.pat[0].bass[0].gen.scale == s);
        assert(proj.pat[0].bass[1].gen.scale == 21 - s);
        assert(proj.pat[0].bass[0].step[0].note == 65);
        assert(proj.pat[0].bass[0].step[0].flags == (G_NOTE | BS_ACCENT));
    }
    puts("TB-3PO UI: both 303 scale pages, all 22 labels/selections, default, bounds, screenshots and numeric project storage passed");
    return 0;
}
