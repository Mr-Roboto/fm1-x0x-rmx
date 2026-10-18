/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual UI input, command queue, motion and project/Undo paths for ENV reset. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/engine.c"
#include <assert.h>

static void flush(void) { drain(); engine_sync_sound(&proj.sound); }
static void setup(int part, int view, int page)
{
    held_btn = held_keys = 0; memset(enc_acc, 0, sizeof enc_acc);
    memset(&ui, 0, sizeof ui);
    project_defaults(); drum_gen_init(0);
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    engine_apply_sound(&proj.sound); flush(); ui_init();
    ui.part = (uint8_t)part; ui.view = (uint8_t)view;
    *page_of(view) = (uint8_t)page; build_pages();
}
static void turn(int knob, int amount)
{
    accel_off = 1; enc_acc[EN_K1 + knob] = amount; ui_input_only(); accel_off = 0; flush();
}
static void reset(int knob, int amount)
{
    held_btn = 1u << B_ENV; ui_input_only();
    turn(knob, amount);
    held_btn = 0; ui_input_only(); flush();
}
static void changed_reset(int knob)
{
    pref_t r = pg.r[cur_page()][knob];
    x0x_param_t d = *pref_desc(r);
    pref_set(r, d.def ? 0 : d.max); flush();
    assert(pref_get(r) != d.def);
    ui.dirty = 0;
    reset(knob, -9);
    assert(pref_get(r) == d.def && ui.dirty);
}

static void ordinary(void)
{
    setup(PART_909, V_PART, 0);
    /* The factory kick Decay is 80; its descriptor default is 90. */
    assert(pref_get(pg.r[0][2]) == 80 && pref_desc(pg.r[0][2])->def == 90);
    for (int k = 0; k < 4; ++k) changed_reset(k);
    setup(PART_808, V_PART, 0);
    for (int k = 0; k < 4; ++k) changed_reset(k);
    setup(PART_303A, V_PART, 0);
    for (int k = 0; k < 4; ++k) changed_reset(k);
    setup(PART_BRK, V_PART, 1); changed_reset(0);
    assert(proj.sound.v[T_BRK][0][2] == 64 && breaks_get(&brk, BRK_FILT) == 64);
    changed_reset(1);
    setup(PART_909, V_FX, 0);
    for (int k = 0; k < 4; ++k) if (pref_desc(pg.r[cur_page()][k])) changed_reset(k);
    setup(PART_909, V_MIX, 0);
    for (int k = 0; k < 4; ++k) changed_reset(k);
    puts("PASS: all four knob positions, both turn directions, 909/808/303/BREAK/FX/mix and descriptor rather than factory-mix defaults");
}

static void custom(void)
{
    for (int voice = 0; voice < 2; ++voice) {
        setup(PART_303A + voice, V_PART, 2);
        engine_bass_wave_user(voice, 2); flush();
        for (int shape = 0; shape <= 127; shape += 127) {
            pref_set(pg.r[2][0], shape); flush(); reset(0, 1);
            assert(b303[voice].morph_target == 64 && engine_bass_wave(voice) == 2);
        }
        ui.page[V_PART][ui.part] = 1; build_pages();
        reset(1, 1); assert(engine_bass_wave(voice) == 0 && proj.sound.v[T_303][voice][BASS303_WAVE] == 0);
        engine_bass_wave_user(voice ^ 1, 2); flush();
        engine_bass_wave_user(voice, 1); flush();
        turn(1, 1); /* Original Wave path still rejects acquiring the peer's Morph. */
        uint32_t rejected = eng_wave_rejected;
        ui_frame();
        assert(engine_bass_wave(voice) == 1 && !strcmp(ui.msg, "MORPH IN USE"));
        reset(1, -1);
        assert(engine_bass_wave(voice) == 0 && engine_bass_wave(voice ^ 1) == 2);
        assert(eng_wave_rejected == rejected);
    }
    for (int part = 0; part < 4; ++part) {
        setup(part, V_SEQ, 1);
        assert(pg.r[1][0].kind == R_RAPPLY && pg.r[1][1].kind == R_RBANK);
        pref_set(pg.r[1][0], REMIX_LOOP); pref_set(pg.r[1][1], 5);
        pref_set(pg.r[1][2], 7); pref_set(pg.r[1][3], 1);
        reset(1, 1);
        assert(pattern_remix_selection(cur_pat(), part) == 7 && pattern_remix_enabled(cur_pat(), part));
        reset(2, -1);
        assert(pattern_remix_selection(cur_pat(), part) == 0 && pattern_remix_enabled(cur_pat(), part));
        reset(3, 1); assert(!pattern_remix_enabled(cur_pat(), part));
        reset(0, -1); assert(settings_remix_apply(&proj.set) == REMIX_BEAT && seq.remix_apply == REMIX_BEAT);
    }
    setup(PART_303A, V_SEQ, 0);
    pattern_set_remix_selection(cur_pat(), PART_303A, 5, 7);
    pattern_set_remix_enabled(cur_pat(), PART_303A, 1);
    pref_set(pg.r[0][2], DIR_MREV); reset(2, 1);
    assert(cur_pat()->bass[0].dir == DIR_FWD);
    assert(pattern_remix_selection(cur_pat(), PART_303A) == 0x57 && pattern_remix_enabled(cur_pat(), PART_303A));
    setup(PART_303A, V_GEN, 1); changed_reset(1); /* custom scale default */
    setup(PART_909, V_GEN, 0);
    pref_t r = pg.r[0][0]; int def = pref_desc(r)->def;
    pref_set(r, def ? 0 : 1); ui.dirty = 0; reset(0, 1);
    assert(pref_get(r) == def && !ui.dirty); /* generator session stays nonpersistent */
    puts("PASS: both Waves/Morph ownership, custom Morph reset, Remix independence/APPLY, MREV to FWD, TB3PO and Grids defaults");
}

