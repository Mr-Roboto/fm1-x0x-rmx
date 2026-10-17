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
static void directions(void)
{
    setup();
    static const char *const names[] = {"FWD", "REV", "MREV", "PING", "RND"};
    assert(sizeof(bpart_t) == 144 && sizeof(pattern_t) == 528 && sizeof(project_t) == 30952);
    assert(DIR_MREV == 2 && DIR_PINGPONG == 3 && DIR_RANDOM == 4);
    for (int v = 0; v < 2; v++) {
        ui.part = PART_303A + v; ui.view = V_SEQ;
        ui.page[V_SEQ][ui.part] = 0; build_pages();
        assert(pg.r[0][2].kind == R_BDIR);
        pref_t r = PR(R_BDIR, v, 0, 0);
        assert(pref_desc(r)->max == 4);
        pref_set(r, DIR_FWD);
        for (int d = 0; d < NDIRS; d++) {
            char value[16], unit[8];
            pref_value(r, value, unit);
            assert(pref_get(r) == d && !strcmp(value, names[d]));
            if (d < NDIRS - 1) panel_turn(EN_K3, 1);
        }
        open_list_of_pages();
        int row = -1;
        for (int i = 0; i < ui.list_n; i++) if (list_rows[i].kind == R_BDIR) row = i;
        assert(row >= 0); ui.list_sel = row;
        for (int d = NDIRS - 1; d > 0; d--) {
            panel_turn(EN_ALGO, -1);
            assert(pref_get(r) == d - 1);
        }
        ui.overlay = O_NONE;
        for (int d = 0; d < NDIRS; d++) {
            pref_set(r, d);
            pattern_set_remix_selection(&proj.pat[0], ui.part, 5, 7);
            pattern_set_remix_enabled(&proj.pat[0], ui.part, 1);
            assert(project_save() == 0);
            project_defaults(); assert(project_load() == 0);
            assert(proj.pat[0].bass[v].dir == d);
            assert(copy_part(1, 0, ui.part) == 0 && proj.pat[1].bass[v].dir == d);
            undo_start(); pref_set(r, (d + 1) % NDIRS);
            do_undo(0); assert(pref_get(r) == d);
            do_undo(1); assert(pref_get(r) == (d + 1) % NDIRS);
        }
    }
    /* Legacy raw values are deliberately reinterpreted, never migrated. */
    proj.pat[0].bass[0].dir = 2; proj.pat[0].bass[1].dir = 3;
    assert(project_save() == 0); project_defaults(); assert(project_load() == 0);
    assert(proj.pat[0].bass[0].dir == DIR_MREV && proj.pat[0].bass[1].dir == DIR_PINGPONG);
    puts("Directions: five ordered knob/SEL choices, unchanged sizes, raw legacy values, save/load/copy/Undo passed");
}

static void mrev_probability(void)
{
    static const int lengths[] = {1, 8, 16, 17, 32, 64};
    for (unsigned l = 0; l < sizeof lengths / sizeof lengths[0]; l++)
        for (int v = 0; v < NBASS; v++) {
            setup();
            int part = PART_303A + v, len = lengths[l];
            proj.pat[0].bass[v].dir = DIR_MREV; proj.pat[0].bass[v].len = len;
            int k = motion_alloc(proj.arr.lane, 0, part, T_PROB, part << 4, 0);
            assert(k >= 0);
            memset(proj.arr.lane[k].val, PROB_COND, NSTEPS); /* 1:2 on every source step */
            seq_set_remix(&seq, part, 0, 6, 1);
            seq.ext = 1; seq_start(&seq);
            seq_ext_clock(&seq); seq_advance(&seq, 1, &SINK);
            for (int n = 0; n < len * 3; n++) {
                assert(seq.t[part].bars == (unsigned)(n / len));
                assert(prob_skip(part, 0) == ((n / len) & 1));
                for (int tick = 0; tick < 6; tick++) {
                    seq_ext_clock(&seq); seq_advance(&seq, 1, &SINK);
                }
            }
        }
    puts("MREV conditional probability: 1:2 follows complete passes at all six lengths on both voices");
}

