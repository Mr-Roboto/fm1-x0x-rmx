/* SPDX-License-Identifier: GPL-3.0-only */
/* Isolated implementation of docs/TB303_WAVEFORMS_DESIGN.md. */
#include "waveform_prototype.h"
#include "../../firmware/src/dsp/fastmath.h"

static float blep(float t, float d)
{
    float x;
    if (t < d) {
        x = t / d;
        return x + x - x * x - 1.0f;
    }
    if (t > 1.0f - d) {
        x = (t - 1.0f) / d;
        return x * x + x + x + 1.0f;
    }
    return 0.0f;
}
/* Signed event distance avoids rounding frac(phase+.5) near one and
 * amplifying that rounding by 1/d. Equivalent to the documented residuals
 * for d<.5, including coincident supports from different events. */
static inline float distance(float p, float event)
{
    float t = p - event;
    if (t < -0.5f) t += 1.0f;
    if (t > 0.5f) t -= 1.0f;
    return t;
}
static inline float event_blep(float t, float d)
{
    float x = fm_fabsf(t);
    if (x >= d) return 0.0f;
    x = 1.0f - x / d;
    return t < 0.0f ? x * x : -x * x;
}
static inline float blamp(float t, float d)
{
    float x = fm_fabsf(t);
    if (x >= d) return 0.0f;
    x = 1.0f - x / d;
    return (d / 6.0f) * x * x * x;
}
/* Four equal boxes of phase width d convolve to a cubic B-spline kernel.
 * tail(r) is its integral from r*d to infinity, r >= 0. The BLEP residual
 * for a 2-high step is +/-2*tail. The BLAMP is d*integral_r^infinity tail.
 * Use the far piece's small powers near support boundaries, avoiding the
 * cancellation in a generic truncated-power implementation. */
