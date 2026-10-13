/* SPDX-License-Identifier: GPL-3.0-only */
/* Focused production integration checks; no audio files or spectral matrix. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "waveform_prototype.h"
/* Access oscillator helpers without adding a production test API. */
#include "../../firmware/src/dsp/bass303.c"

void baseline_bass303_init(void *b);
void baseline_bass303_set(void *b, int i, int value);
void baseline_bass303_note_on(void *b, int note, int accent, int slide);
void baseline_bass303_note_off(void *b);
void baseline_bass303_render(void *b, float *out, int n);
void baseline_bass303_set_lite(void *b, int on);
void oracle_bass303_render(bass303_t *b, float *out, int n);

static int fails;
#define CHECK(c, label) do { if (!(c)) { puts("FAIL: " label); fails++; } } while (0)
static int bits_equal(float a, float b) { return memcmp(&a, &b, sizeof(a)) == 0; }

static void oscillator(void)
{
    const float morphs[] = {0, 32, 63, 63.999f, 64, 64.001f, 65, 96, 127};
    const float increments[] = {0.00001f, 0.001f, 0.01f, 0.15f, 0.25f, 0.31f, 0.49f};
    bass303_morph_mix_t dut;
    waveform_mix_t ref;
    unsigned m, d;
    int p, mismatch = 0;
    for (m = 0; m < sizeof(morphs)/sizeof(*morphs); m++) {
        morph_prepare(&dut, morphs[m]);
        waveform_prepare(&ref, morphs[m]);
        for (d = 0; d < sizeof(increments)/sizeof(*increments); d++) {
            for (p = 0; p < 1024; p++) {
                float phase = (float)p / 1024.0f;
                float a = morph_eval(&dut, phase, increments[d]);
                float b = waveform_eval(&ref, phase, increments[d], WAVEFORM_CUBIC);
                mismatch += !bits_equal(a, b);
            }
        }
    }
    CHECK(mismatch == 0, "cubic oscillator/prototype bit identity (events and periodic overlap)");
    /* Both sides of 64 must converge to the same trapezoid. */
    for (p = 0; p < 1024; p++) {
        float phase = (float)p / 1024.0f, a, b;
        morph_prepare(&dut, 63.999f); a = morph_eval(&dut, phase, 0.01f);
        morph_prepare(&dut, 64.001f); b = morph_eval(&dut, phase, 0.01f);
        CHECK(fabsf(a - b) <= 1e-6f, "central morph continuity");
    }
}

static void controls(void)
{
    bass303_t b;
    uint8_t pots[BASS303_NPARAMS];
    bass303_init(&b);
    memcpy(pots, b.pot, sizeof(pots));
    CHECK(b.morph == 64.0f && b.morph_target == 64, "default trapezoid");
    CHECK(bass303_nparams() == 13 && BASS303_MORPH == 12, "Morph index 12, thirteen parameters");
    CHECK(bass303_param(BASS303_WAVE)->max == 2 && bass303_param(12)->max == 127,
          "Wave family and Morph exposed");
    bass303_set_waveform(&b, 2);
    bass303_set_morph(&b, -10);
    CHECK(b.wave == 2 && b.morph == 0.0f, "internal mode and low clamp");
    bass303_set_morph(&b, 500);
    CHECK(b.morph == 127.0f && b.morph_target == 127, "high clamp, idle settle");
    CHECK(memcmp(pots, b.pot, sizeof(pots)) == 0, "internal controls preserve pots");
    bass303_set_waveform(&b, 3);
    CHECK(b.wave == 2, "invalid mode ignored");
    bass303_set(&b, BASS303_WAVE, 2);
    CHECK(b.wave == 2 && b.pot[BASS303_WAVE] == 2, "public Wave setter selects Morph");
}

