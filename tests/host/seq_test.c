/* SPDX-License-Identifier: GPL-3.0-only */
/* Sequencer and TB-3PO tests on the host: event times to the sample, 303 gate / slide /
 * tie semantics, swing, rates, polymeter, pattern cue + chain, external clock, MIDI clock
 * out, the break clock, and TB-3PO lines identical to schwung-tb3po's for the same seed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../firmware/src/seq/sequencer.h"
#include "../../firmware/src/seq/tb3po.h"  /* IWYU pragma: keep (tb3po_* below) */

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---- event log */
enum { E_DRUM, E_ON, E_OFF, E_BRK, E_BRKSTOP, E_MIDI };
typedef struct { uint32_t t; int kind, a, b, c, d; } ev_t;
static ev_t evs[100000];
static int nev;
static uint32_t now;
static void log_ev(int k, int a, int b, int c, int d)
{
    if (nev < (int)(sizeof evs / sizeof evs[0]))
        evs[nev++] = (ev_t){now, k, a, b, c, d};
}
static int s_drum(void *x, int k, int v, float vel) { (void)x; log_ev(E_DRUM, k * 100 + v, (int)(vel * 127.0f + 0.5f), 0, 0); return 1; }
static void s_on(void *x, int p, int n, int acc, int sl) { (void)x; log_ev(E_ON, p, n, acc, sl); }
static void s_off(void *x, int p) { (void)x; log_ev(E_OFF, p, 0, 0, 0); }
static void s_brk(void *x, int s16, int bar, float spb, int en) { (void)x; log_ev(E_BRK, s16, bar, (int)spb, en); }
static void s_brkstop(void *x) { (void)x; log_ev(E_BRKSTOP, 0, 0, 0, 0); }
static void s_midi(void *x, uint8_t st, uint8_t d1, uint8_t d2) { (void)x; log_ev(E_MIDI, st, d1, d2, 0); }
static const seq_sink_t SINK = {s_drum, s_on, s_off, s_brk, s_brkstop, s_midi, 0, 0};

static seq_t S;
static pattern_t P[NPAT];

/* render like the engine: block of 256, split at events */
static void run(uint32_t samples)
{
    while (samples) {
        uint32_t n = samples < 256 ? samples : 256;
        seq_advance(&S, 0, &SINK);
        while (n) {
            uint32_t k = seq_until_event(&S, n);
            now += k;
            seq_advance(&S, k, &SINK);
            n -= k;
            samples -= k;
        }
    }
}

static void reset(void)
{
    int i;
    for (i = 0; i < NPAT; i++)
        pattern_init(&P[i], 0x100 + i, 0x200 + i);
    seq_init(&S, P);
    S.send_clock = 0;
    nev = 0;
    now = 0;
}

static int find(int kind, int a, int from)
{
    int i;
    for (i = from; i < nev; i++)
        if (evs[i].kind == kind && (a < 0 || evs[i].a == a))
            return i;
    return -1;
}

static void test_drum_timing(void)
{
    int i, k, n = 0;
    double step = 44100.0 * 60.0 / 120.0 / 4.0;      /* 5512.5 samples */
    reset();
    S.bpm = 120.0f;
    for (i = 0; i < 16; i += 4)
        P[0].drum[0].hit[0][0] |= 1u << i;                 /* BD on the quarters */
    P[0].drum[0].hit[7][0] = 0xAAAAu;                      /* CH on the off-16ths */
    P[0].drum[0].accent[0] = 1u;                           /* accent on step 1 */
    seq_start(&S);
    run(44100 * 4 - 1);                              /* 2 bars, not the downbeat of the 3rd */
    for (i = 0, k = 0; (i = find(E_DRUM, 0, i)) >= 0; i++, k++) {
        double want = k * 4 * step;
        CHECK(abs((int)evs[i].t - (int)(want + 0.5)) <= 1, "BD %d at %u, want %.1f", k, evs[i].t, want);
        if (k % 4 == 0)
            CHECK(evs[i].b == 127, "BD %d accented vel %d", k, evs[i].b);
        else
            CHECK(evs[i].b == 88, "BD %d vel %d, want 88", k, evs[i].b);
        n++;
    }
    CHECK(n == 8, "BD hits %d, want 8 in 2 bars", n);
    for (i = 0, k = 0; (i = find(E_DRUM, 7, i)) >= 0; i++, k++)
        CHECK(abs((int)evs[i].t - (int)((2 * k + 1) * step + 0.5)) <= 1, "CH %d at %u", k, evs[i].t);
    CHECK(k == 16, "CH hits %d", k);
    printf("  drum timing: %d BD, %d CH on the sample\n", n, k);
}

