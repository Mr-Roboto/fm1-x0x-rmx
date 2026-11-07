/* SPDX-License-Identifier: GPL-3.0-only */
/* Real hat UI, p-locks, motion, trigger audio and saved-project migration. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/engine.c"
#include <assert.h>

static void flush(void) { drain(); engine_sync_sound(&proj.sound); }
static void setup(int kit, int voice)
{
    held_btn = held_keys = 0;
    memset(enc_acc, 0, sizeof enc_acc);
    memset(&ui, 0, sizeof ui);
    project_defaults();
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    engine_apply_sound(&proj.sound); flush(); ui_init();
    ui.part = (uint8_t)kit; ui.view = V_PART; ui.sel[kit] = (uint8_t)voice;
    *page_of(V_PART) = 0; build_pages();
}
static void knob3(int amount)
{
    accel_off = 1; enc_acc[EN_K3] = amount; ui_input_only(); accel_off = 0; flush();
}
static void stock_hat_migration(void)
{
    setup(PART_909, DR_OH);
    proj.set.snd_rev = 6;
    proj.sound.v[T_909][DR_CH][HAT_VELOCITY] = 0;
    proj.sound.v[T_909][DR_OH][HAT_VELOCITY] = 0;
    proj.sound.v[T_808][D8_CH][HAT_VELOCITY] = 0;
    proj.sound.v[T_808][D8_OH][HAT_VELOCITY] = 0;
    assert(project_save() == 0);
    /* Genuine HARDER revision 6 zeros remain deliberate silence. */
    assert(project_load() == 0);
    assert(proj.sound.v[T_909][DR_CH][HAT_VELOCITY] == 0);
    assert(proj.sound.v[T_808][D8_OH][HAT_VELOCITY] == 0);
    proj.set.snd_rev = 6;
    assert(project_save() == 0);
    store_len[OBJ_SET] = 36; /* Stock 1.2.1: format word plus 32-byte settings. */
    sound_t expected = proj.sound;
    expected.v[T_909][DR_CH][HAT_VELOCITY] = 127;
    expected.v[T_909][DR_OH][HAT_VELOCITY] = 127;
    expected.v[T_808][D8_CH][HAT_VELOCITY] = 127;
    expected.v[T_808][D8_OH][HAT_VELOCITY] = 127;
    assert(project_load() == 0);
    assert(!memcmp(&proj.sound, &expected, sizeof expected));
    engine_apply_sound(&proj.sound); flush();
    run_action(ACT_CLEAR_PAT, 0); flush();
    assert(!memcmp(&proj.sound, &expected, sizeof expected));
    assert(hat_velocity[0][0] == 127 && hat_velocity[0][1] == 127);
    assert(hat_velocity[1][0] == 127 && hat_velocity[1][1] == 127);
    /* Clearing notes retains chosen sound values, including deliberate silence. */
    proj.sound.v[T_909][DR_CH][HAT_VELOCITY] = 53;
    proj.sound.v[T_808][D8_CH][HAT_VELOCITY] = 0;
    engine_apply_sound(&proj.sound); flush();
    run_action(ACT_CLEAR_PAT, 0); flush();
    assert(hat_velocity[0][0] == 53 && hat_velocity[1][0] == 0);
    assert(project_save() == 0 && project_load() == 0);
    assert(proj.sound.v[T_909][DR_CH][HAT_VELOCITY] == 53);
    assert(proj.sound.v[T_808][D8_CH][HAT_VELOCITY] == 0);
    puts("stock revision 6 migration and Clear Pattern sound preservation passed");
}
static double hit_energy(int kit, int voice, int value, int depth)
{
    float dry[256], rev[256], dly[256];
    double power = 0;
    setup(kit, voice);
    pref_set(PR(R_ENG, kit, 11, 1), depth); flush();
    pref_set(pg.r[0][2], value); flush();
    s_drum(0, kit, voice, 1.0f);
    for (int b = 0; b < 12; b++) {
        memset(dry, 0, sizeof dry); memset(rev, 0, sizeof rev); memset(dly, 0, sizeof dly);
        if (kit == PART_909) drum909_render(&d909, dry, rev, dly, 256);
        else drum808_render(&d808, dry, rev, dly, 256);
        for (int i = 0; i < 256; i++) power += (double)dry[i] * dry[i];
    }
    return power;
}