static void conflict_notice(void)
{
    uint16_t ordinary[240 * HDR_H], warning[240 * HDR_H];
    for (int owner = 0; owner < 2; owner++) {
        for (int list = 0; list < 2; list++) {
            setup();
            int voice = owner ^ 1;
            pref_set(wave_ref(owner), 2); pref_set(wave_ref(voice), 1); flush();
            ui.part = PART_303A + voice; ui.view = V_PART;
            ui.page[V_PART][ui.part] = 1; ui.msg_until = 0; build_pages();
            if (list) {
                panel_tap(B_SEL);
                int row = -1;
                for (int i = 0; i < ui.list_n; i++)
                    if (list_rows[i].kind == R_ENG && list_rows[i].c == BASS303_WAVE) row = i;
                assert(row >= 0); ui.list_sel = row;
            }
            ui_frame(); memcpy(ordinary, fb, sizeof ordinary);
            uint32_t count = eng_wave_rejected;
            panel_turn(list ? EN_ALGO : EN_K2, 1);
            assert(eng_wave_rejected == count + 1);
            assert(pref_get(wave_ref(voice)) == 1 && b303[owner].wave == 2);
            ui_frame();
            assert(!strcmp(ui.msg, "MORPH IN USE") && ui.msg_until == plat_ms() + 1300u);
            assert(list ? ui.overlay == O_LIST : ui.touched == 1);
            memcpy(warning, fb, sizeof warning);
            assert(memcmp(warning, ordinary, sizeof warning));
            draw_main(); draw_knobs();
            assert(!memcmp(warning, fb, sizeof warning)); /* readout/list cannot obscure header */
            char path[96];
            snprintf(path, sizeof path, "build/host/waveform-ui/conflict-%c-%s.png", voice ? 'b' : 'a', list ? "sel" : "knob");
            png_write(path);
            uint32_t until = ui.msg_until;
            now_ms += 100; ui_frame(); assert(ui.msg_until == until);
            /* Ordinary commands, motion and loading cannot generate UI notices. */
            engine_set(T_303, voice, BASS303_WAVE, 2); flush();
            mot_apply(NULL, T_303, voice, BASS303_WAVE, 2);
            assert(project_save() == 0); assert(project_load() == 0);
            engine_apply_sound(&proj.sound); flush(); ui_frame();
            assert(eng_wave_rejected == count + 1 && ui.msg_until == until);
            now_ms = until + 1; ui_frame(); assert(ui.msg_until == 0);
            /* Queue order wins over the UI's stale mirror: release then select succeeds. */
            engine_set(T_303, owner, BASS303_WAVE, 0);
            turn_ref(wave_ref(voice), 1, EN_K2); flush(); ui_frame();
            assert(b303[voice].wave == 2 && eng_wave_rejected == count + 1 && !ui.msg_until);
            /* A motion-owned effective Morph also excludes a fresh user request. */
            engine_set(T_303, voice, BASS303_WAVE, 1); flush();
            assert(mot_apply(NULL, T_303, owner, BASS303_WAVE, 2) == 2);
            assert(engine_bass_wave(owner) == 0);
            turn_ref(wave_ref(voice), 1, EN_K2); flush(); ui_frame();
            assert(eng_wave_rejected == count + 2 && pref_get(wave_ref(voice)) == 1);
            assert(!strcmp(ui.msg, "MORPH IN USE"));
        }
    }
    puts("MORPH IN USE: symmetric knob/SEL, actual framebuffer visibility, timeout, silence and queued release passed");
}

int main(void)
{
    extra_pages(); selections(); automation(); persistence();
    directions(); mrev_probability(); conflict_notice();
    puts("bass303 EXTRA pages/knobs/SEL/Undo and command exclusion/motion/persistence: PASS");
    return 0;
}