static void test_swing_and_rates(void)
{
    int i, k;
    double step = 44100.0 * 60.0 / 120.0 / 4.0;
    reset();
    S.bpm = 120.0f;
    P[0].swing = 100;                                /* 75 %: odd 16ths half a step late */
    P[0].drum[0].hit[7][0] = 0xFFFFu;
    seq_start(&S);
    run(44100 * 2);
    for (i = 0, k = 0; (i = find(E_DRUM, 7, i)) >= 0 && k < 16; i++, k++) {
        double want = k * step + ((k & 1) ? 0.5 * step : 0.0);
        CHECK(abs((int)evs[i].t - (int)(want + 0.5)) <= 1, "swung CH %d at %u want %.1f", k, evs[i].t, want);
    }
    /* triplets: 1/16T = 1/6 quarter, never swung */
    reset();
    S.bpm = 120.0f;
    P[0].swing = 100;
    P[0].drum[0].rate = RATE_16T;
    P[0].drum[0].len = 12;
    P[0].drum[0].hit[7][0] = 0xFFFu;
    seq_start(&S);
    run(44100);
    for (i = 0, k = 0; (i = find(E_DRUM, 7, i)) >= 0 && k < 12; i++, k++) {
        double want = k * 44100.0 * 60.0 / 120.0 / 6.0;
        CHECK(abs((int)evs[i].t - (int)(want + 0.5)) <= 1, "triplet %d at %u want %.1f", k, evs[i].t, want);
    }
    printf("  swing 75%%, 1/16T unswung: checked\n");
}

static void test_303(void)
{
    bpart_t *b;
    int i, on, off;
    double step = 44100.0 * 60.0 / 120.0 / 4.0;
    reset();
    S.bpm = 120.0f;
    b = &P[0].bass[0];
    b->len = 8;
    b->step[0] = (bstep_t){36, G_NOTE};                       /* plain: off at half step */
    b->step[1] = (bstep_t){38, G_NOTE | BS_SLIDE};            /* slides into step 2 */
    b->step[2] = (bstep_t){41, G_NOTE | BS_ACCENT};           /* reached by slide, accented */
    b->step[3] = (bstep_t){41, G_TIE};                        /* tie: 2 held through 3 */
    b->step[4] = (bstep_t){0, G_REST};
    b->step[5] = (bstep_t){43, G_TIE};                        /* tie after rest = a note */
    b->transpose = 26;                                        /* +2 */
    seq_start(&S);
    run((uint32_t)(step * 8));
    on = find(E_ON, 0, 0);
    CHECK(on >= 0 && evs[on].b == 38 && evs[on].d == 0 && evs[on].t == 0, "step 1 note 36+2, no slide");
    off = find(E_OFF, 0, on);
    CHECK(off >= 0 && abs((int)evs[off].t - (int)(step / 2)) <= 1, "step 1 off at half step (%u)", off >= 0 ? evs[off].t : 0);
    on = find(E_ON, 0, on + 1);
    CHECK(on >= 0 && evs[on].b == 40 && evs[on].d == 0, "step 2 on");
    i = find(E_ON, 0, on + 1);
    off = find(E_OFF, 0, on + 1);
    CHECK(i >= 0 && evs[i].b == 43 && evs[i].d == 1 && evs[i].c == 1, "step 3 slides (slide=%d acc=%d)", i >= 0 ? evs[i].d : -1, i >= 0 ? evs[i].c : -1);
    CHECK(off > i, "no gate off between a slide and its target");
    CHECK(off >= 0 && abs((int)evs[off].t - (int)(3.5 * step + 0.5)) <= 1, "tie holds step 3 into step 4, whose gate falls at its half (%u, want %.0f)", off >= 0 ? evs[off].t : 0, 3.5 * step);
    i = find(E_ON, 0, off);
    CHECK(i >= 0 && evs[i].b == 45 && abs((int)evs[i].t - (int)(5 * step + 0.5)) <= 1, "tie after a rest plays as a note");
    printf("  303 gate / slide / tie / transpose: checked\n");
}

