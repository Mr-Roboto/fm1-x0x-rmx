/* SPDX-License-Identifier: GPL-3.0-only */
#include "../../firmware/src/dsp/alpha_juno.h"
#include "../../firmware/src/dsp/alpha_juno_wave.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static alpha_juno_t synth;
static float audio[256];

static void plain(void)
{
    alpha_juno_init(&synth);
    alpha_juno_set(&synth, J_CHORUS, 0);
    alpha_juno_set(&synth, J_SUBLEVEL, 0);
    alpha_juno_set(&synth, J_PULSE, 0);
    alpha_juno_set(&synth, J_CUTOFF, 127);
    alpha_juno_set(&synth, J_RESO, 0);
    alpha_juno_set(&synth, J_VCFENV, 0);
    alpha_juno_set(&synth, J_L1, 127);
    alpha_juno_set(&synth, J_L2, 127);
    alpha_juno_set(&synth, J_L3, 127);
    alpha_juno_set(&synth, J_T1, 0);
}

int main(void)
{
    /* Gate placement and DC: cover whole cycles, including all four sub cycles. */
    for (int mode=0;mode<=5;mode++) {
        double saw_mean=0,sub_mean=0;
        for(int i=0;i<65536;i++) {
            float phase=((float)i+0.5f)/65536.0f;
            saw_mean+=juno_saw_wave(phase,0.5f,mode);
            sub_mean+=juno_sub_wave(phase,juno_wave_frac(phase*4.0f),mode);
        }
        assert(fabs(saw_mean/65536.0)<0.00001);
        assert(fabs(sub_mean/65536.0)<0.00001);
    }
    assert(fabsf(juno_saw_wave(.34f,.5f,5)-(2*.34f-.328125f))<1e-6f);
    assert(juno_saw_wave(.28f,.5f,5)==-.328125f); /* old XOR selected this wrong gate */
    assert(juno_saw_wave(.16f,.5f,2)==-.3125f);  /* gated off, never polarity-flipped */
    puts("Raw DCO: exact gate windows, relative polarity and zero DC over full cycles passed");
    for (int preset = 0; preset < JUNO_PRESET_COUNT; preset++) {
        alpha_juno_init(&synth);
        alpha_juno_preset(&synth, preset);
        const uint8_t *values = alpha_juno_preset_values(preset);
        assert(values && alpha_juno_preset_name(preset)[0]);
        for (int i = 0; i < J_NPARAMS; i++) {
            assert(values[i] <= alpha_juno_param(i)->max);
            assert(synth.value[i] == values[i]);
        }
        for (int old = 0; old < preset; old++)
            assert(memcmp(values, alpha_juno_preset_values(old), J_NPARAMS));
        alpha_juno_note(&synth, 60, 100);
        double power = 0;
        for (int block = 0; block < 400; block++) {
            alpha_juno_render(&synth, audio, 256);
            for (int i = 0; i < 256; i++) {
                assert(isfinite(audio[i]) && fabsf(audio[i]) < 8.0f);
                power += audio[i] * audio[i];
            }
        }
        if (!(power > 0.001)) fprintf(stderr, "Silent preset %d %s\n", preset, alpha_juno_preset_name(preset));
        assert(power > 0.001);
    }
    puts("All 64 presets: unique, valid controls and audible finite audio passed");
    plain();
    alpha_juno_note(&synth, 69, 127);
    int crossings = 0;
    float previous = 0;
    for (int b = 0; b < 200; b++) {
        alpha_juno_render(&synth, audio, 256);
        if (b < 20)
            continue;
        for (int i = 0; i < 256; i++) {
            if (previous <= 0 && audio[i] > 0)
                crossings++;
            previous = audio[i];
        }
    }
    float hz = crossings * 44100.0f / (180.0f * 256.0f);
    assert(fabsf(hz - 440.0f) < 2.0f);

    /* Saw variants must have actual, distinct oscillator output. */
    float waves[5][256];
    for (int mode = 1; mode <= 5; mode++) {
        plain();
        alpha_juno_set(&synth, J_SAW, mode);
        alpha_juno_note(&synth, 60, 127);
        for (int b = 0; b < 20; b++)
            alpha_juno_render(&synth, audio, 256);
        memcpy(waves[mode - 1], audio, sizeof audio);
        double power = 0;
        for (int i = 0; i < 256; i++)
            power += audio[i] * audio[i];
        assert(power > 0.0001);
        for (int j = 0; j < mode - 1; j++)
            assert(memcmp(waves[j], audio, sizeof audio));
    }

    /* Wide pitch/parameter ranges, repeated steals, maximum resonance and drive. */
    alpha_juno_init(&synth);
    uint32_t rng = 1234567;
    for (int block = 0; block < 3000; block++) {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        int p = block % J_NPARAMS;
        alpha_juno_set(&synth, p,
                       (block & 1) ? alpha_juno_param(p)->max
                                   : rng % (alpha_juno_param(p)->max + 1));
        if (!(block % 7))
            alpha_juno_note(&synth, rng % 128, 1 + rng % 127);
        if (!(block % 11))
            alpha_juno_off(&synth, rng % 128);
        alpha_juno_render(&synth, audio, 256);
        for (int i = 0; i < 256; i++)
            assert(isfinite(audio[i]) && fabsf(audio[i]) < 8.0f);
        for (int i = 0; i < JUNO_VOICES; i++) {
            assert(isfinite(synth.voice[i].ic1[0]) && isfinite(synth.voice[i].ic2[1]));
        }
    }
    alpha_juno_all_off(&synth, 1);
    assert(!alpha_juno_active(&synth));
    printf("Alpha Juno DSP: A4 %.2f Hz; five distinct saw variants; six-voice stealing and 17 "
           "seconds of extreme control/pitch stress finite; state %zu bytes\n",
           hz, sizeof synth);
    return 0;
}
