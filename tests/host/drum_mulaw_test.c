/* SPDX-License-Identifier: GPL-3.0-only */
/* Compiled against both the release PCM renderer and the current mu-law renderer.
 * The Python runner supplies identical decoded samples and compares all outputs. */
#include <time.h>
#include <string.h>
#include "drum909.c"

static void setup(drum909_t *d, int pitch, int dist)
{
    drum909_init(d);
    for (int v = DR_CH; v <= DR_RD; ++v) {
        for (int i = 0; i < drum909_nparams(v); ++i) {
            const char *name = drum909_param(v, i)->name;
            if (!strcmp(name, "Pitch")) drum909_set(d, v, i, pitch);
            if (!strcmp(name, "Decay")) drum909_set(d, v, i, 127);
            if (!strcmp(name, "Drive")) drum909_set(d, v, i, dist ? 90 : 0);
            if (!strcmp(name, "Dist")) drum909_set(d, v, i, dist);
            if (!strcmp(name, "Rev") || !strcmp(name, "Dly")) drum909_set(d, v, i, 64);
            if (!strcmp(name, "Pan")) drum909_set(d, v, i, (v & 1) ? 16 : 112);
        }
    }
}

void render_case(int voice, int pitch, int dist, int block, int edge, float *out, uint32_t *state)
{
    drum909_t d;
    setup(&d, pitch, dist);
    drum909_trigger(&d, DR_CH + voice, 0.83f);
    if (edge) {
        d.smp[voice].pos = d.smp[voice].len - 1;
        d.smp[voice].frac = 0x80000000u;
    }
    memset(out, 0, 60000 * 4 * sizeof(float));
    for (int pos = 0; pos < 60000;) {
        int n = 60000 - pos < block ? 60000 - pos : block;
        if (!edge && pos >= 2048 && pos - n < 2048)
            drum909_trigger(&d, DR_CH + (voice ^ 1), 0.5f);
        if (!edge && pos >= 4096 && pos - n < 4096)
            drum909_trigger(&d, DR_CH + voice, 1.0f);
        drum909_render_st(&d, out + pos, out + 60000 + pos,
                          out + 120000 + pos, out + 180000 + pos, n);
        pos += n;
    }
    for (int i = 0; i < 4; ++i) {
        state[4*i] = d.smp[i].pos;
        state[4*i+1] = d.smp[i].frac;
        state[4*i+2] = d.smp[i].mute;
        state[4*i+3] = d.smp[i].out.left;
    }
}

unsigned engine_bytes(void) { return sizeof(drum909_t); }

static volatile float bench_sink;

double benchmark(int repeats)
{
    drum909_t d;
    float dry[256], rev[256], dly[256];
    setup(&d, 0, 0);
    clock_t begin = clock();
    for (int r = 0; r < repeats; ++r) {
        drum909_trigger(&d, DR_OH, 1.0f);
        drum909_trigger(&d, DR_CR, 1.0f);
        drum909_trigger(&d, DR_RD, 1.0f);
        for (int b = 0; b < 128; ++b) {
            memset(dry, 0, sizeof(dry));
            memset(rev, 0, sizeof(rev));
            memset(dly, 0, sizeof(dly));
            drum909_render(&d, dry, rev, dly, 256);
            bench_sink = dry[127] + rev[127] + dly[127];
        }
    }
    return (double)(clock() - begin) / CLOCKS_PER_SEC;
}
