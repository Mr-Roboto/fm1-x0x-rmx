/* SPDX-License-Identifier: GPL-3.0-only */
/* Real BREAK preferences, command consumption, motion, Undo and flash loading. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/engine.c"
#include <assert.h>

static pref_t filter_ref(void) { return PR(R_ENG, T_BRK, 0, 2); }
static void flush(void) { drain(); engine_sync_sound(&proj.sound); }
static void setup(void)
{
    project_defaults();
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    engine_apply_sound(&proj.sound); flush(); ui_init();
}
static void turn(int encoder, int amount)
{
    accel_off = 1; enc_acc[encoder] = amount; ui_input_only(); accel_off = 0; flush();
}

static void controls(void)
{
    setup();
    ui.part = PART_BRK; ui.view = V_PART; ui.page[V_PART][PART_BRK] = 0; build_pages();
    assert(pg.n == 2 && pg.r[0][3].c == 1); /* LOOPS and the old Pitch mapping */
    ui.page[V_PART][PART_BRK] = 1; build_pages();
    assert(!strcmp(pg.title[1], "SOUND"));
    assert(pg.r[1][0].a == T_BRK && pg.r[1][0].c == 2);
    assert(pg.r[1][1].a == T_BRK && pg.r[1][1].c == 0);
    assert(engine_nparams(T_BRK, 0) == 3 && engine_param(T_BRK, 0, 2)->def == 64);
    turn(EN_K1, -30); assert(pref_get(filter_ref()) == 34 && breaks_get(&brk, BRK_FILT) == 34);
    draw_header(); draw_main(); draw_knobs(); png_write("build/host/break-filter/lp-ui.png");
    turn(EN_K1, 60); assert(pref_get(filter_ref()) == 94 && breaks_get(&brk, BRK_FILT) == 94);
    draw_header(); draw_main(); draw_knobs(); png_write("build/host/break-filter/hp-ui.png");
    for (int v = 0; v <= 127; ++v) {
        char number[24], unit[8], expected[24];
        pref_set(filter_ref(), v); flush(); pref_value(filter_ref(), number, unit);
        if (v >= 60 && v <= 68) {
            assert(!strcmp(number, "OFF") && !*unit);
        } else {
            snprintf(expected, sizeof expected, "%s %d", v < 60 ? "LP" : "HP",
                     v < 60 ? (60-v)*100/60 : (v-68)*100/59);
            assert(!strcmp(number, expected) && !strcmp(unit, "%"));
        }
        assert(breaks_get(&brk, BRK_FILT) == v);
    }
    pref_set(filter_ref(), -1); flush(); assert(pref_get(filter_ref()) == 0);
    pref_set(filter_ref(), 128); flush(); assert(pref_get(filter_ref()) == 127);
    open_list_of_pages();
    int row = -1;
    for (int i = 0; i < ui.list_n; ++i)
        if (list_rows[i].kind == R_ENG && list_rows[i].a == T_BRK && list_rows[i].c == 2) row = i;
    assert(row >= 0); ui.list_sel = row;
    turn(EN_ALGO, -1); assert(pref_get(filter_ref()) == 126);
    ui.overlay = O_NONE;
    pref_set(filter_ref(), 64); flush(); undo_start();
    pref_set(filter_ref(), 0); flush(); do_undo(0); flush();
    assert(pref_get(filter_ref()) == 64 && breaks_get(&brk, BRK_FILT) == 64);
    do_undo(1); flush(); assert(pref_get(filter_ref()) == 0 && breaks_get(&brk, BRK_FILT) == 0);
    puts("PASS: BREAK SOUND page, unchanged LOOPS/Pitch, knob/SEL, all value readouts/bounds and Undo/Redo");
}

