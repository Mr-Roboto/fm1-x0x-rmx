/* SPDX-License-Identifier: GPL-3.0-only */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/engine.c"
#include <assert.h>

static void flush(void) { drain(); engine_sync_sound(&proj.sound); }
static void setup(void)
{
    held_btn = held_keys = 0;
    memset(enc_acc, 0, sizeof enc_acc); memset(&ui, 0, sizeof ui);
    project_defaults(); engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    engine_apply_sound(&proj.sound); flush(); ui_init();
}

static double filtered_power(int voice, int pot, float hz)
{
    float wave[256]; double power = 0; int sample = 0;
    apply_param(T_303, voice, B303_HPF, pot);
    for (int block = 0; block < 200; block++) {
        for (int i = 0; i < 256; i++, sample++) wave[i] = sinf(2.0f * FM_PI * hz * (float)sample / 44100.0f);
        bass_hpf_render(&bass_hpf[voice], wave, 256);
        if (block > 100) for (int i = 0; i < 256; i++) { assert(isfinite(wave[i])); power += wave[i] * wave[i]; }
    }
    return power;
}
static void hpf_test(void)
{
    setup();
    assert(engine_bass_hpf_hz(0) == 0 && engine_bass_hpf_hz(1) == 20 && engine_bass_hpf_hz(127) == 10000);
    for (int b = 0; b < 2; b++) {
        float wave[256], same[256];
        for (int i = 0; i < 256; i++) wave[i] = same[i] = (float)i / 256.0f - 0.5f;
        bass_hpf_render(&bass_hpf[b], wave, 256); assert(!memcmp(wave, same, sizeof wave));
        double low = filtered_power(b, 80, 40), high = filtered_power(b, 80, 8000);
        assert(low < high * 0.0001 && high > 10000);
        assert(bass_hpf[b ^ 1].value == 0);
        filtered_power(b, 0, 500);
        memcpy(wave, same, sizeof wave); bass_hpf_render(&bass_hpf[b], wave, 256);
        assert(!memcmp(wave, same, sizeof wave));
    }
    puts("303 HPFs: independent 12 dB/octave response, OFF exact bypass, finite output and smooth bypass passed");
}

typedef struct { unsigned time; int voice, note, accent, slide; } event_t;
static event_t events[2048]; static int nevent, midi_on[2], zero_off; static unsigned tick;
static seq_t test_seq; static pattern_t patterns[NPAT];
static void on(void *x, int b, int note, int acc, int slide)
{ (void)x; assert(nevent < 2048); events[nevent++] = (event_t){tick,b,note,acc,slide}; }
static void off(void *x, int b)
{ (void)x; assert(nevent < 2048); events[nevent++] = (event_t){tick,b,-1,0,0}; }
static int drum(void *x, int k, int v, float vel) { (void)x;(void)k;(void)v;(void)vel;return 1; }
static void midi_out(void *x, uint8_t st, uint8_t note, uint8_t vel)
{ (void)x;if ((st & 0xf0) == 0x80 && note == 0) zero_off++;
  if ((st & 0xf0) == 0x90 && vel) { if ((st & 15) == 1) midi_on[0]++; if ((st & 15) == 2) midi_on[1]++; } }
