/* SPDX-License-Identifier: GPL-3.0-only */
/* Real engine step callback and probability sinks, with the host platform. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/app/engine.c"
#include "../../firmware/src/seq/remix.h"
#include <assert.h>

static int value_lane[NTRACKS], chance_lane[NTRACKS];
static unsigned visits[NTRACKS], accepted[NKIT], rejected[NKIT];
static int mapping_on;

static int expected_source(int t, int slot)
{
    if (!mapping_on || t == TRK_BRK ||
        (t >= NKIT && proj.pat[0].bass[t - NKIT].dir != DIR_FWD))
        return slot;
    return remix_source_step(1, 11, slot); /* B12: repeats, jumps and parity changes */
}

static void checked_step(void *ctx, int t, int slot)
{
    int src = expected_source(t, slot);
    int next = seq_next_step(&seq, t, slot);
    s_step(ctx, t, slot); /* production path, before actual event/probability sinks */
    assert(eng_step[t] == slot && mot.last[t] == slot);
    assert(engine_motion_value(value_lane[t]) == src + 20);
    assert(prob[t][0] == (src % 2 ? 0 : 100));
    lane_rt_t *r = &mot.rt[value_lane[t]];
    assert(r->from == src + 20 && r->to == expected_source(t, next) + 20);
    assert(r->part_parity == (slot & 1));
    visits[t]++;
}

static int checked_drum(void *ctx, int kit, int voice, float vel)
{
    int played = s_drum_seq(ctx, kit, voice, vel);
    assert(played == (expected_source(kit, seq.t[kit].pos) % 2 == 0));
    if (played) accepted[kit]++; else rejected[kit]++;
    return played;
}
static void checked_bass(void *ctx, int part, int note, int accent, int slide)
{
    s_bon(ctx, part, note, accent, slide);
    assert(b303[part].gate == (expected_source(PART_303A + part,
                               seq.t[PART_303A + part].pos) % 2 == 0));
}
static const seq_sink_t checked_sink = {.drum = checked_drum,
    .bass_on = checked_bass, .bass_off = s_boff, .brk = s_brk, .brk_stop = s_brkstop,
    .step = checked_step};

static void setup_playback(int len, int dir, int on, int external)
{
    project_defaults();
    memset(visits, 0, sizeof visits);
    memset(accepted, 0, sizeof accepted);
    memset(rejected, 0, sizeof rejected);
    mapping_on = on;
    for (int t = 0; t < NTRACKS; t++) {
        if (t < NKIT) {
            proj.pat[0].drum[t].len = len;
            for (int s = 0; s < NSTEPS; s++) sm_set(proj.pat[0].drum[t].hit[0], s);
        } else if (t < TRK_BRK) {
            proj.pat[0].bass[t - NKIT].len = len;
            proj.pat[0].bass[t - NKIT].dir = dir;
            for (int s = 0; s < NSTEPS; s++) {
                proj.pat[0].bass[t - NKIT].step[s].flags = G_NOTE;
                proj.pat[0].bass[t - NKIT].step[s].note = 36 + s % 24;
            }
        }
        int target = t < NKIT ? (t == 0 ? T_909 : T_808) :
                     t < TRK_BRK ? T_303 : T_BRK;
        int voice = t >= NKIT && t < TRK_BRK ? t - NKIT : 0;
        value_lane[t] = motion_alloc(proj.arr.lane, 0, t, target, voice, 0);
        chance_lane[t] = motion_alloc(proj.arr.lane, 0, t, T_PROB, t << 4, 0);
        assert(value_lane[t] >= 0 && chance_lane[t] >= 0);
        for (int s = 0; s < NSTEPS; s++) {
            proj.arr.lane[value_lane[t]].val[s] = s + 20;
            proj.arr.lane[chance_lane[t]].val[s] = s % 2 ? 0 : 100;
        }
    }
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    seq.send_clock = 0;
    seq.ext = external;
    for (int t = 0; t < 4; t++) seq_set_remix(&seq, t, 1, 11, on);
    seq_start(&seq);
    if (external) seq_ext_clock(&seq);
    seq_advance(&seq, 1, &checked_sink);
    for (int t = 0; t < NTRACKS; t++) assert(visits[t] == 1);
}

