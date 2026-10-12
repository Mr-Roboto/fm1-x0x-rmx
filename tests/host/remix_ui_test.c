/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual app preferences, undo/settings storage, and clocked audible boundaries. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include "../../firmware/src/seq/remix.h"

static unsigned hits[2];
static int notes[2], positions[5];
static motion_t test_motion;
static int test_motion_on;
static int motion_applied;
static int motion_base(void *ctx, int t, int v, int i)
{ (void)ctx; (void)t; (void)v; (void)i; return 77; }
static void motion_apply(void *ctx, int t, int v, int i, int value)
{ (void)ctx; (void)t; (void)v; (void)i; motion_applied = value; }
static void playhead_shot(const char *name)
{
    build_pages();
    draw_header();
    draw_main();
    draw_knobs();
    /* Verify the rendered 303 stripe, including sources beyond partial lengths. */
    int ph = playback_head(ui.part);
    uint16_t color = dim(part_col(), 2);
    color = (uint16_t)((color >> 8) | (color << 8));
    for (int c = 0; c < 16; c++)
        assert((fb[(MAIN_Y + 1) * 240 + col_x(c) + 1] == color) ==
               (ph >= 0 && (ph & 15) == c));
    png_write(name);
}
static int drum_hit(void *ctx, int kit, int voice, float vel)
{
    (void)ctx; (void)vel;
    hits[kit] |= 1u << voice;
    return 1;
}
static void bass_hit(void *ctx, int part, int note, int accent, int slide)
{
    (void)ctx; (void)accent; (void)slide;
    notes[part] = note;
}
static void bass_end(void *ctx, int part) { (void)ctx; (void)part; }
static void position(void *ctx, int part, int step)
{
    (void)ctx;
    positions[part] = step;
    eng_step[part] = step;
    if (test_motion_on)
        motion_step(&test_motion, part, seq.ppat[part], step,
                    seq_next_step(&seq, part, step), seq_part_len(&seq, part));
}
static const seq_sink_t sink = {.drum = drum_hit, .bass_on = bass_hit,
    .bass_off = bass_end, .step = position};

