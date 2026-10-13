/* SPDX-License-Identifier: GPL-3.0-only */
/* Real preferences, audio command consumer, motion and simulated project storage. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/engine.c"
#include <assert.h>

static pref_t wave_ref(int v) { return PR(R_ENG, T_303, v, BASS303_WAVE); }
static pref_t morph_ref(int v) { return PR(R_ENG, T_303, v, BASS303_MORPH); }
static void flush(void)
{
    drain();
    engine_sync_sound(&proj.sound);
    assert(!(b303[0].wave == 2 && b303[1].wave == 2));
    for (int v = 0; v < 2; v++) {
        assert(pref_get(wave_ref(v)) == proj.sound.v[T_303][v][BASS303_WAVE]);
        assert(b303[v].wave == b303[v].pot[BASS303_WAVE]);
    }
}
static void setup(void)
{
    project_defaults();
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    engine_apply_sound(&proj.sound);
    flush();
    ui_init();
}
static void panel_tap(int button)
{
    held_btn = 1u << button; ui_input_only();
    held_btn = 0; ui_input_only();
    flush();
}
static void panel_turn(int encoder, int amount)
{
    accel_off = 1;
    enc_acc[encoder] = amount; ui_input_only();
    accel_off = 0;
    flush();
}
static void panel_chord(int button)
{
    held_btn = 1u << B_HOME; ui_input_only();
    held_btn |= 1u << button; ui_input_only();
    held_btn = 1u << B_HOME; ui_input_only();
    held_btn = 0; ui_input_only();
    flush();
}
static void page_shot(int voice, const char *name)
{
    char path[96];
    snprintf(path, sizeof path, "build/host/waveform-ui/303%c-%s.png", voice ? 'b' : 'a', name);
    ui.help = 0; ui.msg_until = 0; ui.touched = -1;
    build_pages(); draw_header(); draw_main(); draw_knobs();
    png_write(path);
}
static void extra_pages(void)
{
    static const int params[3][4] = {
        {BASS303_CUTOFF, BASS303_RESO, BASS303_ENVMOD, BASS303_DECAY},
        {BASS303_ACCENT, BASS303_WAVE, BASS303_DRIVE, BASS303_SLIDE},
        {BASS303_MORPH, BASS303_ACCDEC, BASS303_DRVTYPE, BASS303_TUNE},
    };
    static const char *const names[3] = {"TONE", "VOICE", "EXTRA"};
    static const char *const labels[4] = {"MORPH", "ACCDEC", "DRVTYPE", "TUNE"};
    setup();
    held_btn = held_keys = 0;
    memset(enc_acc, 0, sizeof enc_acc);
    ui.page[V_PART][PART_303A] = ui.page[V_PART][PART_303B] = 0;
    for (int v = 0; v < 2; v++) {
        ui.part = PART_303A + v; ui.view = V_PART; ui.overlay = O_NONE;
        build_pages();
        assert(pg.n == 3 && pg.nmore == 1);
        assert(pg.more[0].kind == R_ENG && pg.more[0].a == T_303 && pg.more[0].b == v && pg.more[0].c == BASS303_VOLUME);
        for (int page = 0; page < 3; page++) {
            assert(cur_page() == page && !strcmp(pg.title[page], names[page]) && pg.group[page] == GR_SOUND);
            for (int k = 0; k < 4; k++) {
                pref_t r = pg.r[page][k];
                assert(r.kind == R_ENG && r.a == T_303 && r.b == v && r.c == params[page][k]);
                if (page == 2) {
                    assert(!strcmp(caps_of(pref_name(r)), labels[k]));
                    assert(tw(&FONT_XS, labels[k]) <= 56);
                }
            }
            page_shot(v, page == 0 ? "tone" : page == 1 ? "voice" : "extra");
            panel_tap(B_EDIT);
        }
        assert(cur_page() == 0); /* EDIT wraps; SELECT retains its established clamping. */
        panel_turn(EN_SELECT, -1); assert(cur_page() == 0);
        panel_turn(EN_SELECT, 1); assert(cur_page() == 1);
        panel_turn(EN_SELECT, 1); assert(cur_page() == 2);
        panel_turn(EN_SELECT, 1); assert(cur_page() == 2);
        panel_turn(EN_SELECT, -1); assert(cur_page() == 1);
        panel_turn(EN_SELECT, 1);
        panel_tap(B_SEL);
        assert(ui.overlay == O_LIST && ui.list_n == 14);
        for (int i = 0; i < 12; i++) {
            pref_t r = list_rows[i];
            assert(r.kind == R_ENG && r.a == T_303 && r.b == v && r.c == params[i / 4][i % 4]);
            for (int j = 0; j < i; j++) assert(list_rows[j].c != r.c);
        }
        assert(list_rows[12].kind == R_HDR && list_rows[12].a == GR_MORE);
        assert(list_rows[13].kind == R_ENG && list_rows[13].c == BASS303_VOLUME);
        ui.list_sel = 8; ui.list_top = 8; page_shot(v, "sel-extra");
        ui.list_sel = 13; page_shot(v, "sel-volume");
        int volume = pref_get(list_rows[13]);
        panel_turn(EN_ALGO, -1); assert(pref_get(list_rows[13]) == volume - 1);
        panel_turn(EN_ALGO, 1); assert(pref_get(list_rows[13]) == volume);
        panel_tap(B_HOME); assert(ui.overlay == O_NONE && cur_page() == 2);
        undo_start();
        sound_t before = proj.sound, edited;
        for (int k = 0; k < 4; k++) {
            sound_t expected = proj.sound;
            pref_t r = pg.r[2][k];
            int old = pref_get(r);
            assert(old == (k == 2 ? 2 : pref_desc(r)->def)); /* existing factory RAT default */
            int delta = k == 2 ? -1 : 1;
            panel_turn(EN_K1 + k, delta);
            expected.v[T_303][v][r.c] = old + delta;
            assert(!memcmp(&proj.sound, &expected, sizeof expected));
            assert(bass303_get(&b303[v], r.c) == old + delta);
        }
        edited = proj.sound;
        assert(undo_commit(&undo) == 1);
        panel_chord(B_REC); assert(!memcmp(&proj.sound, &before, sizeof before));
        panel_chord(B_PLAY); assert(!memcmp(&proj.sound, &edited, sizeof edited));
        assert(project_save() == 0);
        boot(); flush(); assert(!memcmp(&proj.sound, &edited, sizeof edited));
        ui.part = PART_303A + v; ui.view = V_PART; ui.page[V_PART][ui.part] = 2; build_pages();
        page_shot(v, "extra-edited");
        for (int k = 0; k < 4; k++) {
            pref_t r = pg.r[2][k];
            pref_set(r, pref_desc(r)->max); flush();
            panel_turn(EN_K1 + k, 1); assert(pref_get(r) == pref_desc(r)->max);
            pref_set(r, 0); flush();
            panel_turn(EN_K1 + k, -1); assert(pref_get(r) == 0);
            pref_set(r, pref_desc(r)->def); flush();
        }
    }
    /* EXTRA's encoder path records each existing parameter, separately per voice. */
    setup();
    for (int v = 0; v < 2; v++) {
        ui.part = PART_303A + v; ui.view = V_PART; ui.page[V_PART][ui.part] = 2; build_pages();
        ui.rec = 1; seq.playing = 1;
        for (int k = 0; k < 4; k++) {
            int param = params[2][k];
            panel_turn(EN_K1 + k, k == 2 ? -1 : 1);
            int lane = motion_find(proj.arr.lane, 0, ui.part, T_303, v, param);
            motion_step(&mot, ui.part, 0, 0, 1, 16);
            assert(lane >= 0 && proj.arr.lane[lane].val[0] == pref_get(pg.r[2][k]));
        }
        seq.playing = 0; ui.rec = 0;
    }
    /* Voice pages keep the real knob-based one-Morph-owner restriction. */
    setup(); pref_set(wave_ref(0), 2); pref_set(wave_ref(1), 1); flush();
    ui.part = PART_303B; ui.view = V_PART; ui.page[V_PART][ui.part] = 1; build_pages();
    panel_turn(EN_K2, 1); assert(pref_get(wave_ref(1)) == 1);
    panel_tap(B_EDIT); panel_turn(EN_K1, 1);
    assert(pref_get(morph_ref(1)) == 65 && pref_get(wave_ref(1)) == 1 && pref_get(wave_ref(0)) == 2);
    ui.part = PART_303A; build_pages();
    assert(cur_page() == 2); /* independent remembered page */
    panel_turn(EN_SELECT, -1); panel_turn(EN_K2, -1); assert(pref_get(wave_ref(0)) == 1);
    ui.part = PART_303B; build_pages(); assert(cur_page() == 2);
    panel_turn(EN_SELECT, -1); panel_turn(EN_K2, 1); assert(pref_get(wave_ref(1)) == 2);
}
static void selections(void)
{
    setup();
    assert(sizeof(sound_t) == NTARGETS * NVOICES_MAX * 16);
    assert(engine_nparams(T_303, 0) == 13 && BASS303_MORPH == 12);
    const x0x_param_t *p = engine_param(T_303, 0, BASS303_WAVE);
    assert(p->max == 2 && !strcmp(p->names[0], "Saw") && !strcmp(p->names[1], "Square") && !strcmp(p->names[2], "Morph"));
    for (int v = 0; v < 2; v++) {
        ui.view = V_PART; ui.part = PART_303A + v;
        build_pages();
        assert(pg.n == 3 && pg.r[2][0].kind == R_ENG && pg.r[2][0].a == T_303 &&
               pg.r[2][0].b == v && pg.r[2][0].c == BASS303_MORPH);
        assert(pref_get(morph_ref(v)) == 64);
        pref_set(morph_ref(v), v ? 127 : 0);
    }
    flush();
    assert(b303[0].morph == 0 && b303[1].morph == 127);
    /* Requests cannot change UI/save state until the audio consumer accepts. */
    pref_set(wave_ref(0), 2);
    assert(pref_get(wave_ref(0)) == 0 && b303[0].wave == 0);
    flush();
    pref_set(wave_ref(1), 1); flush();
    turn_ref(wave_ref(1), 1, EN_K2); /* real encoder path: Square -> rejected Morph */
    assert(pref_get(wave_ref(1)) == 1);
    assert(project_save() == 0);   /* save before queue drains must not save the request */
    flush();
    assert(b303[0].wave == 2 && b303[1].wave == 1 && pref_get(wave_ref(1)) == 1);
    pref_set(wave_ref(0), 0);
    pref_set(wave_ref(1), 2); flush(); /* release + acquisition ordered in queue */
    assert(b303[0].wave == 0 && b303[1].wave == 2);
    engine_set(T_303, 0, BASS303_WAVE, 2); flush();
    assert(b303[0].wave == 0 && b303[1].wave == 2); /* direct command rejected */
    engine_set(T_303, 1, BASS303_WAVE, 1);
    engine_set(T_303, 0, BASS303_WAVE, 2); flush();
    assert(b303[0].wave == 2 && b303[1].wave == 1);
    engine_set(T_303, 1, BASS303_WAVE, 2); /* consumed rejection has no delayed acquisition */
    flush();
    engine_set(T_303, 0, BASS303_WAVE, 1); flush();
    assert(b303[1].wave == 1);
    /* Burst and full-queue drops also cannot write a rejected Wave to the mirror. */
    for (unsigned i = 0; i < CQ; i++) engine_set(T_303, 0, BASS303_MORPH, 64);
    pref_set(wave_ref(1), 2); flush();
    assert(b303[1].wave == 1 && pref_get(wave_ref(1)) == 1);
}
static void automation(void)
{
    setup();
    pref_set(wave_ref(0), 2); pref_set(wave_ref(1), 1); flush();
    int k = motion_alloc(proj.arr.lane, 0, PART_303B, T_303, 1, BASS303_WAVE);
    assert(k >= 0);
    proj.arr.lane[k].val[0] = 2;
    proj.arr.lane[k].val[1] = 1;
    motion_step(&mot, PART_303B, 0, 0, 1, 16);
    assert(b303[1].wave == 1 && engine_motion_value(k) == 1 && mot.rt[k].applied == 1 && mot.rt[k].ramp == 0);
    engine_set(T_303, 0, BASS303_WAVE, 0); flush();
    motion_tick(&mot, 512);
    assert(b303[1].wave == 1); /* rejected ramp cannot acquire after release */
    motion_step(&mot, PART_303B, 0, 0, 1, 16); /* explicit new step request */
    assert(b303[1].wave == 2 && engine_motion_value(k) == 2);
    motion_release(&mot);
    assert(b303[1].wave == 1);
    /* A base released by motion cannot reappear as a hidden ownership claim. */
    engine_set(T_303, 0, BASS303_WAVE, 2); flush();
    assert(mot_apply(NULL, T_303, 0, BASS303_WAVE, 0) == 0);
    assert(mot_apply(NULL, T_303, 1, BASS303_WAVE, 2) == 2);
    engine_sync_sound(&proj.sound);
    assert(proj.sound.v[T_303][0][BASS303_WAVE] == 0);
    assert(mot_base(NULL, T_303, 0, BASS303_WAVE) == 0);
    assert(mot_apply(NULL, T_303, 0, BASS303_WAVE, 2) == 0);
    /* Recording a rejected UI turn records only the prior accepted base. */
    setup();
    pref_set(wave_ref(0), 2); pref_set(wave_ref(1), 1); flush();
    ui.part = PART_303B; ui.rec = 1; seq.playing = 1;
    turn_ref(wave_ref(1), 1, EN_K2);
    flush();
    k = motion_find(proj.arr.lane, 0, PART_303B, T_303, 1, BASS303_WAVE);
    assert(k >= 0);
    motion_step(&mot, PART_303B, 0, 0, 1, 16);
    assert(proj.arr.lane[k].val[0] == 1 && b303[1].wave == 1);
    seq.playing = 0; ui.rec = 0;
}
static void persistence(void)
{
    setup();
    pref_set(wave_ref(1), 2);
    pref_set(morph_ref(0), 32); pref_set(morph_ref(1), 96); flush();
    assert(project_save() == 0);
    boot(); flush();
    assert(b303[0].wave == 0 && b303[1].wave == 2);
    assert(b303[0].morph_target == 32 && b303[1].morph_target == 96);
    assert(proj.sound.v[T_303][0][12] == 32 && proj.sound.v[T_303][1][12] == 96);
    /* Temporary PERF sound does not overwrite the saved Wave choice. */
    sound_t temporary;
    engine_sound_defaults(&temporary);
    engine_apply_sound(&temporary); flush();
    assert(b303[1].wave == 0 && proj.sound.v[T_303][1][BASS303_WAVE] == 2);
    engine_apply_sound(&proj.sound); flush();
    assert(b303[1].wave == 2);
    /* Malformed experimental sound: no revision migration, simply A first. */
    proj.sound.v[T_303][0][BASS303_WAVE] = 2;
    proj.sound.v[T_303][1][BASS303_WAVE] = 2;
    proj.sound.v[T_303][0][BASS303_MORPH] = 255;
    assert(save_obj(OBJ_SOUND, &proj.sound, sizeof(proj.sound)) == 0);
    boot(); flush();
    assert(b303[0].wave == 2 && b303[1].wave == 0);
    assert(proj.sound.v[T_303][0][BASS303_WAVE] == 2 && proj.sound.v[T_303][1][BASS303_WAVE] == 0);
    assert(b303[0].morph_target == 127 && b303[1].morph_target == 96);
    assert(project_save() == 0);
    boot(); flush();
    assert(b303[0].wave == 2 && b303[1].wave == 0);
    ui.view = V_PART; ui.part = PART_303A; ui.page[V_PART][PART_303A] = 1;
    build_pages(); draw_header(); draw_main(); draw_knobs();
    png_write("build/host/waveform-ui/303-morph.png");
    open_list_of_pages();
    int row = -1;
    for (int i = 0; i < ui.list_n; i++) if (list_rows[i].kind == R_ENG && list_rows[i].a == T_303 && list_rows[i].c == BASS303_MORPH) row = i;
    assert(row >= 0);
    ui.list_sel = row; ui.list_top = row > 4 ? row - 4 : 0;
    accel_off = 1;
    turn_ref(list_rows[row], -1, EN_SELECT); flush();
    accel_off = 0;
    assert(proj.sound.v[T_303][0][12] == 126 && b303[0].morph_target == 126);
    draw_header(); draw_main(); draw_knobs();
    png_write("build/host/waveform-ui/303-sel-morph.png");
}
int main(void)
{
    extra_pages(); selections(); automation(); persistence();
    puts("bass303 EXTRA pages/knobs/SEL/Undo and command exclusion/motion/persistence: PASS");
    return 0;
}
