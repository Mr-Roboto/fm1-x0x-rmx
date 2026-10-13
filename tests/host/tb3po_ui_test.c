/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual scale page, selection labels, rendering and numeric storage round trips. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

static void playback_tick(void)
{
    int32_t audio[2];
    seq_ext_clock(&seq);
    engine_render(audio, 1);             /* real engine step callback publishes eng_step[] */
}

static void assert_playback_pixels(int source)
{
    ui_frame();                         /* includes the incremental LCD transfer */
    for (int c = 0; c < 16; c++)
        for (int y = 0; y < 2; y++)
            for (int x = 0; x < 12; x++) {
                if (fb[(MAIN_Y + y) * 240 + col_x(c) + x] !=
                    swap16(source == step_of(c) ? part_col() : C_BLACK)) {
                    fprintf(stderr, "playhead LCD: part %d view %d page %d len %d slot %d source %d at %d,%d col %d got %04x expected %04x\n",
                            ui.part, ui.view, ui.spage, cur_pat()->bass[bidx()].len,
                            eng_step[ui.part], source, x, y, c,
                            fb[(MAIN_Y + y) * 240 + col_x(c) + x],
                            swap16(source == step_of(c) ? part_col() : C_BLACK));
                    png_write("build/host/tb3po_playhead_failed.png");
                }
                assert(fb[(MAIN_Y + y) * 240 + col_x(c) + x] ==
                       swap16(source == step_of(c) ? part_col() : C_BLACK));
            }
}

/* Independent B12/A3 oracles: repeats and jumps to stored sources beyond len.
 * Check actual LCD output through clocks, wraps, page changes and both views;
 * checking playback_head() alone misses invisible/obscured markers. */
static void test_playback_pixels(void)
{
    static const uint8_t b12[16] = {0, 0, 15, 15, 0, 0, 15, 15, 0, 0, 15, 15, 0, 0, 15, 15};
    static const uint8_t a3[16] = {0, 5, 2, 7, 0, 5, 2, 7, 8, 13, 10, 15, 8, 13, 10, 15};
    for (int len = 1; len <= NSTEPS; len++) {
        for (int remix = 0; remix < 2; remix++) {
            project_defaults();
            engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
            ui_init();
            seq.send_clock = 0;
            for (int b = 0; b < NBASS; b++) {
                bpart_t *bp = &proj.pat[0].bass[b];
                bp->len = len;
                bp->gen.mutate_bars = 0;
                for (int s = 0; s < NSTEPS; s++) {
                    bp->step[s].note = 48 + s % 12;
                    bp->step[s].flags = s % 3 == 0 ? G_REST : s % 3 == 1 ? G_NOTE | BS_ACCENT : G_TIE;
                }
                if (len == 3 && b == 0) {
                    /* B12 selects this note beyond len: its clipped brick
                     * overlaps the cap unless the marker is drawn last. */
                    bp->step[15].note = 62;
                    bp->step[15].flags = G_NOTE;
                }
                seq_set_remix(&seq, TRK_BASS0 + b, b ? 0 : 1, b ? 2 : 11, remix);
            }
            seq_start(&seq);
            playback_tick();
            for (int n = 0; n <= len; n++) {  /* includes the final partial block and loop wrap */
                int slot = n % len;
                for (int b = 0; b < NBASS; b++) {
                    int source = remix ? (slot & ~15) + (b ? a3 : b12)[slot & 15] : slot;
                    assert(eng_step[TRK_BASS0 + b] == slot);
                    ui.part = PART_303A + b;
                    for (int view = 0; view < 2; view++) {
                        set_view(view ? V_SEQ : V_GEN);
                        for (int page = 0; page < 4; page++) {
                            ui.spage = page;
                            assert_playback_pixels(page == slot / 16 ? source : -1);
                            assert(ui.spage == page);
                        }
                    }
                }
                if (len == 24 && n == 18 && remix) {
                    ui.part = PART_303A;
                    ui.spage = 1;
                    assert_playback_pixels(31);
                    png_write("build/host/tb3po_playhead_partial.png");
                }
                for (int k = 0; k < 6; k++) playback_tick();
            }
            seq_stop(&seq);
            playback_tick();             /* process the transport request in the audio engine */
            assert(!seq.playing);
            for (int b = 0; b < NBASS; b++) {
                ui.part = PART_303A + b;
                for (int view = 0; view < 2; view++) {
                    set_view(view ? V_SEQ : V_GEN);
                    ui.spage = 0;
                    assert_playback_pixels(-1);
                }
            }
        }
    }
    puts("303 playback LCD: every length 1-64, both parts, ARP/SEQ, all pages, real engine clocks, stops/wraps and independent B12/A3 repeated/jumping/beyond-length sources passed");
}