static void clock_tick(void)
{
    seq_ext_clock(&seq);
    seq_advance(&seq, 1, &sink);
}
static void next_step(void)
{
    hits[0] = hits[1] = 0;
    for (int k = 0; k < 6; k++)
        clock_tick();
}
static void setup(int len, int mode)
{
    project_defaults();
    seq_init(&seq, proj.pat);
    seq.send_clock = 0;
    seq.remix_apply = mode;
    for (int t = 0; t < 4; t++) {
        /* Separate playing patterns as well as separate part state. */
        seq.ppat[t] = t;
        pattern_t *p = &proj.pat[t];
        if (t < 2) {
            p->drum[t].len = len;
            for (int s = 0; s < 64; s++)
                for (int v = 0; v < 5; v++)
                    if (((s % 16 + 1) >> v) & 1)
                        p->drum[t].hit[v][s / 32] |= 1u << (s % 32);
        } else {
            p->bass[t - 2].len = len;
            for (int s = 0; s < 64; s++) {
                p->bass[t - 2].step[s].note = 40 + s;
                p->bass[t - 2].step[s].flags = G_NOTE;
            }
        }
    }
    seq_start(&seq);
    hits[0] = hits[1] = 0;
    clock_tick();
}
static void audible(int t, int slot, int selection, int enabled)
{
    int src = enabled ? remix_source_step(selection >> 4, selection & 15, slot) : slot;
    assert(seq_playback_source_step(&seq, t) == src);
    assert(seq.t[t].pos == slot);
    assert(positions[t] == slot); /* callbacks stay chronological */
    if (t < 2)
        assert(hits[t] == (unsigned)(src % 16 + 1));
    else
        assert(notes[t - 2] == 40 + src);
}
static void timing(void)
{
    const int lengths[] = {1, 3, 8, 16, 24, 40, 64};
    for (int mode = 0; mode < 4; mode++)
        for (unsigned l = 0; l < sizeof lengths / sizeof lengths[0]; l++) {
            int len = lengths[l];
            setup(len, mode);
            for (int t = 0; t < 4; t++) {
                /* Rapid edits replace pending values, including ON and OFF. */
                seq_set_remix(&seq, t, 1, 2, 1);
                seq_set_remix(&seq, t, 3, 4, 0);
                seq_set_remix(&seq, t, t + 1, t + 5, 1);
                assert(seq.remix[t].active == 0);
            }
            int activated = 0;
            for (int n = 1; n <= len + 16; n++) {
                int slot = n % len;
                if (mode == REMIX_STEP || (mode == REMIX_BEAT && slot % 4 == 0) ||
                    (mode == REMIX_BAR && slot % 16 == 0) || (mode == REMIX_LOOP && slot == 0))
                    activated = 1;
                next_step();
                for (int t = 0; t < 4; t++) {
                    int sel = ((t + 1) << 4) | (t + 5);
                    assert(seq.remix[t].active == (activated ? (sel | 256) : 0));
                    audible(t, slot, sel, activated);
                    if (activated)
                        assert(seq.remix[t].active == seq.remix[t].pending);
                }
            }
            /* Edit just after a boundary: disable waits just like enable. */
            while (seq.t[0].pos != 0)
                next_step();
            for (int t = 0; t < 4; t++)
                seq_set_remix(&seq, t, t + 1, t + 5, 0);
            activated = 0;
            for (int n = 1; n <= len; n++) {
                int slot = n % len;
                if (mode == REMIX_STEP || (mode == REMIX_BEAT && slot % 4 == 0) ||
                    (mode == REMIX_BAR && slot % 16 == 0) || (mode == REMIX_LOOP && slot == 0))
                    activated = 1;
                next_step();
                for (int t = 0; t < 4; t++)
                    audible(t, slot, ((t + 1) << 4) | (t + 5), !activated);
            }
            /* OFF still retains edited selection; stop resolves pending immediately. */
            seq_set_remix(&seq, 0, 15, 15, 0);
            seq_stop(&seq);
            seq_advance(&seq, 1, &sink);
            assert(seq.remix[0].active == 255);
            seq_set_remix(&seq, 0, 15, 15, 1);
            assert(seq.remix[0].active == 511);
        }
    /* Each track reaches LOOP at its own length, with the same global APPLY. */
    setup(8, REMIX_LOOP);
    proj.pat[1].drum[1].len = 3;
    proj.pat[2].bass[0].len = 24;
    proj.pat[3].bass[1].len = 40;
    const int lens[] = {8, 3, 24, 40};
    for (int t = 0; t < 4; t++) seq_set_remix(&seq, t, t + 1, 7, 1);
    for (int n = 1; n <= 40; n++) {
        next_step();
        for (int t = 0; t < 4; t++) {
            assert(seq.remix[t].active == (n >= lens[t] ? (256 | ((t + 1) << 4) | 7) : 0));
            audible(t, n % lens[t], ((t + 1) << 4) | 7, n >= lens[t]);
        }
    }
    /* Selection edits while OFF are retained for a subsequent quantized enable. */
    setup(16, REMIX_BEAT);
    seq_set_remix(&seq, 0, 9, 10, 0);
    next_step();
    seq_set_remix(&seq, 0, 9, 10, 1);
    for (int n = 2; n <= 4; n++) {
        next_step();
        audible(0, n, 0x9A, n == 4);
        assert(seq.remix[1].active == 0 && seq.remix[2].active == 0 && seq.remix[3].active == 0);
    }
    /* Changing global APPLY redirects an outstanding change, without applying early. */
    setup(24, REMIX_LOOP);
    seq_set_remix(&seq, 0, 2, 3, 1);
    next_step();
    pref_set(PR(R_RAPPLY, 0, 0, 0), REMIX_BEAT);
    assert(seq.remix[0].active == 0);
    for (int n = 2; n <= 4; n++) {
        next_step();
        audible(0, n, 0x23, n == 4);
    }
    /* Internal clock also activates STEP on the next step, not the next audio block. */
    setup(16, REMIX_STEP);
    seq.ext = 0;
    for (int t = 0; t < NTRACKS; t++) seq.t[t].to_next = seq_step_q8(&seq, RATE_16);
    seq_set_remix(&seq, 0, 5, 6, 1);
    seq_advance(&seq, 1, &sink);
    assert(seq.remix[0].active == 0);
    hits[0] = hits[1] = 0;
    while (seq.t[0].pos == 0) seq_advance(&seq, seq_until_event(&seq, 65535), &sink);
    audible(0, 1, 0x56, 1);
    /* Pending state belongs to the old pattern; a cue adopts the new pattern. */
    setup(8, REMIX_LOOP);
    seq_set_remix(&seq, 0, 4, 5, 1);
    pattern_set_remix_selection(&proj.pat[7], 0, 8, 9);
    pattern_set_remix_enabled(&proj.pat[7], 0, 1);
    seq_cue_part(&seq, 0, 7);
    for (int n = 0; n < 8; n++) next_step();
    assert(seq.ppat[0] == 7 && seq.remix[0].active == 0x189);
    assert(seq.remix[0].active == seq.remix[0].pending);
}
static void playhead(void)
{
    const int lengths[] = {16, 24, 32, 40, 48, 64};
    int repeated = 0, jumped = 0, beyond_length = 0;
    ui_init();
    for (unsigned l = 0; l < sizeof lengths / sizeof lengths[0]; l++) {
        int len = lengths[l];
        setup(len, REMIX_BEAT);
        for (int n = 0; n < len; n++) {
            for (int t = 0; t < 4; t++) {
                assert(seq_playback_source_step(&seq, t) == n);
                for (int page = 0; page < 4; page++) {
                    ui.spage = page;
                    assert(playback_head(t) == (page == n / 16 ? n : -1));
                    assert(ui.spage == page);
                }
            }
            next_step();
        }
        for (int t = 0; t < 4; t++) seq_set_remix(&seq, t, 1, 11, 1);
        for (int n = 1; n <= len; n++) {
            next_step();
            int slot = n % len;
            for (int t = 0; t < 4; t++) {
                int src = n < 4 ? slot : remix_source_step(1, 11, slot);
                assert(seq_playback_source_step(&seq, t) == src);
                for (int page = 0; page < 4; page++) {
                    ui.spage = page;
                    assert(playback_head(t) == (page == slot / 16 ? src : -1));
                }
                assert(rec_step(t) == slot && seq.t[t].pos == slot);
                if (src >= len) beyond_length = 1;
                if (n > 4) {
                    int prev = remix_source_step(1, 11, (n - 1) % len);
                    repeated |= src == prev;
                    jumped |= abs(src - prev) > 1;
                }
            }
        }
        /* A different stored mapping stays pending until the next BEAT. */
        for (int t = 0; t < 4; t++) seq_set_remix(&seq, t, 15, 15, 1);
        for (int n = 1; n <= 4; n++) {
            next_step();
            for (int t = 0; t < 4; t++)
                assert(seq_playback_source_step(&seq, t) ==
                       remix_source_step(n < 4 ? 1 : 15, n < 4 ? 11 : 15, n));
        }
        for (int dir = DIR_REV; dir <= DIR_RANDOM; dir++) {
            proj.pat[2].bass[0].dir = dir;
            next_step();
            assert(seq_playback_source_step(&seq, 2) == positions[2]);
        }
        assert(seq_playback_source_step(&seq, TRK_BRK) == positions[TRK_BRK]);
        seq_stop(&seq);
        assert(playback_head(0) == -1);
    }
    assert(repeated && jumped && beyond_length);
    /* Real drum recording writes the chronological slot, not its audible source. */
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    setup(16, REMIX_STEP);
    seq_set_remix(&seq, 0, 1, 11, 1);
    next_step();
    ui.part = 0;
    ui.rec = 1;
    int slot = positions[0], src = seq_playback_source_step(&seq, 0);
    assert(src != slot);
    memset(proj.pat[0].drum[0].hit[10], 0, sizeof proj.pat[0].drum[0].hit[10]);
    drum_key(10, 1);
    assert(sm_get(proj.pat[0].drum[0].hit[10], slot));
    assert(!sm_get(proj.pat[0].drum[0].hit[10], src));
    /* Motion receives and applies the chronological lane through the step callback. */
    int k = motion_alloc(proj.arr.lane, 0, 0, T_PROB, 0, 0);
    assert(k >= 0);
    for (int n = 0; n < 16; n++) proj.arr.lane[k].val[n] = n + 30;
    motion_init(&test_motion, proj.arr.lane);
    test_motion.base = motion_base;
    test_motion.apply = motion_apply;
    test_motion_on = 1;
    next_step();
    assert(motion_value(&test_motion, k) == positions[0] + 30);
    assert(test_motion.last[0] == positions[0]);
    assert(motion_applied == positions[0] + 30);
    motion_req_rec(&test_motion, k);
    next_step();
    assert(proj.arr.lane[k].val[positions[0]] == 77);
    assert(proj.arr.lane[k].val[seq_playback_source_step(&seq, 0)] != 77);
    test_motion_on = 0;
    setup(24, REMIX_BEAT);
    ui_init();
    ui.part = 2;
    ui.view = V_SEQ;
    ui.rec = 0;
    ui.msg_until = 0;
    ui.spage = 0;
    seq_set_remix(&seq, 2, 1, 11, 1);
    next_step();
    playhead_shot("build/host/remix_head_pending.png");
    next_step(); next_step(); next_step(); next_step(); next_step();
    playhead_shot("build/host/remix_head_active.png");
    while (positions[2] != 18) next_step();
    ui.spage = 1;
    assert(playback_head(2) == 31);
    playhead_shot("build/host/remix_head_partial.png");
    ui.spage = 0;
    assert(playback_head(2) == -1);
    playhead_shot("build/host/remix_head_hidden.png");
}

