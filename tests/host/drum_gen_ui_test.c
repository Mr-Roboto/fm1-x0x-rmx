/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual panel/UI dispatch, scoped Undo, publication and rendered pages. */
#define X0X_TEST_EDIT_HOOK
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

static int lock_depth, lock_calls, watch, inject;
static uint32_t pool_at_lock;
static dpart_t *watched;
static dpart_t old_drum;
static drum_gen_edit_t expected;
static drum_gen_cfg_t *watched_cfg;

static void test_edit_hook(int locked)
{
    if (locked) {
        assert(!lock_depth);
        lock_depth = 1;
        lock_calls++;
        pool_at_lock = undo.used;
        if (watch) assert(!memcmp(watched, &old_drum, sizeof old_drum));
    } else {
        assert(lock_depth && undo.used == pool_at_lock); /* no Undo in critical section */
        if (watch) {
            int kit = ui.part;
            for (int ch = 0; ch < DG_CHANNELS; ch++)
                assert(!memcmp(watched->hit[dg_row(kit, ch)], expected.hit[ch], sizeof expected.hit[ch]));
            assert(!memcmp(watched->accent, expected.accent, sizeof expected.accent));
            assert(!memcmp(watched_cfg, &expected.cfg, sizeof expected.cfg));
        }
        lock_depth = 0;
        if (inject) proj.pat[0].bass[0].step[3].flags ^= BS_ACCENT; /* audio-side edit at release */
    }
}

static void reset_ui(int length)
{
    watch = inject = lock_depth = lock_calls = 0;
    held_btn = held_keys = 0;
    memset(enc_acc, 0, sizeof enc_acc);
    project_defaults();
    for (int k = 0; k < NKIT; k++) proj.pat[0].drum[k].len = (uint8_t)length;
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    engine_apply_sound(&proj.sound);
    drum_gen_init(1234);
    ui_init();
    ui.dirty = 0;
}

static void tap(int button)
{
    held_btn = 1u << button;
    ui_input_only();
    held_btn = 0;
    ui_input_only();
    assert(!lock_depth);
}

static void turn(int encoder, int amount)
{
    enc_acc[encoder] = amount;
    ui_input_only();
}

static void chord(int modifier, int button)
{
    held_btn = 1u << modifier;
    ui_input_only();
    held_btn |= 1u << button;
    ui_input_only();
    held_btn = 1u << modifier;
    ui_input_only();
    held_btn = 0;
    ui_input_only();
}

static void shot(const char *name)
{
    ui.help = 0; ui.msg_until = 0; ui.touched = -1;
    build_pages();
    draw_header(); draw_main(); draw_knobs(); draw_footer();
    png_write(name);
}

static void assert_page(const char *name, const char *a, const char *b, const char *c, const char *d)
{
    const char *labels[] = {a, b, c, d};
    assert(!strcmp(pg.title[cur_page()], name));
    for (int k = 0; k < 4; k++) {
        pref_t r = pg.r[cur_page()][k];
        if (!labels[k]) assert(r.kind == R_NONE);
        else {
            assert(r.kind == R_DGEN);
            assert(!strcmp(pref_desc(r)->name, labels[k]));
        }
    }
}