static void next_external(const seq_sink_t *sink)
{
    for (int n = 0; n < 6; n++) {
        seq_ext_clock(&seq);
        seq_advance(&seq, 1, sink);
    }
}

static void test_playback(void)
{
    const int lengths[] = {8, 16, 24, 40, 64};
    for (int on = 0; on < 2; on++)
        for (unsigned n = 0; n < sizeof lengths / sizeof lengths[0]; n++) {
            setup_playback(lengths[n], DIR_FWD, on, 1);
            for (int step = 0; step < lengths[n] * 2; step++) next_external(&checked_sink);
            for (int t = 0; t < NTRACKS; t++) assert(visits[t] >= (unsigned)lengths[n] * 2);
            for (int t = 0; t < NKIT; t++) assert(accepted[t] && rejected[t]);
        }
    for (int dir = DIR_REV; dir <= DIR_RANDOM; dir++) {
        setup_playback(24, dir, 1, 1);
        for (int step = 0; step < 60; step++) next_external(&checked_sink);
    }
}

static void test_ramps_and_swing(void)
{
    setup_playback(16, DIR_FWD, 1, 0);
    proj.pat[0].swing = 80;
    /* Learn both chronological durations before checking interpolation. */
    int jumps = 0, repeats = 0, parity_changes = 0;
    while (visits[0] < 50) {
        uint32_t n = seq_until_event(&seq, 256);
        motion_tick(&mot, n);
        seq_advance(&seq, n, &checked_sink);
        int slot = seq.t[0].pos, src = expected_source(0, slot);
        int next_src = expected_source(0, (slot + 1) % 16);
        if (visits[0] < 5 || mot.age[0] != 0) continue;
        uint32_t even = mot.len[0][0], odd = mot.len[0][1];
        assert(even > odd); /* Swing belongs to the chronological slots. */
        lane_rt_t *r = &mot.rt[value_lane[0]];
        uint32_t duration = mot.len[0][slot & 1];
        uint32_t ages[MOT_PARTS];
        memcpy(ages, mot.age, sizeof ages);
        motion_tick(&mot, duration / 2);
        assert(engine_motion_value(value_lane[0]) ==
               src + 20 + (next_src - src) * (int)(duration / 2) / (int)duration);
        motion_tick(&mot, duration - duration / 2);
        assert(engine_motion_value(value_lane[0]) == next_src + 20);
        /* Restore age so the subsequent clock advance still measures real timing. */
        memcpy(mot.age, ages, sizeof ages);
        if (src == next_src) { assert(!r->ramp); repeats++; }
        if (abs(src - next_src) > 1) jumps++;
        if ((src & 1) != (slot & 1)) parity_changes++;
    }
    assert(jumps && repeats && parity_changes);
}

static void test_pending(void)
{
    for (int mode = REMIX_STEP; mode <= REMIX_LOOP; mode++) {
        setup_playback(24, DIR_FWD, 1, 1);
        seq.remix_apply = mode;
        next_external(&checked_sink);
        for (int t = 0; t < 4; t++) seq_set_remix(&seq, t, 0, 0, 0);
        int boundary = mode == REMIX_STEP ? 2 : mode == REMIX_BEAT ? 4 : mode == REMIX_BAR ? 16 : 24;
        for (int slot = 2; slot < boundary; slot++) next_external(&checked_sink);
        /* Lookahead on the preceding slot must still use the active selection. */
        assert(mot.rt[value_lane[0]].to == expected_source(0, boundary % 24) + 20);
        mapping_on = 0;
        next_external(&checked_sink);
        assert(seq.remix[0].active == 0);
        /* Pending enable and selection changes are equally inaudible. */
        for (int t = 0; t < 4; t++) seq_set_remix(&seq, t, 1, 11, 1);
        int slot = seq.t[0].pos;
        int next_boundary = mode == REMIX_STEP ? slot + 1 : mode == REMIX_BEAT ? (slot / 4 + 1) * 4 :
                            mode == REMIX_BAR ? 24 : 24;
        for (int n = slot + 1; n < next_boundary; n++) next_external(&checked_sink);
        assert(mot.rt[value_lane[0]].to == next_boundary % 24 + 20);
        mapping_on = 1;
        next_external(&checked_sink);
        assert(seq.remix[0].active == (256 | 27));
    }
}