static void check_hat(int kit, int voice)
{
    setup(kit, voice);
    pref_t r = pg.r[0][2];
    assert(r.kind == R_ENG && r.a == kit && r.b == voice && r.c == HAT_VELOCITY);
    assert(name_is(pref_desc(r), "Velocity") && pref_get(r) == 127);
    if (kit == PART_909 && voice == DR_OH) {
        assert(pg.r[0][3].kind == R_ENG && pg.r[0][3].c == OH_HOLD);
        assert(engine_nparams(kit, voice) == 10);
    } else {
        assert(pg.r[0][3].kind == R_NONE);
        assert(engine_nparams(kit, voice) == 9);
    }
    assert(name_is(engine_param(kit, voice, 7), "Pan"));
    run_action(ACT_CLEAR_PAT, 0); flush();
    assert(pref_get(r) == 127 && hat_velocity[kit][hat_slot(kit, voice)] == 127);
    run_action(ACT_RESET, 0); flush();
    assert(pref_get(r) == 127 && hat_velocity[kit][hat_slot(kit, voice)] == 127);
    assert(pg.r[0][0].c != HAT_VELOCITY && pg.r[0][1].c != HAT_VELOCITY);
    knob3(-27); assert(pref_get(r) == 100 && hat_velocity[kit][hat_slot(kit, voice)] == 100);
    assert(hat_velocity[kit][hat_slot(kit, voice) ^ 1] == 127);
    assert(hat_velocity[kit ^ 1][0] == 127 && hat_velocity[kit ^ 1][1] == 127);
    held_btn = 1u << B_ENV; ui_input_only(); knob3(1);
    held_btn = 0; ui_input_only(); assert(pref_get(r) == 127);

    /* A real held step plus knob 3 creates a lock without changing the base. */
    drum_step(0, 1); knob3(-63);
    int lane = motion_find(proj.arr.lane, 0, kit, kit, voice, HAT_VELOCITY);
    assert(lane >= 0 && proj.arr.lane[lane].val[0] == 64);
    assert(proj.arr.lane[lane].val[1] == MOT_NONE && pref_get(r) == 127);
    drum_step(0, 0); flush();
    assert(sm_get(proj.pat[0].drum[kit].hit[voice], 0));
    assert(hat_velocity[kit][hat_slot(kit, voice)] == 127);
    motion_step(&mot, kit, 0, 0, 1, 16);
    assert(hat_velocity[kit][hat_slot(kit, voice)] == 64);
    motion_step(&mot, kit, 0, 1, 2, 16);
    assert(hat_velocity[kit][hat_slot(kit, voice)] == 127);
    motion_release(&mot);
    assert(project_save() == 0); project_defaults(); assert(project_load() == 0);
    assert(proj.sound.v[kit][voice][HAT_VELOCITY] == 127 && proj.arr.lane[lane].val[0] == 64);

    /* Zero is persistent and distinguishable from an old unused zero byte. */
    proj.sound.v[kit][voice][HAT_VELOCITY] = 0;
    assert(project_save() == 0); project_defaults(); assert(project_load() == 0);
    assert(proj.sound.v[kit][voice][HAT_VELOCITY] == 0);
    proj.set.snd_rev = 5;
    proj.sound.v[T_909][DR_CH][HAT_VELOCITY] = 0;
    proj.sound.v[T_909][DR_OH][HAT_VELOCITY] = 42;
    proj.sound.v[T_808][D8_CH][HAT_VELOCITY] = 255;
    proj.sound.v[T_808][D8_OH][HAT_VELOCITY] = 1;
    assert(project_save() == 0);
    project_t expected = proj; expected.set.snd_rev = PROJ_SND_REV;
    expected.sound.v[T_909][DR_CH][HAT_VELOCITY] = 127;
    expected.sound.v[T_909][DR_OH][HAT_VELOCITY] = 127;
    expected.sound.v[T_808][D8_CH][HAT_VELOCITY] = 127;
    expected.sound.v[T_808][D8_OH][HAT_VELOCITY] = 127;
    assert(project_load() == 0 && !memcmp(&proj, &expected, sizeof proj));

    /* Recorded motion reaches the same audio-owned pot; SAVE clears locks. */
    setup(kit, voice); r = pg.r[0][2]; seq.playing = 1; ui.rec = 1; knob3(-47);
    lane = motion_find(proj.arr.lane, 0, kit, kit, voice, HAT_VELOCITY);
    assert(lane >= 0 && mot.rt[lane].rec_req);
    motion_step(&mot, kit, 0, 0, 1, 16); assert(proj.arr.lane[lane].val[0] == 80);
    seq.playing = 0; ui.rec = 0; motion_release(&mot);
    drum_step(0, 1); held_btn = 1u << B_SAVE; ui_input_only(); knob3(1);
    assert(!proj.arr.lane[lane].used); held_btn = 0; ui_input_only(); drum_step(0, 0); flush();

    setup(kit, voice); r = pg.r[0][2]; undo_start(); knob3(-31);
    assert(pref_get(r) == 96); do_undo(0); flush(); assert(pref_get(r) == 127);
    do_undo(1); flush(); assert(pref_get(r) == 96);

    const int depths[] = {0, 34, 127}, values[] = {0, 1, 16, 32, 64, 96, 127};
    for (unsigned d = 0; d < sizeof depths / sizeof *depths; d++) {
        double full = hit_energy(kit, voice, 127, depths[d]), last = -1;
        assert(full > 0);
        for (unsigned j = 0; j < sizeof values / sizeof *values; j++) {
            double power = hit_energy(kit, voice, values[j], depths[d]);
            double ratio = (double)values[j] / 127.0;
            assert(power > last);
            if (fabs(power / full - ratio * ratio) >= 0.0001)
                fprintf(stderr, "kit=%d voice=%d depth=%d velocity=%d power/full=%g expected=%g\n",
                        kit, voice, depths[d], values[j], power / full, ratio * ratio);
            assert(fabs(power / full - ratio * ratio) < 0.0001);
            last = power;
        }
    }
    printf("hat velocity: kit %d voice %d, knob 3/locks/motion/undo/save/migration and 7 audio levels at 3 kit sensitivities passed\n", kit, voice);
}
static void render_909(int frames, int block)
{
    float dry[256], rev[256], dly[256];
    while (frames > 0) {
        int n = frames < block ? frames : block;
        memset(dry, 0, sizeof dry); memset(rev, 0, sizeof rev); memset(dly, 0, sizeof dly);
        drum909_render(&d909, dry, rev, dly, n);
        frames -= n;
    }
}

