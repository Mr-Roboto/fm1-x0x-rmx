/* SPDX-License-Identifier: GPL-3.0-only */
/* Fixed-size performance controller; standard arp orders implemented locally. */
#include "juno_part.h"
#include "../app/plat.h"
#include "breaks.h"
#include "fastmath.h"
#include "master.h"
alpha_juno_t part_juno X0X_POOL;
static struct {
    uint8_t value[JX_NPARAMS], order[128], velocity[128], down[16], member[16];
    uint8_t count, pedal, playing, free_tick, step, filter_mode;
    int16_t arp_note, seq_note;
    int32_t off_left, seq_left;
    uint32_t arp_index, rng, seq_serial;
    uint8_t seq_connect;
    float bpm, tick_samples, clock_left, gate_gain, filter_g, ic1, ic2, wet;
} jp X0X_POOL;
static const char *const modes[] = {"POLY", "MONO", "LEGATO"}, *const onoff[] = {"OFF", "ON"};
static const char *const patterns[] = {"UP", "DOWN", "UP/DOWN", "DOWN/UP", "AS PLAYED", "RANDOM"};
static const char *const rates[] = {"1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T"};
static const char *const octaves[] = {"1 OCT", "2 OCT", "3 OCT"};
static const uint8_t divisions[] = {12, 8, 6, 4, 3, 2};
static const x0x_param_t extra[JX_NPARAMS] = {
    {"VOICE MODE", 2, 0, modes}, {"GLIDE", 127, 0, 0},      {"TRANCE GATE", 1, 0, onoff},
    {"GATE 1", 15, 13, 0},       {"GATE 2", 15, 10, 0},     {"GATE 3", 15, 13, 0},
    {"GATE 4", 15, 10, 0},       {"DJ FILTER", 127, 64, 0}, {"ARP", 1, 0, onoff},
    {"PATTERN", 5, 0, patterns}, {"RATE", 5, 2, rates},     {"OCTAVES", 2, 0, octaves},
    {"NOTE LEN", 100, 60, 0},    {"ARP LATCH", 1, 0, onoff}};