static void test_polymeter_cue_chain(void)
{
    int i, n, last;
    double step = 44100.0 * 60.0 / 120.0 / 4.0;
    reset();
    S.bpm = 120.0f;
    P[0].bass[0].len = 3;
    P[0].bass[0].step[0] = (bstep_t){36, G_NOTE};
    P[0].drum[0].hit[0][0] = 1u;
    P[1].drum[0].hit[1][0] = 1u;                                    /* SD only in pattern 2 */
    seq_start(&S);
    run((uint32_t)(step * 15) - 1);                         /* not the note due exactly at step 15 */
    for (i = 0, n = 0; (i = find(E_ON, 0, i)) >= 0; i++, n++)
        CHECK(abs((int)evs[i].t - (int)(n * 3 * step + 0.5)) <= 1, "3-step 303 loop %d at %u", n, evs[i].t);
    CHECK(n == 5, "3-step line over 15 steps: %d notes", n);
    seq_cue(&S, 1);
    nev = 0;
    run((uint32_t)(step * 17));                               /* to the bar line and one more */
    i = find(E_DRUM, 1, 0);
    CHECK(i >= 0 && abs((int)evs[i].t - (int)(16 * step + 0.5)) <= 1, "cued pattern starts on the bar (%u)", i >= 0 ? evs[i].t : 0);
    CHECK(S.ppat[0] == 1 && S.ppat[4] == 1, "now playing pattern 2");
    i = find(E_ON, 0, 0);
    /* pattern 2's 303 A line is empty: no notes after the switch */
    for (n = 0; (i = find(E_ON, 0, i < 0 ? 0 : i)) >= 0; i++)
        if (evs[i].t >= (uint32_t)(16 * step))
            n++;
    CHECK(n == 0, "303 restarts with the new (empty) pattern");
    /* chain 3..5 */
    reset();
    S.bpm = 120.0f;
    for (i = 0; i < NPAT; i++)
        P[i].drum[0].hit[0][0] = 1u;
    for (i = 0; i < NTRACKS; i++)
        S.ppat[i] = 3;
    seq_chain(&S, 3, 5);
    seq_start(&S);
    for (i = 0, last = -1; i < 7; i++) {
        uint8_t c = S.ppat[0];
        run((uint32_t)(16 * step));
        if (i < 6)
            CHECK(c == 3 + (i % 3), "chain bar %d plays pattern %d (got %d)", i, 3 + i % 3, c + 1 - 1);
        last = c;
    }
    (void)last;
    printf("  polymeter, cue on the bar, chain 4-6: checked\n");
}

/* one part's cue: it switches on the bar and restarts; the others keep their place */
static void test_part_cue(void)
{
    int i, n;
    double step = 44100.0 * 60.0 / 120.0 / 4.0;
    reset();
    S.bpm = 120.0f;
    P[0].drum[0].hit[0][0] = 1u;
    P[0].bass[1].len = 3;                                       /* 303 B: a 3-step loop */
    P[0].bass[1].step[0] = (bstep_t){40, G_NOTE};
    P[1].bass[0].step[0] = (bstep_t){50, G_NOTE};               /* 303 A in pattern 2 */
    seq_start(&S);
    run((uint32_t)(step * 5));
    seq_cue_part(&S, TRK_BASS0, 1);
    run((uint32_t)(step * 11) - 1);                             /* to just before the bar */
    CHECK(S.ppat[TRK_BASS0] == 0, "a part's cue waits for the bar");
    nev = 0;
    run((uint32_t)(step * 4));
    CHECK(S.ppat[TRK_BASS0] == 1 && S.ppat[0] == 0 && S.ppat[TRK_BASS0 + 1] == 0, "only 303 A switched");
    i = find(E_ON, 0, 0);
    CHECK(i >= 0 && evs[i].b == 50 && abs((int)evs[i].t - (int)(16 * step + 0.5)) <= 1, "303 A's new line on the bar");
    for (i = 0, n = 0; (i = find(E_ON, 1, i)) >= 0; i++, n++)   /* 303 B: steps 18 (its 7th loop), not 16 */
        CHECK(abs((int)evs[i].t - (int)(18 * step + 0.5)) <= 1, "303 B kept its place (note at %u)", evs[i].t);
    CHECK(n == 1, "303 B: one note in the window, %d", n);
    printf("  a part's own cue: checked\n");
}

