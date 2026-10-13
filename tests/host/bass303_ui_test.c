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
        int found = 0;
        for (int i = 0; i < pg.nmore; i++)
            found |= pg.more[i].kind == R_ENG && pg.more[i].a == T_303 && pg.more[i].b == v && pg.more[i].c == BASS303_MORPH;
        assert(found);
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
    png_write("build/host/waveform-ui/303-more.png");
}
int main(void)
{
    selections(); automation(); persistence();
    puts("bass303 UI/command exclusion/motion/persistence: PASS");
    return 0;
}
