/* SPDX-License-Identifier: GPL-3.0-only */
/* Filter response/bounds and a common bypass oracle, compiled with the pre-filter
 * renderer as well as the new renderer. No reference tolerances are weakened. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "breaks.c"
#include "x0x_break_bank.h"

static void setup(breaks_t *b, int pot)
{
    brk_loop_t loop = {X0X_BREAK_0, X0X_BREAK_0_N, X0X_BREAK_RATE, 2};
    breaks_init(b);
    breaks_set_loop(b, 0, &loop);
    loop.adpcm = X0X_BREAK_1;
    loop.nsamples = X0X_BREAK_1_N;
    breaks_set_loop(b, 1, &loop);
#ifndef BREAK_FILTER_REFERENCE
    breaks_set(b, BRK_FILT, pot);
#else
    (void)pot;
#endif
}

/* Same inputs, clocks, live controls and audio blocks in both implementations. */
static void render(FILE *f, int pot)
{
    for (int c = 0; c < 6; ++c) {
        breaks_t b;
        float out[256];
        const int block = c % 3 == 0 ? 1 : c % 3 == 1 ? 17 : 256;
        const int step_len = c < 3 ? 7350 : 3891; /* 90 / 170 BPM */
        setup(&b, pot);
        breaks_set(&b, BRK_PITCH, (c % 3) * 12);
        breaks_set(&b, BRK_LEVEL, 110);
        breaks_set(&b, BRK_ROLL, 35);
        breaks_set(&b, BRK_R2, 40);
        for (int pos = 0; pos < 65536;) {
            int step = pos / step_len;
            if (pos % step_len == 0) {
                if (step == 2) breaks_live(&b, 8, 1);
                if (step == 3) breaks_live(&b, 8, 0);
                if (step == 4) breaks_live(&b, 9, 1);
                if (step == 5) breaks_live(&b, 9, 0);
                if (step == 6) breaks_live(&b, 10, 1);
                if (step == 7) breaks_live(&b, 10, 0);
                if (step == 8) breaks_stop(&b);
                if (step != 8) { /* render a real stopped interval before restarting */
                    breaks_pin(&b, step % 8);
                    breaks_step(&b, step % 16, step / 16, (float)step_len, step % 4 != 3);
                }
            }
            int n = step_len - pos % step_len;
            if (n > block) n = block;
            if (n > 65536 - pos) n = 65536 - pos;
            breaks_render(&b, out, n);
            assert(fwrite(out, sizeof(float), (size_t)n, f) == (size_t)n);
            pos += n;
        }
        uint32_t state[] = {b.rng, b.st_trigs, b.v[0].pos, b.v[1].pos,
                            (uint32_t)b.current_slice, (uint32_t)b.trigger_count};
        assert(fwrite(state, sizeof state, 1, f) == 1);
    }
}

static volatile float bench_sink;
static double benchmark(int pot)
{
    breaks_t b;
    float out[256];
    setup(&b, pot);
    breaks_step(&b, 0, 0, 5512.5f, 1);
    clock_t begin = clock();
    for (int i = 0; i < 65536; ++i) {
        breaks_render(&b, out, 256);
        bench_sink = out[127];
    }
    return (double)(clock() - begin) / CLOCKS_PER_SEC;
}

#ifndef BREAK_FILTER_REFERENCE
static double response(int pot, double hz)
{
    breaks_t b;
    float x[256];
    double sum = 0;
    breaks_init(&b);
    breaks_set(&b, BRK_FILT, pot);
    for (int pos = 0; pos < 88200;) {
        int n = 88200 - pos < 256 ? 88200 - pos : 256;
        for (int i = 0; i < n; ++i)
            x[i] = (float)(0.3 * sin(6.283185307179586 * hz * (pos + i) / BRK_FS));
        brk_filter(&b, x, n);
        for (int i = 0; i < n; ++i)
            if (pos + i >= 44100) sum += (double)x[i] * x[i];
        pos += n;
    }
    return sqrt(sum / 44100) / (0.3 / sqrt(2.0));
}

static void filter_tests(void)
{
    breaks_t b;
    float x[256], copy[256];
    breaks_init(&b);
    assert(breaks_param(BRK_FILT)->def == 64 && breaks_get(&b, BRK_FILT) == 64);
    breaks_set(&b, BRK_FILT, -999); assert(breaks_get(&b, BRK_FILT) == 0);
    breaks_set(&b, BRK_FILT, 999); assert(breaks_get(&b, BRK_FILT) == 127);
    for (int v = 60; v <= 68; ++v) {
        for (int i = 0; i < 256; ++i) copy[i] = x[i] = (float)(i - 128) / 256.0f;
        b.f_g = 0.5f; b.f_ic1 = 0.75f; b.f_ic2 = -0.3f;
        breaks_set(&b, BRK_FILT, v); brk_filter(&b, x, 256);
        assert(!memcmp(x, copy, sizeof x));
        assert(b.f_g == 0 && b.f_ic1 == 0 && b.f_ic2 == 0);
    }
    for (int v = 0; v <= 127; ++v) {
        breaks_init(&b); breaks_set(&b, BRK_FILT, v);
        memset(x, 0, sizeof x); x[0] = 1;
        for (int block = 0; block < 128; ++block) {
            brk_filter(&b, x, 256);
            for (int i = 0; i < 256; ++i) assert(isfinite(x[i]) && fabsf(x[i]) < 2);
            memset(x, 0, sizeof x);
        }
    }
    const int pots[] = {0, 30, 59, 69, 90, 127};
    const double freqs[] = {20, 100, 1000, 8000};
    for (unsigned p = 0; p < sizeof pots / sizeof *pots; ++p) {
        int v = pots[p];
        double fc = v < 60 ? 18000 * exp2((v - 60) * (8.0 / 60))
                           : 20 * exp2((v - 68) * (8.5 / 59));
        for (unsigned f = 0; f < sizeof freqs / sizeof *freqs; ++f) {
            double t = tan(3.141592653589793 * freqs[f] / BRK_FS) /
                       tan(3.141592653589793 * fc / BRK_FS);
            double expected = 1 / sqrt((1 - t*t)*(1 - t*t) + 1.44*t*t);
            if (v > 68) expected *= t*t;
            double actual = response(v, freqs[f]);
            assert(fabs(actual - expected) < 0.005 * expected + 0.00001);
        }
    }
    printf("LP 8k gain %.8f; HP 100Hz gain %.8f\n", response(0, 8000), response(127, 100));
    breaks_init(&b); breaks_set(&b, BRK_FILT, 30);
    memset(x, 0, sizeof x); brk_filter(&b, x, 256);
    float previous = b.f_g;
    breaks_set(&b, BRK_FILT, 0); brk_filter(&b, x, 256);
    float target = fm_tanf(FM_PI * (18000.0f / 256.0f) / (float)BRK_FS);
    assert(b.f_g == previous + 0.5f * (target - previous));
    puts("PASS: all parameter bounds, 9 transparent OFF values, 128 stable impulse responses, LP/HP curves/Q and block glide");
}
#endif

int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "bench")) {
        printf("%.9f\n", benchmark(argc > 2 ? atoi(argv[2]) : 64));
    } else if (argc > 1) {
        FILE *f = fopen(argv[1], "wb"); assert(f);
        render(f, argc > 2 ? atoi(argv[2]) : 64);
        assert(fclose(f) == 0);
    } else {
#ifndef BREAK_FILTER_REFERENCE
        filter_tests();
#endif
    }
    return 0;
}