static void navigation(void)
{
    reset_ui(16);                                  /* match the emulator's default pattern */
    tap(B_ARP);
    assert(ui.view == V_GEN && pg.n == 3);
    assert_page("MAP", "X", "Y", "CHAOS", "MODE");
    shot("build/scenarios/drum_gen/909-map.png");
    turn(EN_K4, 1);
    assert_page("EUCLID", "LEN K", "LEN S", "LEN H", "MODE");
    assert(!ui.dirty && undo.n == 0);
    shot("build/scenarios/drum_gen/909-euclid.png");
    tap(B_ARP);
    assert_page("FILL", "FILL K", "FILL S", "FILL H", 0);
    shot("build/scenarios/drum_gen/909-fill.png");
    tap(B_ARP);
    assert_page("OFFSET", "OFF K", "OFF S", "OFF H", 0);
    shot("build/scenarios/drum_gen/909-offset.png");
    tap(B_ARP);
    assert(cur_page() == 0 && pg.n == 3);
    turn(EN_SELECT, 1); assert(cur_page() == 1);
    turn(EN_SELECT, 1); assert(cur_page() == 2);
    turn(EN_SELECT, -1); assert(cur_page() == 1);
    *page_of(V_GEN) = 0; build_pages();
    pref_set(pg.r[0][0], 11); /* LEN K = 12 */
    drum_gen_cfg_t keep = drum_gen_session[0];
    turn(EN_ALGO, 1);
    assert(ui.part == PART_808 && ui.view == V_GEN && pg.n == 3);
    assert_page("MAP", "X", "Y", "CHAOS", "MODE");
    turn(EN_K1, 1);
    assert(drum_gen_session[1].x > 128);
    assert(!memcmp(&keep, &drum_gen_session[0], sizeof keep));
    shot("build/scenarios/drum_gen/808-map.png");
    turn(EN_ALGO, -1);
    assert_page("EUCLID", "LEN K", "LEN S", "LEN H", "MODE");
    assert(!memcmp(&keep, &drum_gen_session[0], sizeof keep));
    for (int part = PART_303A; part <= PART_303B; part++) {
        ui.part = (uint8_t)part; set_view(V_HOME); tap(B_ARP);
        assert(ui.view == V_GEN && pg.n == 2 && pg.r[0][0].kind == R_GEN);
        assert(!strcmp(pg.title[0], "GENERATE"));
        tap(B_OCTUP);
        assert(!strncmp(ui.msg, "NEW LINE ", 9));
        tap(B_OCTDN); assert(!strcmp(ui.msg, "MUTATED"));
    }
    turn(EN_ALGO, 1);
    assert(ui.part == PART_BRK && ui.view == V_PART);
    tap(B_ARP); assert(ui.view != V_GEN);
    assert(!strcmp(ui.msg, "PICK 909/808 OR 303"));
    ui.part = PART_909; set_view(V_GEN);
    tap(B_SEL);
    assert(ui.overlay == O_LIST && !strcmp(list_title, "GRIDS"));
    ui.list_sel = 3; turn(EN_ALGO, -1);
    assert(drum_gen_session[0].mode == DG_MAP && list_rows[0].c == D_X && ui.list_sel == 3);
    turn(EN_ALGO, 1);
    assert(drum_gen_session[0].mode == DG_EUCLID && list_rows[0].c == D_LEN && ui.list_sel == 3);
    ui.overlay = O_NONE;
    assert(help_fits());
    puts("drum UI: ARP parts, both three-page modes, SELECT, MODE/list refresh, independent kits and TB-3PO passed");
}

static void bounds_and_transience(void)
{
    reset_ui(64); tap(B_ARP);
    drum_gen_cfg_t *g = &drum_gen_session[0];
    pref_set(pg.r[0][0], 999); assert(g->x == 255);
    pref_set(pg.r[0][1], -1); assert(g->y == 0);
    pref_set(pg.r[0][2], 999); assert(g->chaos == 100);
    pref_set(pg.r[0][3], DG_EUCLID);
    pref_set(pg.r[0][0], 31); assert(g->len[0] == 32);
    pref_set(pg.r[2][0], 31); assert(g->offset[0] == 31);
    pref_set(pg.r[0][0], 7); assert(g->len[0] == 8 && g->offset[0] == 7);
    pref_set(pg.r[0][3], DG_MAP); assert(g->offset[0] == 7);
    g->offset[1] = 15; g->len[1] = 5;
    pref_set(pg.r[0][3], DG_EUCLID);
    assert(g->offset[1] == 0 && g->len[1] == 5);
    assert(!ui.dirty && undo.n == 0);
}

static void transient_checks(void)
{
    reset_ui(64); tap(B_ARP);
    drum_gen_cfg_t *g = &drum_gen_session[0];
    g->mode = DG_EUCLID; g->len[0] = 32; g->offset[0] = 31; build_pages();
    project_t before = proj;
    ui.rec = 1; seq.playing = 1;
    turn(EN_K1, -1);
    assert(!ui.dirty && !memcmp(&before, &proj, sizeof before) && undo.n == 0);
    seq.playing = 0; ui.rec = 0;
    g->x = 77; g->fill[1] = 41;
    drum_gen_cfg_t keep = *g;
    proj.pat[1].drum[0].len = 8;
    turn(EN_PRESET, 1); run_ms(10); ui_input_only();
    assert(seq.ppat[0] == 1 && seq.ppat[1] == 0);
    assert(g->x == 77 && g->fill[1] == 41 && g->mode == DG_EUCLID);
    assert(g->len[0] == 8 && g->offset[0] < 8);
    assert(pref_desc(pg.r[0][0])->max == 7);
    pref_set(pg.r[0][0], 999); assert(g->len[0] == 8);
    pref_set(pg.r[2][0], 999); assert(g->offset[0] == 7);
    pref_set(pg.r[0][0], 0); assert(g->len[0] == 1 && g->offset[0] == 0);
    assert(pref_desc(pg.r[2][0])->max == 0);
    assert(proj.pat[1].drum[0].len == 8 && !ui.dirty);
    turn(EN_PRESET, -1); run_ms(10); ui_input_only();
    assert(g->len[0] == 1 && g->x == keep.x && g->fill[1] == keep.fill[1]);
    pref_set(PR(R_DLEN, 0, 0, 0), 3);
    assert(g->len[1] <= 4 && g->len[2] <= 4);
    assert(project_save() == 0);
    keep = *g;
    project_defaults(); assert(project_load() == 0); build_pages();
    assert(!memcmp(g, &keep, sizeof keep));
    puts("drum UI: bounds, shorter pattern/length normalization, no dirty/motion/Undo from knobs and session retention passed");
}