static void test_song(void)
{
    static song_t sg;
    double bar = 44100.0 * 60.0 / 120.0 * 4.0;
    int i;
    /* playback: three bars, looping; a bar's mutes */
    reset();
    S.bpm = 120.0f;
    S.song = &sg;
    sg.len = 3;
    for (i = 0; i < NTRACKS; i++) {
        sg.bar[0].pat[i] = 0;
        sg.bar[1].pat[i] = 1;
        sg.bar[2].pat[i] = 2;
    }
    sg.bar[1].pat[TRK_BRK] = 7;
    sg.bar[0].mute = sg.bar[1].mute = 0;
    sg.bar[2].mute = 1u << 1;                                   /* the 808 out in bar 3 */
    S.song_on = 1;
    S.song_start = 1;
    seq_start(&S);
    run(1);
    CHECK(S.ppat[0] == 1 && S.ppat[TRK_BRK] == 7 && S.song_pos == 1, "starts at the start bar");
    run((uint32_t)bar);
    CHECK(S.ppat[0] == 2 && S.song_pos == 2 && seq_part_muted(&S, 1) && !seq_part_muted(&S, 0), "bar 3, 808 muted");
    run((uint32_t)bar);
    CHECK(S.ppat[0] == 0 && S.song_pos == 0 && !seq_part_muted(&S, 1), "loops to bar 1, 808 back");
    seq_cue(&S, 9);
    run((uint32_t)bar);
    CHECK(S.ppat[0] == 1, "playing the song, a cue does not override it");
    /* recording: from an empty song, the live cues and mutes are written bar by bar */
    reset();
    S.bpm = 120.0f;
    S.song = &sg;
    sg.len = 0;
    S.song_on = S.song_rec = 1;
    S.song_start = 0;
    seq_start(&S);
    run(1);
    CHECK(sg.len == 1 && sg.bar[0].pat[0] == 0, "record: bar 1 written at the start");
    seq_cue_part(&S, TRK_BASS0, 5);
    S.mute |= seq_part_mask(TRK_BRK);
    run((uint32_t)bar);
    CHECK(sg.len == 2 && sg.bar[1].pat[TRK_BASS0] == 5 && sg.bar[1].pat[0] == 0 && sg.bar[1].mute == 1u << TRK_BRK,
          "record: bar 2 has the cue and the mute (len %d, pat %d, mute %d)", sg.len, sg.bar[1].pat[TRK_BASS0], sg.bar[1].mute);
    run((uint32_t)bar * 2);
    CHECK(sg.len == 4 && sg.bar[3].pat[TRK_BASS0] == 5, "record: the song grows while it runs (%d)", sg.len);
    /* overdub from bar 2 plays bar 2 as it is, then writes on */
    seq_stop(&S);
    run(1);
    S.song_start = 1;
    sg.bar[1].pat[0] = 6;
    seq_start(&S);
    run(1);
    CHECK(S.ppat[0] == 6 && sg.len == 4, "overdub starts from the song's bar");
    printf("  song: playback, loop, mutes, record, overdub: checked\n");
}