static void preferences(void)
{
    project_defaults();
    seq_init(&seq, proj.pat);
    ui_init();
    assert(seq.remix_apply == REMIX_BEAT);
    assert(pref_desc(PR(R_RAPPLY, 0, 0, 0))->def == REMIX_BEAT);
    assert(settings_remix_apply(&proj.set) == REMIX_BEAT);
    ui.view = V_SEQ;
    const int kinds[] = {R_RBANK, R_RPAT, R_RENABLE, R_RAPPLY};
    const char *names[] = {"BANK", "PATTERN", "REMIX", "APPLY"};
    for (int t = 0; t < 5; t++) {
        ui.part = t;
        build_pages();
        int found = -1;
        for (int k = 0; k < pg.n; k++)
            if (!strcmp(pg.title[k], "REMIX")) found = k;
        if (t == TRK_BRK) { assert(found == -1); continue; }
        assert(found == (t < 2 ? 1 : 2));
        assert(!strcmp(pg.title[0], "SEQUENCE"));
        if (t >= 2) assert(!strcmp(pg.title[1], "KEYS"));
        *page_of(V_SEQ) = found;
        for (int k = 0; k < 4; k++) {
            pref_t r = pg.r[found][k];
            assert(r.kind == kinds[k]);
            assert(!strcmp(pref_name(r), names[k]));
            assert(!lockable(r) && !has_motion(r));
            seq.playing = ui.rec = 1;
            lane_t before[NLANE];
            memcpy(before, proj.arr.lane, sizeof before);
            motion_touch(r);
            motion_forget(r);
            assert(!memcmp(before, proj.arr.lane, sizeof before));
            seq.playing = ui.rec = 0;
            assert(in_pattern(r) == (k != 3));
            for (int v = 0; v <= pref_desc(r)->max; v++) {
                char num[24], unit[24], expected[24];
                pref_set(r, v);
                assert(pref_get(r) == v && ui_dirty());
                pref_value_of(r, v, num, unit);
                if (k == 0) { expected[0] = 'A' + v; expected[1] = 0; }
                else if (k == 1) snprintf(expected, sizeof expected, "%d", v + 1);
                else strcpy(expected, k == 2 ? ONOFF_N[v] : APPLY_N[v]);
                assert(!strcmp(num, expected) && !unit[0]);
            }
        }
    }
    /* APPLY is shared across tracks and patterns, outside the undo region. */
    for (int t = 0; t < 4; t++) {
        seq.ppat[t] = 7;
        assert(pref_get(PR(R_RAPPLY, t, 0, 0)) == REMIX_LOOP);
    }
    seq.ppat[2] = 0;
    ui.part = 2;
    undo_start();
    pref_set(PR(R_RBANK, 2, 0, 0), 2);
    pref_set(PR(R_RENABLE, 2, 0, 0), 0);
    pref_set(PR(R_RAPPLY, 0, 0, 0), REMIX_BAR);
    do_undo(0);
    assert(pref_get(PR(R_RBANK, 2, 0, 0)) == 15);
    assert(pref_get(PR(R_RENABLE, 2, 0, 0)) == 1);
    assert(pref_get(PR(R_RAPPLY, 0, 0, 0)) == REMIX_BAR);
    assert(strstr(ui.msg, "303") && !strstr(ui.msg, "BREAK"));
    do_undo(1);
    assert(pref_get(PR(R_RBANK, 2, 0, 0)) == 2);
    /* Settings/project layout stays unchanged. */
    assert(sizeof(settings_t) == 28 && sizeof(project_t) == 30944);
    for (int byte = 0; byte < 256; byte++) {
        proj.set.rsv[0] = byte;
        assert(settings_remix_apply(&proj.set) == ((byte & 0xFC) == 0xB4 ? byte & 3 : REMIX_BEAT));
    }
    for (int mode = 0; mode < 4; mode++) {
        pref_set(PR(R_RAPPLY, 0, 0, 0), mode);
        assert(project_save() == 0);
        project_defaults();
        assert(settings_remix_apply(&proj.set) == REMIX_BEAT);
        assert(project_load() == 0);
        project_apply_io();
        assert(settings_remix_apply(&proj.set) == mode && seq.remix_apply == mode);
    }
    /* A historical shorter settings payload has no reserved byte. */
    store_len[OBJ_SET] = 4 + __builtin_offsetof(settings_t, rsv);
    assert(project_load() == 0);
    project_apply_io();
    assert(seq.remix_apply == REMIX_BEAT);
}
int main(void)
{
    preferences();
    timing();
    playhead();
    puts("Remix UI: pages, labels, ranges, non-motion refs, undo, global storage, audible boundaries, mapped playheads and chronological recording/Motion passed");
    return 0;
}
