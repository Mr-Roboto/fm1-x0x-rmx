/* SPDX-License-Identifier: GPL-3.0-only */
/* Raw DCO gate algebra adapted from mikerodd/june-21's June21 opcode:
 * https://github.com/mikerodd/june-21/blob/master/src/cabbage-module/june-21.csd
 * Csound vco2's saw falls from +1 to -1; its square starts positive.
 * The 8x square has an additional half-cycle phase offset. Keep the reference
 * gate products/relative levels; subtract analytic DC before our drive/filter
 * (June-21 instead removes DC in its pre-VCF HPF). No band limiting or tables. */
#pragma once

static inline float juno_wave_frac(float p) { return p - (int)p; }

static inline float juno_pulse_wave(float phase, float width)
{
    return (phase < width ? -1.0f : 1.0f) - (1.0f - 2.0f * width);
}

static inline float juno_saw_wave(float phase, float width, int mode)
{
    float gate2 = juno_wave_frac(phase * 2.0f);
    float gate8 = juno_wave_frac(phase * 8.0f);
    switch (mode) {
    case 1: return 2.0f * phase - 1.0f;
    case 2: /* (-saw + 1) * (-square2 + 1) / 4 */
        return (gate2 >= 0.5f ? phase : 0.0f) - 0.3125f;
    case 3: /* (-saw + 1) * (-pwm2 + 1) / 2 */
        return (gate2 >= width ? 2.0f * phase : 0.0f)
             - (1.0f - width) * (1.0f + 0.5f * width);
    case 4: /* (-saw + 1) * (-square8(phase + .5) + 1) */
        return (gate8 < 0.5f ? 4.0f * phase : 0.0f) - 0.9375f;
    case 5: /* (-saw + 1) * positive8(phase + .5) * negative2: AND, not XOR */
        return (gate8 >= 0.5f && gate2 >= 0.5f ? 2.0f * phase : 0.0f) - 0.328125f;
    default: return 0.0f;
    }
}

static inline float juno_sub_wave(float quarter_phase, float phase, int mode)
{
    float p = mode >= 4 ? quarter_phase : juno_wave_frac(quarter_phase * 2.0f);
    if (mode == 2 || mode == 3) {
        float multiplier = mode == 2 ? 2.0f : 4.0f;
        return (p >= 0.5f && juno_wave_frac(phase * multiplier) >= 0.5f ? 2.0f : 0.0f)
             - 0.5f;
    }
    /* June-21 negates the square/pulse; its fixed pulse has 75% duty. */
    float width = mode == 1 || mode == 5 ? 0.75f : 0.5f;
    return juno_pulse_wave(p, width);
}