static void test_recording(void)
{
    setup_playback(16, DIR_FWD, 1, 1);
    int k = value_lane[0];
    lane_t before = proj.arr.lane[k];
    /* The existing chronological gap formula writes mapped destinations in order. */
    seq.t[0].pos = 2;
    proj.sound.v[T_909][0][0] = 30;
    engine_motion_rec(k);
    s_step(0, 0, 2);
    s_step(0, 0, 3);
    s_step(0, 0, 4);
    s_step(0, 0, 5);
    proj.sound.v[T_909][0][0] = 70;
    engine_motion_rec(k);
    s_step(0, 0, 6);
    for (int slot = 2; slot <= 6; slot++)
        before.val[expected_source(0, slot)] = 30 + (slot - 2) * 10;
    assert(!memcmp(proj.arr.lane[k].val, before.val, NSTEPS));
    assert(mot.rt[k].rec_last == 6 && !mot.rt[k].ramp);
    for (int s = 7; s < 12; s++) s_step(0, 0, s);
    assert(mot.rt[k].rec_last == MOT_NONE);
    assert(engine_motion_value(k) == proj.arr.lane[k].val[expected_source(0, 11)]);
    engine_motion_hold(k);
    for (int s = 0; s < 16; s++) {
        s_step(0, 0, s);
        assert(engine_motion_value(k) < 0 && !mot.rt[k].ramp);
    }
    s_step(0, 0, 0);
    assert(engine_motion_value(k) == proj.arr.lane[k].val[0]);
    /* Conditional probability still counts chronological wraps. */
    prob[0][0] = PROB_COND;
    seq.t[0].bars = 0; assert(!prob_skip(0, 0));
    seq.t[0].bars = 1; assert(prob_skip(0, 0));
    seq.t[0].bars = 2; assert(!prob_skip(0, 0));
}

/* Endpoints, gaps, wraps and collisions across every supported length and part.
 * Expected writes are applied chronologically, so repeated sources are last-wins. */
static void test_recording_lengths(void)
{
    unsigned repeats = 0, beyond_length = 0;
    for (int on = 0; on <= 1; on++)
        for (int dir = DIR_FWD; dir <= DIR_RANDOM; dir++)
            for (int len = 1; len <= NSTEPS; len++) {
                setup_playback(len, dir, on, 1);
                for (int t = 0; t < NTRACKS; t++) {
                    int pass = seq_part_len(&seq, t);
                    int k = value_lane[t];
                    uint8_t expected[NSTEPS];
                    memcpy(expected, proj.arr.lane[k].val, sizeof expected);
                    int previous = -1, previous_source = -1;
                    int target = proj.arr.lane[k].t, voice = proj.arr.lane[k].v;
                    for (int n = 0; n < pass * 2 + 8; n++) {
                        int slot = n % pass;
                        if (n % 4 == 0) {
                            int now = (n % 8 == 0) ? 30 : 70;
                            proj.sound.v[target][voice][0] = now;
                            engine_motion_rec(k);
                            if (previous >= 0) {
                                int d = (slot - previous + pass) % pass;
                                int from = expected[previous_source];
                                if (d >= 2 && d <= MOT_GAP)
                                    for (int j = 1; j < d; j++) {
                                        int dest = expected_source(t, (previous + j) % pass);
                                        expected[dest] = from + (now - from) * j / d;
                                        if (dest == previous_source) repeats++;
                                    }
                            }
                            previous = slot;
                            previous_source = expected_source(t, slot);
                            if (previous_source >= len) beyond_length++;
                            expected[previous_source] = now;
                        }
                        s_step(0, t, slot);
                        assert(!memcmp(expected, proj.arr.lane[k].val, sizeof expected));
                    }
                }
            }
    assert(repeats && beyond_length);
}