static inline void cubic_residual(float t, float d, int need_step, float *step, float *slope)
{
    float r = fm_fabsf(t), tail, ramp;
    *step = *slope = 0.0f;
    if (r >= 2.0f * d) return;
    r /= d;
    if (r >= 1.0f) {
        float x = 2.0f - r, x2 = x * x, x4 = x2 * x2;
        tail = need_step ? x4 * (1.0f / 24.0f) : 0.0f;
        ramp = x4 * x * (1.0f / 120.0f);
    } else {
        float r2 = r * r;
        tail = need_step ? 0.5f + r * (-2.0f / 3.0f
               + r2 * (1.0f / 3.0f - r * 0.125f)) : 0.0f;
        ramp = 7.0f / 30.0f + r * (-0.5f + r * (1.0f / 3.0f
               + r2 * (-1.0f / 12.0f + r * (1.0f / 40.0f))));
    }
    *step = t < 0.0f ? 2.0f * tail : -2.0f * tail;
    *slope = d * ramp;
}
static float cubic_active_correction(float t, float d, float jump, float slope)
{
    float b, r, y;
    cubic_residual(t, d, jump != 0.0f, &b, &r);
    y = jump * b + slope * r;
    /* For d>.25 the support exceeds a half-cycle. Periodic images of the
     * SAME event can overlap, in addition to overlaps between different events.
     * For 0<d<.5 only this adjacent image can contribute. */
    if (d > 0.25f) {
        cubic_residual(t < 0.0f ? t + 1.0f : t - 1.0f, d, jump != 0.0f, &b, &r);
        y += jump * b + slope * r;
    }
    return y;
}
static inline float cubic_correction(float t, float d, float jump, float slope)
{
    /* Keep the very common unsupported event cheap even if the compiler
     * chooses not to inline the larger polynomial/image helper. A nearest
     * image outside support implies every other image is outside as well. */
    if (fm_fabsf(t) >= 2.0f * d) return 0.0f;
    return cubic_active_correction(t, d, jump, slope);
}
static float precise_compensation(float variance)
{
    float c;
    if (variance == 1.0f) return 1.0f;
    c = fm_rsqrtf(variance);
    /* One additional Newton step, outside the sample loop. The exact unity
     * branch makes converging endpoint weights use exact endpoint gain. */
    return c * (1.5f - 0.5f * variance * c * c);
}
void waveform_prepare(waveform_mix_t *v, float m)
{
    const float gs = 0.6454972243679028f, gt = 0.7071067811865475f;
    const float gn = 0.8295613557843402f;
    float x, w, c;
    m = fm_clampf(m, 0.0f, 127.0f);
    v->endpoint = m == 0.0f ? 0 : m == 64.0f ? 1 : m == 127.0f ? 2 : -1;
    v->shark = v->trap = v->notch = 0.0f;
    if (v->endpoint >= 0) {
        if (v->endpoint == 0) v->shark = gs;
        else if (v->endpoint == 1) v->trap = gt;
        else { v->trap = gn; v->notch = gn; }
    } else if (m < 64.0f) {
        x = m / 64.0f;
        w = x * x * (3.0f - 2.0f * x);
        c = precise_compensation((1.0f - w) * (1.0f - w) + w * w
                     + 2.0f * 0.3423265984407288f * w * (1.0f - w));
        v->shark = c * (1.0f - w) * gs;
        v->trap = c * w * gt;
    } else {
        x = (m - 64.0f) / 63.0f;
        w = x * x * (3.0f - 2.0f * x);
        c = precise_compensation((1.0f - w) * (1.0f - w) + w * w
                     + 2.0f * 0.9532062476387964f * w * (1.0f - w));
        v->trap = c * ((1.0f - w) * gt + w * gn);
        v->notch = c * w * gn;
    }
    v->dc = v->notch * 0.125f;
    v->jump = 1.5f * v->shark;
    v->slope[0] = 6.0f * v->shark - 8.0f * v->trap - 8.0f * v->notch;
    v->slope[1] = 16.0f * v->notch;
    v->slope[2] = -8.0f * v->trap - 8.0f * v->notch;
    v->slope[3] = v->slope[4] = 8.0f * v->trap;
}
float waveform_eval(const waveform_mix_t *v, float phase, float d, int corrected)
{
    float u = phase < 0.5f ? phase + 0.5f : phase - 0.5f;
    float y = v->dc;
    if (v->shark != 0.0f) y += v->shark * (1.0f - 3.0f * u * u);
    if (v->trap != 0.0f) {
        float t = u < 0.25f ? 1.0f : u < 0.5f ? 3.0f - 8.0f * u
                  : u < 0.75f ? -1.0f : 8.0f * u - 7.0f;
        y += v->trap * t;
    }
    if (v->notch != 0.0f && u < 0.25f)
        y -= v->notch * (u < 0.125f ? 8.0f * u : 2.0f - 8.0f * u);
    if (corrected == WAVEFORM_TRIANGULAR) {
        float t = distance(phase, 0.5f);
        if (v->jump != 0.0f) y += v->jump * event_blep(t, d) - 0.5f * v->shark * d * d;
        y += v->slope[0] * blamp(t, d);
        if (v->slope[1] != 0.0f) y += v->slope[1] * blamp(distance(phase, 0.625f), d);
        if (v->slope[2] != 0.0f) y += v->slope[2] * blamp(distance(phase, 0.75f), d);
        if (v->slope[3] != 0.0f) {
            y += v->slope[3] * blamp(distance(phase, 0.0f), d);
            y += v->slope[4] * blamp(distance(phase, 0.25f), d);
        }
    } else if (corrected == WAVEFORM_CUBIC) {
        float t = distance(phase, 0.5f);
        if (v->jump != 0.0f) y -= v->shark * d * d;
        y += cubic_correction(t, d, v->jump, v->slope[0]);
        if (v->slope[1] != 0.0f)
            y += cubic_correction(distance(phase, 0.625f), d, 0.0f, v->slope[1]);
        if (v->slope[2] != 0.0f)
            y += cubic_correction(distance(phase, 0.75f), d, 0.0f, v->slope[2]);
        if (v->slope[3] != 0.0f) {
            y += cubic_correction(distance(phase, 0.0f), d, 0.0f, v->slope[3]);
            y += cubic_correction(distance(phase, 0.25f), d, 0.0f, v->slope[4]);
        }
    }
    return y;
}
/* Host copy of bass303.c's actual allpass halfband ordering and coefficients.
 * No highpass, ladder, envelope or post-filter processing is included. */