static void commands(void)
{
    reset_ui(32); tap(B_ARP);
    tap(B_OCTUP);
    assert(!strcmp(ui.msg, "NEW DRUMS") && ui.dirty && undo.n == 1 && lock_calls == 1);
    dpart_t first = proj.pat[0].drum[0];
    ui.dirty = 0;
    tap(B_OCTUP);
    assert(!strcmp(ui.msg, "NO CHANGE") && !ui.dirty && undo.n == 1 && lock_calls == 1);
    tap(B_OCTDN);
    assert(!strcmp(ui.msg, "NO CHANGE") && !memcmp(&first, &proj.pat[0].drum[0], sizeof first));
    drum_gen_cfg_t *g = &drum_gen_session[0];
    pref_set(pg.r[0][3], DG_EUCLID);
    g->len[0] = 16; g->len[1] = 12; g->len[2] = 8; build_pages();
    tap(B_OCTUP);
    int history = undo.n;
    dpart_t before = proj.pat[0].drum[0];
    drum_gen_cfg_t cfg_before = *g;
    tap(B_OCTDN);
    assert(!strcmp(ui.msg, "MUTATED") && undo.n == history + 1);
    dpart_t mutated = proj.pat[0].drum[0];
    drum_gen_cfg_t cfg_after = *g;
    assert(!memcmp(before.hit[0], mutated.hit[0], sizeof before.hit[0]));
    assert(g->offset[0] == cfg_before.offset[0]);
    assert(!memcmp(g->fill, cfg_before.fill, sizeof g->fill) && !memcmp(g->len, cfg_before.len, sizeof g->len));
    int channel = g->offset[1] != cfg_before.offset[1] ? 1 : 2;
    int delta = (g->offset[channel] + g->len[channel] - cfg_before.offset[channel]) % g->len[channel];
    assert(delta == 2 || delta == g->len[channel] - 2);
    assert(pref_get(pg.r[2][channel]) == g->offset[channel]);
    chord(B_HOME, B_REC);
    assert(!memcmp(&before, &proj.pat[0].drum[0], sizeof before));
    assert(!memcmp(g, &cfg_after, sizeof cfg_after));
    uint32_t used = undo.used;
    ui.dirty = 0;
    tap(B_OCTDN);
    assert(!strcmp(ui.msg, "NEW FIRST") && !ui.dirty && undo.used == used);
    draw_header(); draw_main(); draw_knobs(); draw_footer();
    png_write("build/scenarios/drum_gen/new-first.png");
    assert(!memcmp(&before, &proj.pat[0].drum[0], sizeof before));
    assert(!memcmp(g, &cfg_after, sizeof cfg_after));
    chord(B_HOME, B_PLAY);
    assert(!memcmp(&mutated, &proj.pat[0].drum[0], sizeof mutated));
    assert(!memcmp(g, &cfg_after, sizeof cfg_after));
    tap(B_OCTUP); assert(!strcmp(ui.msg, "NO CHANGE") && undo.used == used);
    g->fill[1] = g->fill[2] = 100;
    tap(B_OCTUP); history = undo.n; cfg_before = *g; before = proj.pat[0].drum[0];
    tap(B_OCTDN);
    assert(!strcmp(ui.msg, "NO CHANGE") && undo.n == history);
    assert(!memcmp(&cfg_before, g, sizeof *g) && !memcmp(&before, &proj.pat[0].drum[0], sizeof before));
    chord(B_SAVE, B_OCTUP);
    assert(ui.pbank == 1 && !memcmp(&before, &proj.pat[0].drum[0], sizeof before));
    assert(undo.n == history);
    puts("drum UI: NEW/MUTATE, effective +/-2, Kick protection, statuses, no-op history, HOME Undo/Redo and SAVE precedence passed");
}