static void unchanged(void)
{
    setup(PART_BRK, V_PART, 1);
    project_t before = proj;
    uint32_t writes = store_writes, queued = cq_w;
    ui.dirty = 0; seq.playing = 1; ui.rec = 1;
    reset(0, 1); /* Filter is already 64: no dirty, queue write or motion lane. */
    assert(!memcmp(&before, &proj, sizeof proj) && !ui.dirty && cq_w == queued);
    assert(motion_count(proj.arr.lane) == 0 && store_writes == writes);
    reset(2, 1); reset(3, -1); /* NONE, not accidental actions */
    assert(!memcmp(&before, &proj, sizeof proj) && !ui.dirty && cq_w == queued);
    seq.playing = 0; ui.rec = 0;
    ui.overlay = O_ASK; held_btn = 1u << B_ENV; ui_input_only(); turn(0, 1);
    assert(!memcmp(&before, &proj, sizeof proj));
    held_btn = 0; ui_input_only();
    puts("PASS: already-default and blank controls cause no dirty/save/queue/motion changes; question overlay keeps knobs blocked");
}

static void motion_and_undo(void)
{
    for (int recording = 0; recording < 2; ++recording) {
        setup(PART_303A, V_PART, 0);
        pref_t r = pg.r[0][0]; pref_set(r, 10); flush();
        int lane = motion_alloc(proj.arr.lane, 0, ui.part, r.a, r.b, r.c); assert(lane >= 0);
        proj.arr.lane[lane].val[0] = 20;
        seq.playing = 1; ui.rec = recording;
        reset(0, 1);
        assert(pref_get(r) == 64);
        assert(recording ? mot.rt[lane].rec_req : mot.rt[lane].hold_req);
        motion_step(&mot, ui.part, 0, 0, 1, 16);
        if (recording) assert(proj.arr.lane[lane].val[0] == 64);
        else assert(mot.rt[lane].hold_left > 0 && proj.arr.lane[lane].val[0] == 20);
    }
    setup(PART_303A, V_PART, 0);
    pref_t recorded = pg.r[0][0]; pref_set(recorded, 10); flush();
    seq.playing = 1; ui.rec = 1; reset(0, 1);
    int allocated = motion_find(proj.arr.lane, 0, ui.part, recorded.a, recorded.b, recorded.c);
    assert(allocated >= 0 && mot.rt[allocated].rec_req);
    motion_step(&mot, ui.part, 0, 0, 1, 16);
    assert(proj.arr.lane[allocated].val[0] == 64);
    setup(PART_303A, V_PART, 0);
    pref_t r = pg.r[0][0]; pref_set(r, 10); flush(); undo_start();
    reset(0, 1); do_undo(0); flush(); assert(pref_get(r) == 10);
    do_undo(1); flush(); assert(pref_get(r) == 64);
    assert(project_save() == 0); project_defaults(); assert(project_load() == 0);
    assert(proj.sound.v[T_303][0][BASS303_CUTOFF] == 64);
    puts("PASS: reset records or holds motion using upstream semantics, Undo/Redo and save/load");
}

static void priorities_and_env(void)
{
    setup(PART_303A, V_PART, 0);
    pref_t r = pg.r[0][0]; pref_set(r, 10); flush();
    ui.held_step = 0; reset(0, 1);
    assert(pref_get(r) == 10 && lock_of(r, 0) == 11); /* p-lock wins over ENV */
    held_btn = (1u << B_ENV) | (1u << B_SAVE); ui_input_only(); turn(0, 1);
    assert(lock_of(r, 0) == -1 && pref_get(r) == 10); /* SAVE clears held lock */
    held_btn = 0; ui_input_only(); ui.held_step = -1;
    int lane = motion_alloc(proj.arr.lane, 0, ui.part, r.a, r.b, r.c); assert(lane >= 0);
    proj.arr.lane[lane].val[0] = 20;
    held_btn = (1u << B_ENV) | (1u << B_SAVE); ui_input_only(); turn(0, 1);
    assert(!proj.arr.lane[lane].used && pref_get(r) == 10); /* SAVE forgets whole lane */
    held_btn = 0; ui_input_only();
    setup(PART_303A, V_SEQ, 0);
    ui.held_step = 0; cur_pat()->bass[0].step[0].note = 48;
    reset(0, 1); assert(cur_pat()->bass[0].step[0].note == 49 && ui.step_edited);
    setup(PART_909, V_SEQ, 0);
    held_btn = 1u << B_ENV; ui_input_only(); drum_step(0, 1);
    assert(sm_get(cur_pat()->drum[0].accent, 0));
    held_btn = 0; ui_input_only();
    setup(PART_303A, V_SEQ, 0); ui.rec = 1;
    held_btn = 1u << B_ENV; ui_input_only(); bass_kbd(0, 1); flush();
    assert(cur_pat()->bass[0].step[0].flags & BS_ACCENT);
    button_tap(B_OCTUP); assert(bstep_gate(&cur_pat()->bass[0].step[1]) == G_REST);
    held_btn = 0; ui_input_only(); bass_kbd(0, 0); flush();
    const char *lines[HELP_LINES]; int n = help_lines(B_ENV, lines), found = 0;
    for (int i = 0; i < n; ++i) if (!strcmp(lines[i], "HOLD + KNOB: ITS DEFAULT")) found = 1;
    assert(found && n <= HELP_LINES && help_fits());
    puts("PASS: held-step, p-lock and SAVE priorities; ENV drum/keyboard accents, octave rest and hold help");
}

int main(void)
{
    ordinary(); custom(); unchanged(); motion_and_undo(); priorities_and_env();
    return 0;
}
