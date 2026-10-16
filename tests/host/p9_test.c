/* SPDX-License-Identifier: GPL-3.0-only */
/* P9: actual panel inputs, storage migration, deferred SAVE and automatic playback. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>

static void turn(int role, int amount)
{
    accel_off = 1;
    enc_acc[role] = amount;
    ui_input_only();
    accel_off = 0;
    run_ms(10);
}
static void combination(const uint8_t *want)
{
    for (int p = 0; p < 5; p++) {
        assert(proj.set.start_pat[p] == want[p]);
        assert(seq.ppat[p] == want[p]);
    }
}
static void restart(const uint8_t *want)
{
    boot();
    run_ms(10);
    combination(want);
}
int main(void)
{
    static const uint8_t zero[5] = {0}, chosen[5] = {1,2,3,4,5}, home[5] = {7,7,7,7,7};
    boot(); run_ms(10);
    assert(proj.set.usb_full == 1);
    unsigned before = store_writes;
    for (int p = 0; p < 5; p++) {
        ui.part = p;
        set_view(V_SEQ);
        for (int n = 0; n <= p; n++) turn(EN_PRESET, 1);
    }
    combination(chosen);
    assert(store_writes == before && ui_dirty());
    assert(project_save() == 0);
    restart(chosen);
    /* Legacy full settings (28 bytes), short settings, invalid values. */
    store_len[OBJ_SET] = 32;
    restart(zero);
    assert(project_save() == 0);
    store_len[OBJ_SET] = 20;
    restart(zero);
    assert(proj.set.usb_full == 1);
    proj.set.usb_full = 0;
    proj.set.start_pat[0] = 255;
    assert(project_save() == 0);
    restart(zero);
    assert(proj.set.usb_full == 0);       /* saved FOLLOWS KNOB survives */
    run_action(ACT_RESET, 0);
    assert(proj.set.usb_full == 1);
    assert(project_save() == 0);
    restart(zero);
    set_view(V_HOME);
    key_event(12, 1); key_event(12, 0);  /* white key 8 = P8 */
    run_ms(10);
    combination(home);
    before = store_writes;
    assert(ui_dirty());
    now_ms += AUTOSAVE_QUIET + AUTOSAVE_GAP;
    autosave();
    assert(store_writes > before && !ui_dirty());
    restart(home);
    /* Independent manual cues while playing; SAVE waits until STOP. */
    seq_start(&seq); run_ms(30);
    for (int p = 0; p < 5; p++) {
        ui.part = p; set_view(V_SEQ);
        for (int n = 7; n > chosen[p]; n--) turn(EN_PRESET, -1);
    }
    before = store_writes;
    save_project();
    assert(save_wait && store_writes == before);
    /* Song changes live selection, never manual startup bytes. */
    proj.arr.song.len = 2;
    for (int b = 0; b < 2; b++) {
        proj.arr.song.bar[b].mute = 0;
        for (int p = 0; p < 5; p++) proj.arr.song.bar[b].pat[p] = 10 + b;
    }
    seq.song_on = 1;
    run_ms(5000);
    assert(seq.ppat[0] == 10 || seq.ppat[0] == 11);
    assert(!memcmp(proj.set.start_pat, chosen, 5));
    seq.song_on = 0;
    seq_chain(&seq, 10, 12);
    run_ms(5000);
    assert(seq.ppat[0] >= 10 && seq.ppat[0] <= 12);
    assert(!memcmp(proj.set.start_pat, chosen, 5));
    seq_stop(&seq); run_ms(40);
    autosave();
    assert(!save_wait && store_writes > before);
    restart(chosen);
    assert(project_save() == 0);
    before = store_writes;
    assert(project_save() == 0 && store_writes == before);
    /* Same shared renderer as hardware; references for actual WASM panel navigation. */
    for (int o = 0; o < OBJ_NOBJ; o++) store_len[o] = -1;
    boot(); run_ms(20);
    for (int t = 0; t < 4; t++) {
        ui.part = t; ui.view = V_SEQ; ui.page[V_SEQ][t] = 1;
        build_pages(); draw_header(); draw_main(); draw_knobs();
        char path[80]; snprintf(path, sizeof path, "build/host/p9/remix-%d.png", t);
        png_write(path);
        if (t >= 2) {
            assert(pg.n == 3 && pg.r[2][0].kind == R_FREEPLAY);
            for (int k = 1; k < 4; k++) assert(pg.r[2][k].kind == R_NONE);
            ui.page[V_SEQ][t] = 2;
            draw_header(); draw_main(); draw_knobs();
            snprintf(path, sizeof path, "build/host/p9/keys-%d.png", t);
            png_write(path);
            ui.page[V_SEQ][t] = 1;
        }
    }
    puts("P9: manual PRESETS/HOME, migration, USB default, reset, autosave, PLAY save, song/chain isolation, reboot passed");
    return 0;
}