/* Check LCD pixels, including pages inherited from SEQ beyond the line's end.
 * An unobscured but empty roll must not pass as a refreshed pattern. */
static void test_line_refresh(void)
{
    static uint16_t immediate[240 * MAIN_ROWS];
    const int lengths[] = {1, 8, 16, 18, 24, 32, 64};
    for (unsigned l = 0; l < sizeof lengths / sizeof lengths[0]; l++) {
        for (int b = 0; b < 2; b++) {
            for (int page = 0; page < 2; page++) {
                for (int playing = 0; playing < 2; playing++) {
                    project_defaults();
                    seq_init(&seq, proj.pat);
                    ui_init();
                    ui.part = PART_303A + b;
                    set_view(V_GEN);
                    *page_of(V_GEN) = page;
                    seq.playing = playing;
                    bpart_t *bp = &cur_pat()->bass[b];
                    bp->len = lengths[l];
                    tb3po_generate(bp);
                    for (int spage = 0; spage < NSTEPS / 16; spage++) {
                        for (int up = 1; up >= 0; up--) {
                            ui.spage = spage;
                            now_ms += 100;
                            enc_acc[EN_K1] = 1;
                            ui_frame();
                            assert(ui.touched == 0 && ui.touch_until > now_ms);
                            int button = up ? B_OCTUP : B_OCTDN;
                            held_btn = 1u << button;
                            ui_frame();
                            now_ms += 20;
                            bpart_t expected = *bp;
                            if (up) {
                                expected.gen.seed = tb3po_new_seed(now_ms ^ (ui.frame << 7));
                                tb3po_generate(&expected);
                            } else {
                                uint32_t rng = expected.gen.seed ^ now_ms;
                                tb3po_mutate(&expected, &rng);
                            }
                            held_btn = 0;
                            ui_frame();                 /* first frame after the real button tap */
                            assert(!memcmp(bp, &expected, sizeof expected));
                            memcpy(immediate, fb + MAIN_Y * 240, sizeof immediate);
                            int note_pixels = 0;
                            for (int y = ROLL_TOP; y < ROLL_BOT + 5; y++)
                                for (int c = 0; c < 16; c++) {
                                    uint16_t px = fb[(MAIN_Y + y) * 240 + col_x(c) + 3];
                                    note_pixels += px == swap16(part_col()) || px == swap16(dim(part_col(), 9));
                                }
                            if (!note_pixels)
                                png_write("build/host/tb3po_empty_roll.png");
                            assert(note_pixels > 0);
                            ui.touched = -1;
                            ui.spage = 0;
                            blit_hash[1] = 0;          /* independent full LCD transfer of the first page */
                            draw_main();
                            assert(!memcmp(immediate, fb + MAIN_Y * 240, sizeof immediate));
                            if (l == 2 && b == 0 && page == 0 && !playing && spage == 3) {
                                char path[80];
                                snprintf(path, sizeof path, "build/host/tb3po_oct%s_visible.png", up ? "up" : "dn");
                                png_write(path);
                            }
                        }
                    }
                }
            }
        }
    }
    puts("TB-3PO UI: lengths 1/8/16/18/24/32/64, visible LCD notes and first-page OCT+/OCT- refresh from all step pages after knob edits, both parts/knob pages, stopped/playing passed");
}

int main(void)
{
    test_playback_pixels();
    test_line_refresh();
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