static void test_ext_clock_and_midi_out(void)
{
    int i, k, clocks;
    uint32_t per_tick = 919;                                  /* ~120 BPM: 44100*60/120/24 = 918.75 */
    reset();
    P[0].drum[0].hit[0][0] = 0xFFFFu;
    for (i = 0; i < 2; i++) {                                 /* clocks before start: tempo only */
        seq_ext_clock(&S);
        run(per_tick);
    }
    seq_start(&S);
    nev = 0;
    {
    uint32_t t_start = now;
    for (i = 0; i < 24 * 4; i++) {                            /* one bar of ticks */
        uint32_t t0 = now;
        seq_ext_clock(&S);
        run(per_tick);
        (void)t0;
    }
    CHECK(S.ext == 1, "following the external clock");
    for (i = 0, k = 0; (i = find(E_DRUM, 0, i)) >= 0; i++, k++)
        CHECK((evs[i].t - t_start) == (uint32_t)k * 6u * per_tick, "ext step %d at +%u, want tick %d", k, evs[i].t - t_start, 6 * k);
    }
    CHECK(k == 16, "16 steps in 96 ticks, got %d", k);
    CHECK(S.ext_bpm > 119.0f && S.ext_bpm < 121.0f, "tempo estimate %.2f", (double)S.ext_bpm);
    run(44100);                                               /* clock gone: back to internal */
    CHECK(S.ext == 0, "internal again after the clock stops");
    /* MIDI clock out: 24 per quarter */
    reset();
    S.send_clock = 1;
    S.bpm = 120.0f;
    seq_start(&S);
    run(44100 * 2 - 1);                                       /* 4 quarters, not the next downbeat */
    for (i = 0, clocks = 0; i < nev; i++)
        if (evs[i].kind == E_MIDI && evs[i].a == 0xF8)
            clocks++;
    CHECK(clocks == 96, "MIDI clocks in 2 s at 120 BPM: %d, want 96", clocks);
    CHECK(find(E_MIDI, 0xFA, 0) == 0, "Start sent first");
    seq_stop(&S);
    run(256);
    CHECK(find(E_MIDI, 0xFC, 0) > 0, "Stop sent");
    printf("  external clock follow + tempo, MIDI clock out: checked\n");
}

static void test_break_clock(void)
{
    int i, k;
    reset();
    S.bpm = 170.0f;
    P[0].drum[0].rate = RATE_16T;                                /* the break stays on 16ths regardless */
    P[0].drum[0].len = 12;
    P[0].brk.steps = 0x00FFu;                                 /* first half of the bar */
    seq_start(&S);
    run(44100 * 3);
    for (i = 0, k = 0; (i = find(E_BRK, -1, i)) >= 0; i++, k++) {
        CHECK(evs[i].a == k % 16, "break step %d is %d", k, evs[i].a);
        CHECK(evs[i].b == k / 16, "break bar %d", evs[i].b);
        CHECK(evs[i].d == (k % 16 < 8), "break step %d enabled=%d", k, evs[i].d);
    }
    CHECK(k > 16 * 2, "break clocked %d times", k);
    seq_stop(&S);
    run(256);
    CHECK(find(E_BRKSTOP, -1, 0) >= 0, "break told to stop");
    printf("  break clock: %d 16ths, bars counted, step mask honoured\n", k);
}