const x0x_param_t *juno_part_param(int i)
{
    return i >= 0 && i < JX_NPARAMS ? &extra[i] : 0;
}
static void zero(void *p, unsigned n)
{
    uint8_t *b = p;
    for (unsigned i = 0; i < n; i++)
        b[i] = 0;
}
static int bit(const uint8_t *b, int n)
{
    return b[n >> 3] >> (n & 7) & 1;
}
static void setbit(uint8_t *b, int n, int on)
{
    if (on)
        b[n >> 3] |= (uint8_t)(1u << (n & 7));
    else
        b[n >> 3] &= (uint8_t)~(1u << (n & 7));
}
static void remove_note(int n)
{
    for (int i = 0; i < jp.count; i++)
        if (jp.order[i] == n) {
            for (int j = i + 1; j < jp.count; j++)
                jp.order[j - 1] = jp.order[j];
            jp.count--;
            break;
        }
    setbit(jp.member, n, 0);
}
static void prune(void)
{
    for (int n = 0; n < 128; n++)
        if (bit(jp.member, n) && !bit(jp.down, n))
            remove_note(n);
}
static unsigned gate_mask(void)
{
    return jp.value[JX_GATE0] | (unsigned)jp.value[JX_GATE1] << 4 |
           (unsigned)jp.value[JX_GATE2] << 8 | (unsigned)jp.value[JX_GATE3] << 12;
}
unsigned juno_part_gate_step(void)
{
    return jp.step;
}
unsigned juno_part_division(void)
{
    unsigned d = jp.value[JX_ARP] ? divisions[jp.value[JX_ARP_RATE]] : 0;
    if (jp.value[JX_TRANCE]) {
        if (!d)
            d = 6;
        else {
            unsigned a = d, b = 6;
            while (b) {
                unsigned t = a % b;
                a = b;
                b = t;
            }
            d = a;
        }
    }
    return d;
}
static juno_voice_t *sequence_voice(void);
static void arp_off(void)
{
    if (jp.arp_note >= 0) {
        uint8_t sustain = part_juno.sustain;
        part_juno.sustain = 0;
        if (jp.seq_note != jp.arp_note || !sequence_voice()) alpha_juno_off(&part_juno, jp.arp_note);
        part_juno.sustain = sustain;
    }
    jp.arp_note = -1;
    jp.off_left = -1;
}
static void play(int n, int vel, int legato)
{
    if (jp.value[JX_MODE])
        alpha_juno_mono(&part_juno, n, vel, legato && jp.value[JX_MODE] == 2);
    else
        alpha_juno_note(&part_juno, n, vel);
}
/* Serial ownership prevents a later keyboard/arp voice steal receiving our note-off. */
static juno_voice_t *sequence_voice(void)
{
    for (int i = 0; i < JUNO_VOICES; i++) {
        juno_voice_t *v = &part_juno.voice[i];
        if (v->active && v->gate && v->sequenced && v->serial == jp.seq_serial) return v;
    }
    return 0;
}
static void sequence_off(void)
{
    int n = jp.seq_note;
    juno_voice_t *v = sequence_voice();
    jp.seq_note = -1; jp.seq_left = -1; jp.seq_serial = 0; jp.seq_connect = 0;
    if (!v) return;
    v->seq_left = 0; /* freeze the release tail at its current pitch */
    if (bit(jp.member, n) || jp.arp_note == n) {
        v->sequenced = 0; v->pitch = (float)n;
        return;
    }
    uint8_t sustain = part_juno.sustain;
    part_juno.sustain = 0;
    alpha_juno_off(&part_juno, n);
    part_juno.sustain = sustain;
    if (jp.value[JX_MODE] && jp.count && !jp.value[JX_ARP]) {
        n = jp.order[jp.count - 1];
        play(n, jp.velocity[n], 1);
    }
}
void juno_part_sequence_step(int note, int samples, unsigned flags, int hold,
                             int target, int slide_samples)
{
    juno_voice_t *v = sequence_voice();
    if (note < 0) { sequence_off(); return; }
    if (flags & JSEQ_TIE) {
        if (!v) { sequence_off(); return; } /* an orphan tie never invents a note */
    } else if (v && jp.seq_connect) {
        v->note = (uint8_t)note;
        v->pitch = (float)note; /* land exactly on the destination's step boundary */
        v->seq_left = 0;
        jp.seq_note = (int16_t)note;
        jp.seq_connect = 0;
    } else {
        sequence_off();
        if (samples < 1) return;
        float glide = part_juno.glide;
        part_juno.glide = 0; /* sequence timing is independent of keyboard GLIDE */
        play(note, 100, 0);
        part_juno.glide = glide;
        jp.seq_serial = part_juno.serial;
        for (int i = 0; i < JUNO_VOICES; i++)
            if (part_juno.voice[i].serial == jp.seq_serial) {
                v = &part_juno.voice[i]; break;
            }
        if (!v) return;
        v->sequenced = 1;
        jp.seq_note = (int16_t)note;
    }
    if (target >= 0 && slide_samples > 0 && ((flags & JSEQ_SLIDE) || jp.seq_connect)) {
        v->seq_target = (float)target;
        v->seq_delta = ((float)target - v->pitch) / (float)slide_samples;
        v->seq_left = (uint32_t)slide_samples;
        jp.seq_connect = 1;
    } else if (jp.seq_connect && target < 0) {
        /* A live edit inserted a rest: freeze until this step's gate ends. */
        jp.seq_connect = 0; v->seq_left = 0;
    }
    jp.seq_left = hold || jp.seq_connect ? -1 : (samples > 0 ? samples : 1);
}
void juno_part_sequence(int note, int samples)
{
    juno_part_sequence_step(note, samples, 0, 0, -1, 0);
}
static void restart_input(void)
{
    alpha_juno_all_off(&part_juno, 1);
    jp.seq_note = -1; jp.seq_left = -1; jp.seq_serial = 0; jp.seq_connect = 0;
    jp.arp_note = -1;
    jp.off_left = -1;
    jp.arp_index = 0;
    if (jp.value[JX_ARP])
        return;
    part_juno.sustain = jp.pedal;
    if (jp.value[JX_MODE]) {
        if (jp.count) {
            int n = jp.order[jp.count - 1];
            play(n, jp.velocity[n], 0);
        }
    } else
        for (int i = 0; i < jp.count; i++)
            play(jp.order[i], jp.velocity[jp.order[i]], 0);
}
void juno_part_set(int i, int v)
{
    if (i < 0 || i >= JX_NPARAMS)
        return;
    v = v < 0 ? 0 : v > extra[i].max ? extra[i].max : v;
    if (jp.value[i] == v)
        return;
    jp.value[i] = (uint8_t)v;
    if (i == JX_GLIDE)
        part_juno.glide = v ? 0.002f * fm_exp2f((float)v * 11.0f / 127.0f) : 0;
    if (i == JX_MODE || i == JX_ARP) {
        if (!jp.value[JX_ARP])
            prune();
        restart_input();
    }
    if (i == JX_ARP_PATTERN || i == JX_ARP_OCT)
        jp.arp_index = 0;
    if (i == JX_LATCH && !v && !jp.pedal) {
        prune();
        if (!jp.count)
            arp_off();
    }
    if (i == JX_FILTER) {
        uint8_t mode, cut;
        master_djf_pots(v, &mode, &cut);
        if (mode) {
            jp.filter_mode = mode;
            float fc = 30.0f * fm_exp2f((float)cut * 9.23f / 127.0f);
            jp.filter_g = fm_tanf(FM_PI * fm_minf(fc, 19000.0f) / 44100.0f);
        }
    }
}
void juno_part_key(int n, int vel)
{
    if (n < 0 || n > 127)
        return;
    int old = jp.count;
    if (vel > 0) {
        if (jp.value[JX_ARP] && jp.value[JX_LATCH] && !jp.pedal) {
            int any = 0;
            for (int i = 0; i < 16; i++)
                any |= jp.down[i];
            if (!any) {
                jp.count = 0;
                zero(jp.member, sizeof jp.member);
                arp_off();
            }
        }
        if (bit(jp.member, n))
            remove_note(n);
        jp.order[jp.count++] = (uint8_t)n;
        jp.velocity[n] = (uint8_t)(vel > 127 ? 127 : vel);
        setbit(jp.member, n, 1);
        setbit(jp.down, n, 1);
        if (!jp.value[JX_ARP])
            play(n, vel, old > 0);
        else if (!old && !jp.playing) {
            jp.free_tick = 0;
            jp.clock_left = 0;
            jp.arp_index = 0;
        }
    } else {
        setbit(jp.down, n, 0);
        if (!jp.value[JX_ARP] || (!jp.value[JX_LATCH] && !jp.pedal))
            remove_note(n);
        if (jp.value[JX_ARP]) {
            if (!jp.count)
                arp_off();
        } else if (!jp.value[JX_MODE]) {
            if (jp.seq_note != n || !sequence_voice()) alpha_juno_off(&part_juno, n);
        }
        else if (part_juno.voice[0].note == n) {
            if (jp.count) {
                int next = jp.order[jp.count - 1];
                play(next, jp.velocity[next], 1);
            } else
                alpha_juno_off(&part_juno, n);
        }
    }
}
void juno_part_midi(int ty, int a, int b)
{
    if (ty == 0x90 || ty == 0x80) {
        juno_part_key(a, ty == 0x90 ? b : 0);
        return;
    }
    if (ty == 0xb0 && (a == 120 || a == 123)) {
            jp.seq_note = -1; jp.seq_left = -1; jp.seq_serial = 0; jp.seq_connect = 0;
        jp.count = jp.pedal = 0;
        zero(jp.down, sizeof jp.down);
        zero(jp.member, sizeof jp.member);
        arp_off();
        alpha_juno_all_off(&part_juno, a == 120);
        return;
    }
    if (ty == 0xb0 && a == 64) {
        jp.pedal = b >= 64;
        if (jp.value[JX_ARP]) {
            if (!jp.pedal && !jp.value[JX_LATCH]) {
                prune();
                if (!jp.count)
                    arp_off();
            }
            return;
        }
    }
    alpha_juno_midi(&part_juno, ty, a, b);
}
void juno_part_tick(void *ctx, unsigned tick)
{
    (void)ctx;
    jp.step = (uint8_t)(tick / 6u);
    jp.free_tick = (uint8_t)((tick + 1u) % 96u);
    if (!jp.value[JX_ARP] || !jp.count || tick % divisions[jp.value[JX_ARP_RATE]])
        return;
    unsigned len = jp.count * (jp.value[JX_ARP_OCT] + 1u), index = jp.arp_index++, slot;
    switch (jp.value[JX_ARP_PATTERN]) {
    case 1:
        slot = len - 1 - index % len;
        break;
    case 2:
    case 3: {
        unsigned period = len > 1 ? 2 * len - 2 : 1;
        slot = index % period;
        if (slot >= len)
            slot = period - slot;
        if (jp.value[JX_ARP_PATTERN] == 3)
            slot = len - 1 - slot;
        break;
    }
    case 5:
        jp.rng ^= jp.rng << 13;
        jp.rng ^= jp.rng >> 17;
        jp.rng ^= jp.rng << 5;
        slot = jp.rng % len;
        break;
    default:
        slot = index % len;
        break;
    }
    unsigned rank = slot % jp.count;
    int n = 0;
    if (jp.value[JX_ARP_PATTERN] == 4)
        n = jp.order[rank];
    else
        for (int note = 0; note < 128; note++)
            if (bit(jp.member, note)) {
                if (!rank) {
                    n = note;
                    break;
                }
                rank--;
            }
    int vel = jp.velocity[n];
    n += (int)(slot / jp.count) * 12;
    while (n > 127)
        n -= 12;
    arp_off();
    play(n, vel, 0);
    jp.arp_note = (int16_t)n;
    int gate = jp.value[JX_ARP_GATE];
    if (gate < 1)
        gate = 1;
    jp.off_left =
        (int32_t)(jp.tick_samples * divisions[jp.value[JX_ARP_RATE]] * (float)gate / 100.0f);
    if (jp.off_left < 1)
        jp.off_left = 1;
}
void juno_part_transport(float bpm, int playing)
{
    if (bpm < 20)
        bpm = 20;
    if (jp.bpm != bpm) {
        jp.bpm = bpm;
        jp.tick_samples = 44100.0f * 60.0f / (bpm * 24.0f);
    }
    if (jp.playing != playing) {
        jp.playing = (uint8_t)playing;
        jp.clock_left = 0;
        if (!playing)
            jp.free_tick = 0;
    }
}
static void post(float *out, int n)
{
    int target = !jp.value[JX_TRANCE] || ((gate_mask() >> jp.step) & 1u);
    int filtering = jp.value[JX_FILTER] < 60 || jp.value[JX_FILTER] > 68;
    for (int i = 0; i < n; i++) {
        float d = (float)target - jp.gate_gain;
        if (d > 1.0f / 88.0f)
            d = 1.0f / 88.0f;
        if (d < -1.0f / 88.0f)
            d = -1.0f / 88.0f;
        jp.gate_gain += d;
        float x = out[i] * jp.gate_gain;
        if (filtering || jp.wet > 0) {
            float g = jp.filter_g, k = 1.41421356f, a = 1.0f / (1 + g * (g + k));
            float u = a * (jp.ic1 + g * (x - jp.ic2)), v = jp.ic2 + g * u;
            jp.ic1 = 2 * u - jp.ic1;
            jp.ic2 = 2 * v - jp.ic2;
            float y = jp.filter_mode == 1 ? v : x - k * u - v;
            jp.wet += filtering ? 1.0f / 88.0f : -1.0f / 88.0f;
            if (jp.wet < 0)
                jp.wet = 0;
            if (jp.wet > 1)
                jp.wet = 1;
            x += (y - x) * jp.wet;
        }
        out[i] = x * 5.62341325f; /* +15 dB total: previous +3 dB plus another +12 dB */
    }
}
void breaks_init(breaks_t *b)
{
    b->unused = 0;
    zero(&jp, sizeof jp);
    alpha_juno_init(&part_juno);
    jp.arp_note = jp.seq_note = -1;
    jp.off_left = jp.seq_left = -1;
    jp.rng = 0x61a49b27;
    jp.gate_gain = 1;
    for (int i = 0; i < JX_NPARAMS; i++)
        jp.value[i] = extra[i].def;
    juno_part_transport(125, 0);
}
void breaks_live(breaks_t *b, int n, int down)
{
    (void)b;
    juno_part_key(n, down ? 100 : 0);
}
void breaks_stop(breaks_t *b)
{
    (void)b;
    juno_part_midi(0xb0, 123, 0);
    jp.clock_left = 0;
    jp.free_tick = 0;
}
void breaks_render(breaks_t *b, float *out, int n)
{
    (void)b;
    if (!juno_part_division() && jp.seq_left < 0) {
        alpha_juno_render(&part_juno, out, n);
        post(out, n);
        return;
    }
    while (n > 0) {
        if (jp.seq_left == 0) sequence_off();
        if (jp.off_left == 0)
            arp_off();
        if (juno_part_division() && !jp.playing && jp.clock_left <= 0) {
            juno_part_tick(0, jp.free_tick);
            jp.clock_left += jp.tick_samples;
        }
        int k = n;
        if (juno_part_division() && !jp.playing && jp.clock_left < (float)k) {
            k = (int)jp.clock_left;
            if (k < 1)
                k = 1;
        }
        if (jp.off_left > 0 && jp.off_left < k)
            k = jp.off_left;
        if (jp.seq_left > 0 && jp.seq_left < k) k = jp.seq_left;
        alpha_juno_render(&part_juno, out, k);
        post(out, k);
        if (!jp.playing)
            jp.clock_left -= (float)k;
        if (jp.off_left > 0)
            jp.off_left -= k;
        if (jp.seq_left > 0) jp.seq_left -= k;
        out += k;
        n -= k;
    }
}
int breaks_silent(const breaks_t *b)
{
    (void)b;
    return 0;
}
void breaks_set_loop(breaks_t *b, int w, const brk_loop_t *l)
{
    (void)b;
    (void)w;
    (void)l;
}
void breaks_step(breaks_t *b, int s, int bar, float samples, int enabled)
{
    (void)b;
    (void)s;
    (void)bar;
    (void)samples;
    (void)enabled;
}
void breaks_pin(breaks_t *b, int s)
{
    (void)b;
    (void)s;
}
void breaks_set_quiet(breaks_t *b, int q)
{
    (void)b;
    (void)q;
}
int breaks_nparams(void)
{
    return 0;
}
const x0x_param_t *breaks_param(int i)
{
    (void)i;
    return 0;
}
void breaks_set(breaks_t *b, int i, int v)
{
    (void)b;
    (void)i;
    (void)v;
}
int breaks_get(const breaks_t *b, int i)
{
    (void)b;
    (void)i;
    return 0;
}
float breaks_send(const breaks_t *b, int w)
{
    (void)b;
    (void)w;
    return 0;
}