static void test_recording_apply(void)
{
    /* Gap writes use the snapshot active at the second endpoint, even when it
     * differs from the first endpoint. Its starting value remains at the old source. */
    for (int mode = REMIX_STEP; mode <= REMIX_LOOP; mode++)
        for (int enable = 0; enable <= 1; enable++) {
            setup_playback(24, DIR_FWD, 1, 1);
            seq.remix_apply = mode;
            int boundary = mode == REMIX_STEP ? 3 : mode == REMIX_BEAT ? 4 :
                           mode == REMIX_BAR ? 16 : 24;
            for (int slot = 1; slot < boundary - 2; slot++) next_external(&SINK);
            int first = seq.t[0].pos;
            uint8_t expected[NTRACKS][NSTEPS];
            int old_source[NTRACKS];
            for (int t = 0; t < TRK_BRK; t++) {
                int k = value_lane[t];
                memcpy(expected[t], proj.arr.lane[k].val, NSTEPS);
                old_source[t] = seq_source_step(&seq, t, (first + 1) % 24);
                proj.sound.v[proj.arr.lane[k].t][proj.arr.lane[k].v][0] = 30;
                engine_motion_rec(k);
            }
            next_external(&SINK);
            first = seq.t[0].pos;
            for (int t = 0; t < TRK_BRK; t++) {
                expected[t][old_source[t]] = 30;
                assert(!memcmp(expected[t], proj.arr.lane[value_lane[t]].val, NSTEPS));
                if (t < TRK_BRK) seq_set_remix(&seq, t, 0, 2, enable);
            }
            /* STEP activates on the next slot. Other modes have a pre-boundary
             * endpoint, which must still record using B12 despite the pending edit. */
            if (mode != REMIX_STEP) {
                for (int t = 0; t < TRK_BRK; t++) {
                    int k = value_lane[t];
                    proj.sound.v[proj.arr.lane[k].t][proj.arr.lane[k].v][0] = 40;
                    engine_motion_rec(k);
                }
                next_external(&SINK);
                for (int t = 0; t < TRK_BRK; t++) {
                    old_source[t] = expected_source(t, seq.t[t].pos);
                    expected[t][old_source[t]] = 40;
                    assert(!memcmp(expected[t], proj.arr.lane[value_lane[t]].val, NSTEPS));
                }
                first = seq.t[0].pos;
            }
            /* Record again three slots later: includes the activation boundary. */
            next_external(&SINK);
            next_external(&SINK);
            for (int t = 0; t < TRK_BRK; t++) {
                int k = value_lane[t];
                proj.sound.v[proj.arr.lane[k].t][proj.arr.lane[k].v][0] = 70;
                engine_motion_rec(k);
            }
            next_external(&SINK);
            for (int t = 0; t < TRK_BRK; t++) {
                int from = expected[t][old_source[t]];
                for (int j = 1; j < 3; j++) {
                    int slot = (first + j) % 24;
                    int dest = (t == TRK_BRK || !enable) ? slot : remix_source_step(0, 2, slot);
                    expected[t][dest] = from + (70 - from) * j / 3;
                }
                int slot = (first + 3) % 24;
                int dest = (t == TRK_BRK || !enable) ? slot : remix_source_step(0, 2, slot);
                expected[t][dest] = 70;
                assert(seq.t[t].pos == slot);
                assert(!memcmp(expected[t], proj.arr.lane[value_lane[t]].val, NSTEPS));
            }
        }
}

static void test_empty_values(void)
{
    setup_playback(8, DIR_FWD, 1, 1);
    int k = value_lane[0];
    int src = expected_source(0, 1), next = expected_source(0, 2);
    assert(src != 1 && next != src);
    proj.arr.lane[k].val[src] = MOT_NONE;
    s_step(0, 0, 1);
    assert(engine_motion_value(k) == proj.sound.v[T_909][0][0]);
    assert(!mot.rt[k].ramp);
    proj.arr.lane[k].val[src] = 80;
    proj.arr.lane[k].val[next] = MOT_NONE;
    s_step(0, 0, 1);
    motion_tick(&mot, 500);
    assert(engine_motion_value(k) == 80 && !mot.rt[k].ramp);
    /* Probability's empty mapped step restores the existing 100% base. */
    proj.arr.lane[chance_lane[0]].val[src] = MOT_NONE;
    s_step(0, 0, 1);
    assert(prob[0][0] == 100);
}

int main(void)
{
    test_playback();
    test_ramps_and_swing();
    test_pending();
    test_recording();
    test_recording_lengths();
    test_recording_apply();
    test_empty_values();
    puts("Remix Motion: source locks/chance, ramps/repeats, blocks, Swing, APPLY, directions and recording passed");
    return 0;
}