static const seq_sink_t sink = {drum,on,off,0,0,midi_out,0,0};
static void advance(unsigned samples)
{
    seq_advance(&test_seq, 0, &sink);
    while (samples) {
        unsigned n = seq_until_event(&test_seq, samples > 256 ? 256 : samples);
        tick += n; seq_advance(&test_seq, n, &sink); samples -= n;
    }
}
static void sequence_setup(int transpose)
{
    for (int i = 0; i < NPAT; i++) pattern_init(&patterns[i], 123, 456);
    seq_init(&test_seq, patterns); nevent = tick = 0; midi_on[0] = midi_on[1] = 0;
    zero_off = 0;
    test_seq.send_clock = 0; test_seq.send_notes = 1; test_seq.bpm = 120;
    test_seq.bass_mirror = 1; test_seq.bass_mirror_transpose = (uint8_t)(transpose + 24);
    bpart_t *a = &patterns[0].bass[0], *b = &patterns[0].bass[1];
    a->len = 8; b->len = 3; b->rate = RATE_32; b->dir = DIR_REV;
    for (int i = 0; i < 8; i++) { a->step[i].note = (uint8_t)(48 + i * 2); a->step[i].flags = G_NOTE; }
    a->step[1].flags |= BS_ACCENT | BS_SLIDE; a->step[2].flags = G_TIE;
    a->step[3].flags = G_REST;
    for (int i = 0; i < NSTEPS; i++) { b->step[i].note = 99; b->step[i].flags = G_NOTE; }
    patterns[0].swing = 60;
}
static void compare_events(int transpose)
{
    int cursor = 0, count = 0;
    for (int i = 0; i < nevent; i++) if (!events[i].voice) {
        while (cursor < nevent && events[cursor].voice != 1) cursor++;
        assert(cursor < nevent);
        event_t a = events[i], b = events[cursor++];
        int note = a.note < 0 ? -1 : a.note + transpose;
        if (note > 127) note = 127; if (note < 0 && a.note >= 0) note = 0;
        assert(a.time == b.time && note == b.note && a.accent == b.accent && a.slide == b.slide); count++;
    }
    while (cursor < nevent) { assert(events[cursor].voice != 1); cursor++; }
    assert(count > 5 && midi_on[0] == midi_on[1] && midi_on[0] > 3);
}
static void mirror_test(void)
{
    for (int transpose = -24; transpose <= 24; transpose += 12) {
        sequence_setup(transpose); seq_start(&test_seq); advance(44100 * 2 - 1); compare_events(transpose);
        seq_stop(&test_seq); advance(1); assert(!test_seq.t[TRK_BASS0 + 1].sounding);
    }
    sequence_setup(12); patterns[0].bass[0].dir = DIR_RANDOM;
    seq_start(&test_seq); advance(44100 * 2 - 1); compare_events(12);
    sequence_setup(-12); pattern_set_remix_selection(&patterns[0], TRK_BASS0, 3, 5);
    pattern_set_remix_enabled(&patterns[0], TRK_BASS0, 1);
    seq_start(&test_seq); advance(44100 * 2 - 1); compare_events(-12);
    sequence_setup(0); seq_start(&test_seq);
    for (int i = 0; i < 96; i++) { seq_ext_clock(&test_seq); advance(919); }
    compare_events(0);
    test_seq.bass_mirror = 0;
    for (int i = 0; i < 24; i++) { seq_ext_clock(&test_seq); advance(919); }
    assert(!test_seq.bass_mirror_active);
    int own = 0; for (int i = 0; i < nevent; i++) own |= events[i].voice == 1 && events[i].note == 99;
    assert(own && patterns[0].bass[1].step[0].note == 99);
    sequence_setup(0); test_seq.mute = 1u << MUTE_BASS0;
    seq_start(&test_seq); advance(22050); assert(!midi_on[0] && midi_on[1]);
    sequence_setup(0); test_seq.mute = 1u << (MUTE_BASS0 + 1);
    seq_start(&test_seq); advance(22050); assert(midi_on[0] && !midi_on[1]);
    sequence_setup(-24);
    for (int i = 0; i < 8; i++) patterns[0].bass[0].step[i].note = 127;
    patterns[0].bass[0].transpose = 48;
    seq_start(&test_seq); advance(44100); compare_events(-24);
    sequence_setup(-24);
    for (int i = 0; i < 8; i++) patterns[0].bass[0].step[i].note = 0;
    seq_start(&test_seq); advance(44100); compare_events(-24);
    seq_stop(&test_seq); advance(1); assert(zero_off && !test_seq.bass_midi_active);
    puts("303B mirror: matching note/gate times, accents/slides/ties/rests, signed transpose, random/Remix, external clock, MIDI, mute and disabling passed");
}
static void persistence_test(void)
{
    setup();
    for (int b = 0; b < 2; b++) {
        ui.part = PART_303A + b; ui.view = V_PART; build_pages(); open_list_of_pages();
        int hpf = -1, mirror = -1, transpose = -1;
        for (int i = 0; i < ui.list_n; i++) {
            pref_t r = list_rows[i]; if (r.kind != R_ENG) continue;
            if (r.c == B303_HPF) hpf = i;
            if (r.c == B303_MIRROR) mirror = i;
            if (r.c == B303_MIRROR_TRANSPOSE) transpose = i;
        }
        assert(hpf >= 0 && (b ? mirror >= 0 && transpose >= 0 : mirror < 0 && transpose < 0));
        ui.list_sel = (uint8_t)hpf; accel_off = 1; enc_acc[EN_ALGO] = 40; ui_input_only(); flush();
        assert(pref_get(list_rows[hpf]) == 40 && bass_hpf[b].value == 40);
        if (b) {
            ui.list_sel = (uint8_t)mirror; enc_acc[EN_ALGO] = 1; ui_input_only(); flush(); assert(seq.bass_mirror);
            ui.list_sel = (uint8_t)transpose; enc_acc[EN_ALGO] = -12; ui_input_only(); flush(); assert(seq.bass_mirror_transpose == 12);
            char value[24], unit[8]; pref_value(list_rows[transpose],value,unit); assert(!strcmp(value,"-12") && !strcmp(unit,"st"));
        }
        ui.list_top = b ? 16 : 12; ui.help = 0; ui.msg_until = 0; ui.touched = -1;
        build_pages(); draw_header(); draw_main(); draw_knobs();
        png_write(b ? "build/host/303b-mirror-more.png" : "build/host/303a-hpf-more.png");
        ui.overlay = O_NONE; accel_off = 0;
    }
    undo_start();
    pref_set(PR(R_ENG,T_303,1,B303_HPF), 72); flush();
    do_undo(0); flush(); assert(bass_hpf[1].value == 40);
    do_undo(1); flush(); assert(bass_hpf[1].value == 72);
    int lane = motion_alloc(proj.arr.lane, 0, PART_303A, T_303, 0, B303_HPF);
    assert(lane >= 0); proj.arr.lane[lane].val[0] = 96;
    motion_step(&mot, PART_303A, 0, 0, 1, 16); assert(bass_hpf[0].value == 96);
    motion_step(&mot, PART_303A, 0, 1, 2, 16); assert(bass_hpf[0].value == 40);
    motion_release(&mot); motion_clear(proj.arr.lane, lane);
    pref_set(PR(R_ENG,T_303,0,B303_HPF), 60);
    pref_set(PR(R_ENG,T_303,1,B303_HPF), 90);
    pref_set(PR(R_ENG,T_303,1,B303_MIRROR), 1);
    pref_set(PR(R_ENG,T_303,1,B303_MIRROR_TRANSPOSE), 12); flush();
    sound_t expected = proj.sound;
    assert(project_save() == 0 && project_load() == 0 && !memcmp(&expected,&proj.sound,sizeof expected));
    engine_apply_sound(&proj.sound); flush(); assert(seq.bass_mirror && seq.bass_mirror_transpose == 12);
    proj.set.snd_rev = 7; assert(project_save() == 0 && project_load() == 0);
    expected.v[T_303][0][B303_HPF] = expected.v[T_303][1][B303_HPF] = 0;
    expected.v[T_303][1][B303_MIRROR] = 0; expected.v[T_303][1][B303_MIRROR_TRANSPOSE] = 24;
    assert(!memcmp(&expected,&proj.sound,sizeof expected));
    assert(sizeof(sound_t)==1344 && sizeof(pattern_t)==528 && RAW_LEN==29572);
    puts("303 new controls: save/load, safe revision 7 migration and unchanged persistent sizes passed");
}
int main(void) { hpf_test(); mirror_test(); persistence_test(); return 0; }