static void open_hat_hold(void)
{
    for (int block = 1; block <= 256; block = block == 1 ? 17 : 256) {
        setup(PART_909, DR_OH);
        drum909_trigger(&d909, DR_OH, 0.5f);
        d9_smp_t *s = &d909.smp[DR_OH - DR_CH];
        float initial = s->out.v;
        int decay = s->out.k, old_mute = s->mute;
        drum909_open_hat_hold(&d909, 100);
        assert(s->out.left == -4410 && s->mute == old_mute + 4410);
        render_909(4409, block);
        assert(s->out.left == -1 && s->out.v == initial && s->out.cur == initial);
        render_909(1, 1); assert(s->out.left == decay && s->out.v == initial);
        render_909(2, 1); assert(s->out.cur < initial && s->out.v < initial);
        if (block == 256) break;
    }
    setup(PART_909, DR_OH);
    pref_t hold = pg.r[0][3];
    assert(name_is(pref_desc(hold), "Hold") && pref_get(hold) == 0);
    assert(engine_oh_hold_ms(0) == 0 && engine_oh_hold_ms(127) == 500);
    char number[16], unit[8];
    pref_value_of(hold, 127, number, unit); assert(!strcmp(number, "500") && !strcmp(unit, "ms"));
    drum_step(0, 1); accel_off = 1; lock_turn(hold, 32, EN_K4); accel_off = 0; flush();
    int lane = motion_find(proj.arr.lane, 0, PART_909, T_909, DR_OH, OH_HOLD);
    assert(lane >= 0 && proj.arr.lane[lane].val[0] == 32 && pref_get(hold) == 0);
    drum_step(0, 0); flush();
    motion_step(&mot, PART_909, 0, 0, 1, 16); assert(oh_hold == 32);
    s_drum(0, PART_909, DR_OH, 1.0f);
    d9_smp_t *s = &d909.smp[DR_OH - DR_CH];
    assert(s->out.left == -(engine_oh_hold_ms(32) * 44100 + 500) / 1000);
    motion_step(&mot, PART_909, 0, 1, 2, 16); assert(oh_hold == 0);
    assert(s->out.left < 0); /* changing the control doesn't rewrite the current hit */
    s_drum(0, PART_909, DR_OH, 1.0f); assert(s->out.left > 0);
    motion_release(&mot);

    pref_set(hold, 100); flush(); s_drum(0, PART_909, DR_OH, 1.0f);
    render_909(37, 17); assert(s->out.left < 0);
    s_drum(0, PART_909, DR_CH, 1.0f); assert(s->out.left > 0 && s->mute < 300);
    render_909(300, 17); assert(s->mute == 0 && s->out.left == 0);
    pref_set(hold, 25); flush(); s_drum(0, PART_909, DR_OH, 0.5f);
    assert(s->pos == 0 && s->out.left == -(engine_oh_hold_ms(25) * 44100 + 500) / 1000);
    assert(s->out.v < s->volume * d909.accent);
    assert(project_save() == 0); project_defaults(); assert(project_load() == 0);
    assert(proj.sound.v[T_909][DR_OH][OH_HOLD] == 25 && proj.arr.lane[lane].val[0] == 32);

    proj.set.snd_rev = 6; proj.sound.v[T_909][DR_OH][OH_HOLD] = 99;
    proj.sound.v[T_909][DR_OH][HAT_VELOCITY] = 41;
    assert(project_save() == 0); assert(project_load() == 0);
    assert(proj.set.snd_rev == PROJ_SND_REV && proj.sound.v[T_909][DR_OH][OH_HOLD] == 0);
    assert(proj.sound.v[T_909][DR_OH][HAT_VELOCITY] == 41);

    setup(PART_909, DR_OH); hold = pg.r[0][3]; undo_start();
    accel_off = 1; enc_acc[EN_K4] = 40; ui_input_only(); accel_off = 0; flush();
    assert(pref_get(hold) == 40); do_undo(0); flush(); assert(pref_get(hold) == 0);
    do_undo(1); flush(); assert(pref_get(hold) == 40);
    held_btn = 1u << B_ENV; ui_input_only(); enc_acc[EN_K4] = 1; ui_input_only();
    held_btn = 0; ui_input_only(); flush(); assert(pref_get(hold) == 0);
    puts("909 OH hold: exact hold/decay timing at 1/17/256-frame blocks, choke, retrigger, velocity, locks, save/migration and Undo passed");
}

int main(void)
{
    assert(sizeof(sound_t) == 1344 && sizeof(pattern_t) == 528 && RAW_LEN == 29572);
    check_hat(PART_909, DR_CH); check_hat(PART_909, DR_OH);
    check_hat(PART_808, D8_CH); check_hat(PART_808, D8_OH);
    open_hat_hold();
    stock_hat_migration();
    puts("hat velocity: all four hats passed; persistent sizes unchanged");
    return 0;
}