/* ---- TB-3PO against schwung-tb3po: include its plugin source and call its generator */
#ifdef REF_TB3PO
#define move_plugin_init_v2 ref_move_plugin_init_v2
#define slot_init_defaults ref_slot_init_defaults
#define generate_pattern ref_generate_pattern
#define mutate_pattern ref_mutate_pattern
#include REF_TB3PO
static void test_tb3po_matches_reference(void)
{
    int seed, len, i, same = 0, total = 0;
    for (seed = 1; seed < 400; seed++) {
        for (len = 8; len <= 32; len += 8) {
            tb3po_slot_t ref;
            bpart_t b;
            ref_slot_init_defaults(&ref);
            ref.length = len;
            ref.density = (float)(30 + seed % 70) / 100.0f;
            ref.accent = (float)(seed % 90) / 100.0f;
            ref.slide = (float)(seed % 60) / 100.0f;
            ref.octave_range = 1 + seed % 3;
            ref.scale = seed % 6;
            ref.root = seed % 12;
            ref_generate_pattern(&ref, (uint32_t)seed * 7919u);
            memset(&b, 0, sizeof b);
            b.len = (uint8_t)len;
            tb3po_defaults(&b.gen, (uint32_t)seed * 7919u);
            b.gen.density = (uint8_t)(30 + seed % 70);
            b.gen.accent = (uint8_t)(seed % 90);
            b.gen.slide = (uint8_t)(seed % 60);
            b.gen.oct_range = (uint8_t)(1 + seed % 3);
            b.gen.scale = (uint8_t)(seed % 6);
            b.gen.root = (uint8_t)(seed % 12);
            tb3po_generate(&b);
            for (i = 0; i < len; i++) {
                int rk = ref.steps[i], gate = bstep_gate(&b.step[i]);
                int kind = gate == G_REST ? 0 : (b.step[i].flags & BS_SLIDE) ? 3 : (b.step[i].flags & BS_ACCENT) ? 2 : 1;
                int ok = rk == kind;
                if (ok && kind)
                    ok = b.step[i].note == note_for_step(&ref, i);
                same += ok;
                total++;
                if (!ok && total - same < 5)
                    printf("  seed %d len %d step %d: ref kind %d note %d, ours %d note %d\n", seed, len, i, rk,
                           note_for_step(&ref, i), kind, b.step[i].note);
            }
        }
    }
    CHECK(same == total, "TB-3PO steps identical to schwung-tb3po: %d of %d", same, total);
    printf("  TB-3PO vs schwung-tb3po: %d / %d steps identical over 396 seeds x 4 lengths\n", same, total);
}
#endif

/* Remix oracle: materialize the expected chronological pattern from explicit
 * historical A3/A5/A7/A8 rows, then compare every callback and gate time with
 * unmodified forward playback. This also checks wraps and partial blocks. */