static void persistence(void)
{
    setup();
    assert(sizeof(settings_t) == 36 && sizeof(sound_t) == 1344 && sizeof(project_t) == 30952);
    assert(sizeof(pattern_t) == 528 && RAW_LEN == 29572 && PROJ_SND_REV == 5);
    for (int p = 0; p < NPAT; ++p) {
        for (int part = 0; part < 4; ++part) {
            pattern_set_remix_selection(&proj.pat[p], part, p & 15, part + 3);
            pattern_set_remix_enabled(&proj.pat[p], part, p & 1);
        }
        proj.pat[p].bass[0].dir = p % 5;
        proj.pat[p].bass[1].dir = (p + 2) % 5;
        proj.pat[p].brk.steps = 0x5555u;
        proj.pat[p].brk.slice[p % 16] = 3;
    }
    int lane = motion_alloc(proj.arr.lane, 0, PART_BRK, T_BRK, 0, 2);
    assert(lane >= 0); proj.arr.lane[lane].val[0] = 0; proj.arr.lane[lane].val[1] = 127;
    for (int l = 1; l < 8; ++l) {
        int k = motion_alloc(proj.arr.lane, l, PART_909, T_909, l, 0);
        assert(k >= 0); proj.arr.lane[k].val[0] = (uint8_t)(l * 10);
    }
    const int values[] = {0, 59, 60, 64, 68, 69, 127};
    for (unsigned v = 0; v < sizeof values / sizeof *values; ++v) {
        pref_set(filter_ref(), values[v]); flush();
        assert(project_save() == 0);
        project_t expected = proj;
        project_defaults(); assert(project_load() == 0);
        assert(!memcmp(&proj, &expected, sizeof proj));
        engine_apply_sound(&proj.sound); flush();
        assert(breaks_get(&brk, BRK_FILT) == values[v]);
    }
    pref_set(filter_ref(), 64); flush();
    motion_step(&mot, PART_BRK, 0, 0, 1, 16);
    assert(breaks_get(&brk, BRK_FILT) == 0 && proj.sound.v[T_BRK][0][2] == 64);
    motion_release(&mot); assert(breaks_get(&brk, BRK_FILT) == 64);
    /* Revision 4 has the exact v1.0.1 byte layout. Only revision and the newly
     * assigned pot may change; all other sound, pattern and motion bytes survive. */
    proj.arr.lane[lane].i = 1; /* legacy BREAK Pitch motion: indices 0/1 stay unchanged */
    proj.arr.lane[lane].val[0] = 8; proj.arr.lane[lane].val[1] = 16;
    for (int unused = 0; unused <= 127; unused += 127) {
        proj.set.snd_rev = 4;
        proj.sound.v[T_BRK][0][2] = (uint8_t)unused;
        proj.sound.v[T_BRK][0][3] = 93; /* another unused byte: must survive */
        assert(project_save() == 0);
        project_t expected = proj;
        expected.set.snd_rev = 5; expected.sound.v[T_BRK][0][2] = 64;
        project_defaults(); assert(project_load() == 0);
        assert(!memcmp(&proj, &expected, sizeof proj));
        assert(project_save() == 0); assert(project_load() == 0);
        assert(!memcmp(&proj, &expected, sizeof proj));
    }
    puts("PASS: unchanged persistent sizes, whole-project round trips, filter motion/base restoration, revision-4 migration preserves all other bytes");
}

static void historical(void)
{
    for (int short_settings = 0; short_settings < 2; ++short_settings) {
        FILE *f = fopen("tests/fixtures/v1_store.bin", "rb"); assert(f);
        for (int o = 0; o < OBJ_NOBJ; ++o) {
            int32_t n;
            assert(fread(&n, 4, 1, f) == 1);
            assert(fread(store[o], 1, PLAT_STORE_MAX, f) == PLAT_STORE_MAX);
            store_len[o] = n;
        }
        fclose(f);
        if (short_settings) store_len[OBJ_SET] = 20; /* before snd_rev existed */
        assert(project_load() == 0);
        assert(proj.set.snd_rev == 5 && proj.sound.v[T_BRK][0][2] == 64);
    }
    store_len[OBJ_SOUND] = -1;
    assert(project_load() == -1 && proj.sound.v[T_BRK][0][2] == 64);
    puts("PASS: actual historical format-1 fixture, short settings and missing sound default to OFF");
}

int main(void)
{
    controls(); persistence(); historical();
    return 0;
}