static float halfband(waveform_ap_t *s, float a, float x)
{
    float y = a * (x - s->y1) + s->x1;
    s->x1 = x;
    s->y1 = y;
    return y;
}
static float halfband_one(waveform_ap_t *s, float old, float newer)
{
    float a = newer, c = old;
    a = halfband(&s[0], 0.068204076f, a);
    c = halfband(&s[1], 0.240270358f, c);
    a = halfband(&s[2], 0.448676236f, a);
    c = halfband(&s[3], 0.641122367f, c);
    a = halfband(&s[4], 0.799997564f, a);
    c = halfband(&s[5], 0.934482236f, c);
    return 0.5f * (a + c);
}
static float halfband_two(waveform_ap_t *s, float old, float newer)
{
    float a = newer, c = old;
    a = halfband(&s[0], 0.104996100f, a);
    c = halfband(&s[1], 0.375357314f, c);
    a = halfband(&s[2], 0.754467339f, a);
    return 0.5f * (a + c);
}
void waveform_decimate(waveform_decimator_t *s, float *out, const float *in, int n, int os)
{
    int i;
    for (i = 0; i < n; i++) {
        if (os == 1) out[i] = in[i];
        else if (os == 2) out[i] = halfband_one(s->hb1, in[2*i], in[2*i+1]);
        else {
            float old = halfband_two(s->hb2, in[4*i], in[4*i+1]);
            float newer = halfband_two(s->hb2, in[4*i+2], in[4*i+3]);
            out[i] = halfband_one(s->hb1, old, newer);
        }
    }
}
float waveform_endpoint(int shape, float p, float d, int corrected)
{
    waveform_mix_t v;
    waveform_prepare(&v, shape == 0 ? 0.0f : shape == 1 ? 64.0f : 127.0f);
    return waveform_eval(&v, p, d, corrected);
}
void waveform_samples(float *out, const float *p, int n, float d, float m, int corrected)
{
    waveform_mix_t v;
    int i;
    waveform_prepare(&v, m);
    for (i = 0; i < n; i++) out[i] = waveform_eval(&v, p[i], d, corrected);
}
/* Exact existing oscillator expressions, copied solely for host benchmarking.
 * Square constants are initialized using the same firmware helpers. */
static float legacy_sq_dc, legacy_sq_h2;
void waveform_legacy_init(void)
{
    float f = fm_db2lin(36.9f), o = 4.37f;
    float plus = (f + o) + fm_logf(1.0f + fm_expf(-2.0f * (f + o)));
    float minus = (f - o) + fm_logf(1.0f + fm_expf(-2.0f * (f - o)));
    legacy_sq_dc = -(plus - minus) / (2.0f * f);
    legacy_sq_h2 = 0.5f * (fm_tanhf(f + o) + fm_tanhf(f - o));
}
float waveform_legacy(int square, float phase, float inc)
{
    float t;
    if (!square) {
        t = phase + 0.5f;
        if (t >= 1.0f) t -= 1.0f;
        return -(t + t - 1.0f - blep(t, inc));
    } else {
        const float sq_dc = legacy_sq_dc, sq_h2 = legacy_sq_h2;
        return 0.5f * (fm_tanhf(69.98419960f * (phase + phase - 1.0f) + 4.37f)
                      + sq_dc - sq_h2 * blep(phase, inc));
    }
}