static void isolation_and_lengths(void)
{
    reset_ui(32); tap(B_ARP);
    drum_gen_cfg_t *g = &drum_gen_session[0];
    pref_set(pg.r[0][3], DG_EUCLID);
    tap(B_OCTUP);
    dpart_t states[3];
    drum_gen_cfg_t last;
    states[0] = proj.pat[0].drum[0];
    int start = undo.n;
    tap(B_OCTDN); states[1] = proj.pat[0].drum[0];
    tap(B_OCTDN); states[2] = proj.pat[0].drum[0]; last = *g;
    assert(undo.n == start + 2); /* no clock advancement / no 400ms batching */
    for (int i = 1; i >= 0; i--) {
        chord(B_HOME, B_REC);
        assert(!memcmp(&states[i], &proj.pat[0].drum[0], sizeof states[i]));
        assert(!memcmp(g, &last, sizeof last));
    }
    for (int i = 1; i <= 2; i++) {
        chord(B_HOME, B_PLAY);
        assert(!memcmp(&states[i], &proj.pat[0].drum[0], sizeof states[i]));
    }
    g->len[1] = 32; g->offset[1] = 31; build_pages();
    pref_set(PR(R_DLEN, 0, 0, 0), 7);
    assert(g->len[1] == 8 && g->offset[1] == 7);
    undo_commit(&undo);
    pref_set(PR(R_DLEN, 0, 0, 0), 31);
    undo_commit(&undo);
    g->len[1] = 32; g->offset[1] = 31;
    chord(B_HOME, B_REC);
    assert(proj.pat[0].drum[0].len == 8 && g->len[1] == 8 && g->offset[1] == 7);

    reset_ui(32); tap(B_ARP);
    watched = &proj.pat[0].drum[0]; watched_cfg = &drum_gen_session[0];
    old_drum = *watched;
    assert(drum_gen_new(watched, watched_cfg, 0, &expected) == DG_CHANGED);
    uint32_t bass_off = (uint32_t)((uint8_t *)&proj.pat[0].bass[0].step[3].flags - undo.region);
    uint8_t original_bass = undo.shadow[bass_off];
    proj.pat[0].bass[0].step[4].flags ^= BS_ACCENT; /* already pending elsewhere */
    uint32_t other_off = bass_off + sizeof(bstep_t);
    watch = inject = 1; seq.playing = 1;
    tap(B_OCTUP);
    watch = inject = 0;
    assert(undo.n == 1 && undo.shadow[bass_off] == original_bass);
    assert(undo.shadow[other_off] != undo.region[other_off]);
    uint32_t lo, hi, drum_off = (uint32_t)((uint8_t *)watched - undo.region);
    undo_span(&undo, 0, &lo, &hi);
    assert(lo >= drum_off && hi < drum_off + sizeof *watched);
    assert(undo_commit(&undo) == 1 && undo.n == 2); /* separate pending playback edit */
    assert(undo_undo(&undo, 0, &lo, &hi) == 1);
    assert(undo_undo(&undo, 0, &lo, &hi) == 1);
    assert(!memcmp(watched, &old_drum, sizeof old_drum));
    assert(undo_redo(&undo, 0, &lo, &hi) == 1);
    assert(!memcmp(watched->hit[0], expected.hit[0], sizeof expected.hit[0]));
    dpart_t playing_drum = *watched;
    drum_gen_cfg_t playing_cfg = *watched_cfg;
    int publications = lock_calls;
    proj.pat[0].bass[0].gen.mutate_bars = 1;
    seq.bpm = 275.0f; seq.req_start = 1;
    run_ms(1200);
    assert(seq.playing && !memcmp(watched, &playing_drum, sizeof playing_drum));
    assert(!memcmp(watched_cfg, &playing_cfg, sizeof playing_cfg) && lock_calls == publications);
    puts("drum UI: rapid isolated transactions, Undo length clamps, guarded publication and pending playback edits passed");
}

int main(void)
{
    navigation();
    /* Boundary checks use actual refs; the core separately exhausts LEN/FILL. */
    bounds_and_transience();
    transient_checks();
    commands();
    isolation_and_lengths();
    puts("drum generator UI: all tests passed");
    return 0;
}