static void legacy(void)
{
    bass303_t dut, ref;
    float a[256], b[256];
    int wave, drive, block, mismatch = 0;
    for (wave = 0; wave < 2; wave++) for (drive = 0; drive < 3; drive++) {
        bass303_init(&dut); memset(&ref, 0, sizeof(ref)); baseline_bass303_init(&ref);
        bass303_set(&dut, BASS303_WAVE, wave); baseline_bass303_set(&ref, BASS303_WAVE, wave);
        bass303_set(&dut, BASS303_DRVTYPE, drive); baseline_bass303_set(&ref, BASS303_DRVTYPE, drive);
        bass303_set(&dut, BASS303_DRIVE, 80); baseline_bass303_set(&ref, BASS303_DRIVE, 80);
        for (block = 0; block < 320; block++) {
            int n = block < 280 ? 256 : block % 4 == 0 ? 1 : block % 4 == 1 ? 4 : block % 4 == 2 ? 64 : 128;
            if (block == 0 || block == 5 || block == 9 || block == 312) {
                int note = block == 5 ? 57 : block == 9 ? 127 : 33;
                int accent = block == 5 || block == 312;
                bass303_note_on(&dut, note, accent, block == 5);
                baseline_bass303_note_on(&ref, note, accent, block == 5);
            }
            if (block == 10 || block == 313) {
                bass303_note_off(&dut); baseline_bass303_note_off(&ref);
            }
            if (block == 6 || block == 8) {
                bass303_set_lite(&dut, block == 6); baseline_bass303_set_lite(&ref, block == 6);
            }
            if (block == 311) CHECK(dut.idle, "legacy test reaches idle before retrigger");
            bass303_set_morph(&dut, block % 128); /* Must have no effect on legacy output. */
            bass303_render(&dut, a, n); baseline_bass303_render(&ref, b, n);
            mismatch += memcmp(a, b, (size_t)n * sizeof(float)) != 0;
        }
    }
    CHECK(mismatch == 0, "Saw/Square baseline bits, accent/slide/drive/Lite/release/idle/retrigger");
}

static void voice(void)
{
    const int morphs[] = {0, 32, 63, 64, 65, 96, 127};
    bass303_t a, b, isolated_a, isolated_b, oracle;
    float x[256], y[256], ref[256];
    int drive, block, mismatch = 0, finite = 1, slew = 1;
    for (drive = 0; drive < 3; drive++) {
        bass303_init(&a); bass303_init(&b);
        bass303_set_waveform(&a, 2); bass303_set_waveform(&b, 2);
        bass303_set_morph(&a, 0); bass303_set_morph(&b, 127);
        bass303_set(&a, BASS303_DRVTYPE, drive); bass303_set(&b, BASS303_DRVTYPE, drive);
        bass303_set(&a, BASS303_DRIVE, 80); bass303_set(&b, BASS303_DRIVE, 127);
        bass303_note_on(&a, 45, 1, 0); bass303_note_on(&b, 60, 0, 0);
        isolated_a = a; isolated_b = b; oracle = a;
        for (block = 0; block < 40; block++) {
            int target = morphs[block % 7];
            float before;
            int n = block % 4 == 0 ? 1 : block % 4 == 1 ? 4 : block % 4 == 2 ? 64 : 256;
            bass303_set_morph(&a, target); bass303_set_morph(&isolated_a, target);
            bass303_set_morph(&oracle, target);
            bass303_set_morph(&b, 127 - target); bass303_set_morph(&isolated_b, 127 - target);
            if (block == 12 || block == 20) {
                bass303_set_lite(&a, block == 12); bass303_set_lite(&isolated_a, block == 12);
                bass303_set_lite(&oracle, block == 12);
            }
            if (block == 15) {
                bass303_note_on(&a, 127, 1, 1); bass303_note_on(&isolated_a, 127, 1, 1);
                bass303_note_on(&oracle, 127, 1, 1);
            }
            before = a.morph;
            bass303_render(&a, x, n); bass303_render(&b, y, n);
            bass303_render(&isolated_a, ref, n);
            mismatch += memcmp(x, ref, (size_t)n * sizeof(float)) != 0;
            oracle_bass303_render(&oracle, ref, n);
            mismatch += memcmp(x, ref, (size_t)n * sizeof(float)) != 0;
            bass303_render(&isolated_b, ref, n);
            mismatch += memcmp(y, ref, (size_t)n * sizeof(float)) != 0;
            slew &= fabsf(a.morph - (float)target) <= fabsf(before - (float)target);
            if (n >= 225) slew &= a.morph == (float)target;
            for (int i = 0; i < n; i++) finite &= isfinite(x[i]) && isfinite(y[i]);
        }
    }
    CHECK(mismatch == 0, "full voice/prototype identity and per-instance isolation during Morph/Lite/slide");
    CHECK(finite, "finite Wave 3 output across landmarks, drive and high pitch");
    CHECK(slew, "monotone finite Morph slew and exact settled targets");
}
int main(void)
{
    oscillator(); controls(); legacy(); voice();
    printf("bass303 waveform OS%d CTRL%d: %s; instance %zu B\n", BASS303_OS, BASS303_CTRL,
           fails ? "FAIL" : "PASS", sizeof(bass303_t));
    return fails != 0;
}