static const uint8_t RM_ROWS[4][16] = {
    {0, 5, 2, 7, 0, 5, 2, 7, 8, 13, 10, 15, 8, 13, 10, 15}, /* 909 A3 */
    {0, 9, 2, 11, 4, 13, 6, 15, 0, 9, 2, 11, 4, 13, 6, 15}, /* 808 A5 */
    {0, 13, 2, 15, 0, 13, 2, 15, 0, 13, 2, 15, 0, 13, 2, 15}, /* 303A A7 */
    {0, 9, 6, 15, 0, 9, 6, 15, 0, 9, 6, 15, 0, 9, 6, 15}, /* 303B A8 */
};
static const int RM_PAT[4] = {2, 4, 6, 7};
static ev_t reference[100000];
static int nreference;
static seq_track_t reference_tracks[NTRACKS];
static void rm_step(void *ctx, int part, int slot)
{
    (void)ctx;
    CHECK(S.t[part].pos == slot, "callback and track position agree");
    log_ev(E_MIDI + 1, part, slot, S.t[part].bars, 0);
}
static const seq_sink_t RM_SINK = {s_drum, s_on, s_off, s_brk, s_brkstop, s_midi, rm_step, 0};
static void rm_advance(uint32_t samples)
{
    while (samples) {
        uint32_t k = seq_until_event(&S, samples < 256 ? samples : 256);
        now += k;
        seq_advance(&S, k, &RM_SINK);
        samples -= k;
    }
}
static void rm_run(uint32_t samples)
{
    seq_start(&S);
    seq_advance(&S, 0, &RM_SINK);
    rm_advance(samples);
}
static void rm_fixture(int len, int dir)
{
    int t, p, v;
    reset();
    S.bpm = 120.0f;
    S.send_notes = S.send_clock = 1;
    P[0].swing = 100;
    P[0].brk.steps = 0x5A31;
    for (t = 0; t < 4; t++) {
        CHECK(!pattern_remix_enabled(&P[0], t) && pattern_remix_selection(&P[0], t) == 0, "Remix defaults OFF for track %d", t);
        if (t < NKIT) {
            dpart_t *d = &P[0].drum[t];
            d->len = len;
            for (p = 0; p < NSTEPS; p++) {
                uint32_t bit = 1u << (p & 31);
                for (v = 0; v < NDRUM; v++)
                    if (v == p % NDRUM || v == (p + 3) % NDRUM || v == 0)
                        d->hit[v][p >> 5] |= bit;
                if (p % 3 == 1)
                    d->accent[p >> 5] |= bit;
            }
        } else {
            bpart_t *b = &P[0].bass[t - NKIT];
            b->len = len;
            b->dir = dir;
            for (p = 0; p < NSTEPS; p++)
                b->step[p] = (bstep_t){30 + p, G_NOTE | (p % 3 == 1 ? BS_ACCENT : 0)};
            /* A7: note -> non-adjacent TIE with slide -> accented note ->
             * REST. The original adjacent step 1 is a REST: only mapped
             * chronological lookahead can hold the first note. A8 also
             * exercises TIE after REST and repeated selected steps. */
            for (p = 0; p < NSTEPS; p += 16) {
                b->step[p + 1].flags = G_REST;
                b->step[p + 13].flags = G_TIE | BS_SLIDE;
                b->step[p + 2].flags = G_NOTE | BS_ACCENT;
                b->step[p + 15].flags = G_REST;
                b->step[p + 9].flags = G_TIE | BS_ACCENT;
                b->step[p + 6].flags = G_NOTE | BS_SLIDE;
            }
        }
    }
}
static void rm_materialize(void)
{
    pattern_t original = P[0];
    int t, p, v;
    for (t = 0; t < 4; t++) {
        if (t < NKIT) {
            memset(P[0].drum[t].hit, 0, sizeof P[0].drum[t].hit);
            memset(P[0].drum[t].accent, 0, sizeof P[0].drum[t].accent);
        }
        for (p = 0; p < NSTEPS; p++) {
            int src = (p / 16) * 16 + RM_ROWS[t][p % 16];
            if (t < NKIT) {
                const dpart_t *d = &original.drum[t];
                uint32_t bit = 1u << (src & 31), dst = 1u << (p & 31);
                for (v = 0; v < NDRUM; v++)
                    if (d->hit[v][src >> 5] & bit)
                        P[0].drum[t].hit[v][p >> 5] |= dst;
                if (d->accent[src >> 5] & bit)
                    P[0].drum[t].accent[p >> 5] |= dst;
            } else
                P[0].bass[t - NKIT].step[p] = original.bass[t - NKIT].step[src];
        }
    }
}
static void rm_save_reference(void)
{
    nreference = nev;
    memcpy(reference, evs, nev * sizeof *evs);
    memcpy(reference_tracks, S.t, sizeof S.t);
}
static void rm_compare(int len, int dir)
{
    int i;
    CHECK(nev == nreference, "len %d dir %d: event count %d vs %d", len, dir, nev, nreference);
    for (i = 0; i < nev && i < nreference; i++) {
        if (memcmp(&evs[i], &reference[i], sizeof *evs)) {
            CHECK(0, "len %d dir %d event %d: got %u/%d/%d/%d/%d/%d expected %u/%d/%d/%d/%d/%d",
                  len, dir, i, evs[i].t, evs[i].kind, evs[i].a, evs[i].b, evs[i].c, evs[i].d,
                  reference[i].t, reference[i].kind, reference[i].a, reference[i].b, reference[i].c, reference[i].d);
            break;
        }
    }
    CHECK(!memcmp(reference_tracks, S.t, sizeof S.t), "len %d dir %d: chronological runtime unchanged", len, dir);
}
static void test_remix_sequencer(void)
{
    static const int lengths[] = {8, 16, 24, 32, 40, 48, 64};
    int l, dir, t;
    for (l = 0; l < (int)(sizeof lengths / sizeof lengths[0]); l++) {
        int len = lengths[l];
        uint32_t samples = (uint32_t)(5512.5 * (len * 2 + 1));
        for (dir = DIR_FWD; dir <= DIR_RANDOM; dir++) {
            rm_fixture(len, dir);
            /* Non-forward bass stays original; drums are always forward. */
            if (dir == DIR_FWD)
                rm_materialize();
            else {
                seq_set_remix(&S, 0, 0, RM_PAT[0], 1);
                seq_set_remix(&S, 1, 0, RM_PAT[1], 1);
            }
            rm_run(samples);
            rm_save_reference();
            rm_fixture(len, dir);
            for (t = 0; t < 4; t++)
                seq_set_remix(&S, t, 0, RM_PAT[t], 1);
            seq_set_remix(&S, TRK_BRK, 0, 6, 1); /* must be ignored */
            rm_run(samples);
            rm_compare(len, dir);
        }
        /* OFF with non-neutral selections and enabled A1 are byte-exact. */
        rm_fixture(len, DIR_FWD);
        for (t = 0; t < 4; t++)
            seq_set_remix(&S, t, 0, RM_PAT[t], 0);
        rm_run(samples);
        rm_save_reference();
        rm_fixture(len, DIR_FWD);
        for (t = 0; t < 4; t++)
            seq_set_remix(&S, t, 0, 0, 1);
        rm_run(samples);
        rm_compare(len, DIR_FWD);
    }
    /* Independent active pattern per part; seq_init must keep saved state.
     * Compare mapped playback to physically materialized source events. */
    rm_fixture(24, DIR_FWD);
    rm_materialize();
    for (t = 0; t < 4; t++) {
        P[t + 1] = P[0];
        S.ppat[t] = t + 1;
    }
    rm_run(300000);
    rm_save_reference();
    rm_fixture(24, DIR_FWD);
    for (t = 0; t < 4; t++) {
        P[t + 1] = P[0];
        pattern_set_remix_selection(&P[t + 1], t, 0, RM_PAT[t]);
        pattern_set_remix_enabled(&P[t + 1], t, 1);
        /* Deliberately conflicting state on pattern zero. */
        pattern_set_remix_selection(&P[0], t, 15, 15);
        pattern_set_remix_enabled(&P[0], t, 1);
    }
    seq_init(&S, P);
    S.bpm = 120.0f;
    S.send_notes = S.send_clock = 1;
    for (t = 0; t < 4; t++)
        S.ppat[t] = t + 1;
    rm_run(300000);
    rm_compare(24, DIR_FWD);
    /* Cue a saved ON pattern from OFF during playback; compare against a
     * physically mapped pattern, including the transition's TIE lookahead. */
    for (int mapped = 0; mapped < 2; mapped++) {
        rm_fixture(16, DIR_FWD);
        pattern_t original = P[0];
        if (!mapped) {
            rm_materialize();
            P[1] = P[0];
            P[0] = original;
        } else {
            P[1] = original;
            for (t = 0; t < 4; t++) {
                pattern_set_remix_selection(&P[1], t, 0, RM_PAT[t]);
                pattern_set_remix_enabled(&P[1], t, 1);
            }
        }
        rm_run(20000);
        for (t = 0; t < 4; t++)
            seq_cue_part(&S, t, 1);
        rm_advance(300000);
        for (t = 0; t < 4; t++)
            CHECK(S.ppat[t] == 1, "Remix pattern cue applied to part %d", t);
        if (!mapped)
            rm_save_reference();
        else
            rm_compare(16, DIR_FWD);
    }
    reset();
    seq_set_remix(&S, -1, 0, 0, 1);
    seq_set_remix(&S, NTRACKS, 0, 0, 1);
    seq_set_remix(&S, 0, 16, 0, 1);
    seq_set_remix(&S, 0, 0, -1, 1);
    CHECK(pattern_remix_selection(&P[0], 0) == 0 && !pattern_remix_enabled(&P[0], 0), "invalid internal controls ignored");
    printf("  Remix: independent source columns, TIE/slide, repeats, partial blocks, chronological swing/callbacks, BREAK, OFF/A1 and directions checked\n");
}

int main(void)
{
    test_drum_timing();
    test_swing_and_rates();
    test_303();
    test_polymeter_cue_chain();
    test_part_cue();
    test_song();
    test_ext_clock_and_midi_out();
    test_break_clock();
    test_remix_sequencer();
#ifdef REF_TB3PO
    test_tb3po_matches_reference();
#endif
    printf("seq: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
