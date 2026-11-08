/* SPDX-License-Identifier: GPL-3.0-only */
#include "alpha_juno.h"
#include "alpha_juno_wave.h"
#include "fastmath.h"
typedef struct {
    char name[18];
    uint8_t value[J_NPARAMS];
} juno_preset_t;
#include "alpha_juno_presets.inc" /* immutable bank stays in flash */

const char *alpha_juno_preset_name(int index)
{
    return index >= 0 && index < JUNO_PRESET_COUNT ? juno_presets[index].name : "USER PATCH";
}
const uint8_t *alpha_juno_preset_values(int index)
{
    return index >= 0 && index < JUNO_PRESET_COUNT ? juno_presets[index].value : 0;
}
static void juno_zero(void *target, uint32_t size)
{
    uint8_t *p = (uint8_t *)target;
    for (uint32_t i = 0; i < size; i++) p[i] = 0;
}

static const char *const saw_names[] = {"OFF", "SAW", "SAW2", "PWM2", "CHOP8", "CHOP2+8"};
static const char *const pulse_names[] = {"OFF", "SQUARE", "PULSE", "PWM"};
static const char *const sub_names[] = {"SQ -1", "PW -1", "CHOP2", "CHOP4", "SQ -2", "PW -2"};
static const char *const modes[] = {"NORMAL", "INVERT", "VEL +", "VEL -"};
static const char *const vca_modes[] = {"ENV", "GATE", "VEL ENV", "VEL GATE"};
static const char *const ranges[] = {"32'", "16'", "8'", "4'"};
static const char *const sources[] = {"LFO", "ENV", "MANUAL"};
static const char *const shapes[] = {"TRI", "SAW", "SQUARE"};
static const char *const onoff[] = {"OFF", "ON"};
static const x0x_param_t params[J_NPARAMS] = {{"SAW", 5, 1, saw_names},
                                              {"PULSE", 3, 3, pulse_names},
                                              {"SUB", 5, 0, sub_names},
                                              {"SUB LVL", 127, 55, 0},
                                              {"NOISE", 127, 0, 0},
                                              {"RANGE", 3, 2, ranges},
                                              {"TUNE", 100, 50, 0},
                                              {"LEVEL", 127, 95, 0},
                                              {"PW", 127, 64, 0},
                                              {"PWM", 127, 65, 0},
                                              {"PWM RATE", 127, 40, 0},
                                              {"PWM SRC", 2, 0, sources},
                                              {"CUTOFF", 127, 86, 0},
                                              {"RESO", 127, 35, 0},
                                              {"ENV DEP", 127, 65, 0},
                                              {"VCF ENV", 3, 0, modes},
                                              {"HPF", 3, 0, 0},
                                              {"KEY TRK", 127, 64, 0},
                                              {"VCF LFO", 127, 0, 0},
                                              {"VCF VEL", 127, 0, 0},
                                              {"T1", 127, 8, 0},
                                              {"L1", 127, 127, 0},
                                              {"T2", 127, 55, 0},
                                              {"L2", 127, 80, 0},
                                              {"T3", 127, 65, 0},
                                              {"L3", 127, 70, 0},
                                              {"T4", 127, 45, 0},
                                              {"ENV KEY", 127, 0, 0},
                                              {"DCO ENV", 127, 0, 0},
                                              {"DCO MODE", 3, 0, modes},
                                              {"DCO LFO", 127, 0, 0},
                                              {"BEND RNG", 12, 2, 0},
                                              {"LFO RATE", 127, 45, 0},
                                              {"LFO DLY", 127, 0, 0},
                                              {"LFO FADE", 127, 15, 0},
                                              {"LFO SHAPE", 2, 0, shapes},
                                              {"VCA MODE", 3, 2, vca_modes},
                                              {"VCA VEL", 127, 60, 0},
                                              {"TOUCH", 127, 0, 0},
                                              {"DRIVE", 127, 12, 0},
                                              {"CHORUS", 1, 1, onoff},
                                              {"CHO RATE", 127, 30, 0},
                                              {"CHO DEP", 127, 60, 0},
                                              {"CHO MIX", 127, 50, 0}};
const x0x_param_t *alpha_juno_param(int i)
{
    return i >= 0 && i < J_NPARAMS ? &params[i] : 0;
}
static float clamp(float x, float lo, float hi)
{
    return x < lo ? lo : x > hi ? hi : x;
}
static float tri(float p)
{
    return 1.0f - 4.0f * fm_fabsf(p - 0.5f);
}
static float frac(float p)
{
    return p - (int)p;
}
static float sat(float x)
{
    return x / (1.0f + fm_fabsf(x));
}
static float seconds(int x)
{
    return 0.002f * fm_exp2f((float)x * (12.0f / 127.0f));
}
void alpha_juno_init(alpha_juno_t *s)
{
    juno_zero(s, sizeof *s);
    s->rng = 0x46a2bc19u;
    for (int i = 0; i < J_NPARAMS; i++)
        s->value[i] = params[i].def;
}
void alpha_juno_set(alpha_juno_t *s, int i, int v)
{
    if (i >= 0 && i < J_NPARAMS)
        s->value[i] = (uint8_t)clamp((float)v, 0, (float)params[i].max);
}
static void stage(alpha_juno_t *s, juno_voice_t *v, int st)
{
    v->stage = (uint8_t)st;
    v->env_from = v->env;
    int ti = st == 0 ? J_T1 : st == 1 ? J_T2 : st == 2 ? J_T3 : J_T4;
    v->env_to = st == 3 ? 0 : (float)s->value[st == 0 ? J_L1 : st == 1 ? J_L2 : J_L3] / 127.0f;
    float scale = fm_exp2f(-(float)s->value[J_ENVKEY] / 127.0f * ((float)v->note - 60.0f) / 24.0f);
    v->env_step = 1.0f / (seconds(s->value[ti]) * scale * 44100.0f);
    v->amp = 0; /* progress through this stage, not the VCA gain */
}
void alpha_juno_preset(alpha_juno_t *s, int index)
{
    const uint8_t *values = alpha_juno_preset_values(index);
    if (!values) return;
    for (int i = 0; i < J_NPARAMS; i++) alpha_juno_set(s, i, values[i]);
    /* Held chords audition the new envelope; releases still refer to the same notes. */
    for (int i = 0; i < JUNO_VOICES; i++) {
        juno_voice_t *v = &s->voice[i];
        if (v->active && (v->gate || v->sustained)) {
            v->age = 0.0f;
            stage(s, v, 0);
        }
    }
}
void alpha_juno_note(alpha_juno_t *s, int note, int velocity)
{
    if (note < 0 || note > 127)
        return;
    if (velocity <= 0) {
        alpha_juno_off(s, note);
        return;
    }
    int pick = -1;
    for (int i = 0; i < JUNO_VOICES; i++)
        if (s->voice[i].active && s->voice[i].note == note) {
            pick = i;
            break;
        }
    if (pick < 0)
        for (int i = 0; i < JUNO_VOICES; i++)
            if (!s->voice[i].active) {
                pick = i;
                break;
            }
    if (pick < 0)
        for (int i = 0; i < JUNO_VOICES; i++)
            if (!s->voice[i].gate && (pick < 0 || s->voice[i].env < s->voice[pick].env))
                pick = i;
    if (pick < 0) {
        pick = 0;
        for (int i = 1; i < JUNO_VOICES; i++)
            if (s->voice[i].serial < s->voice[pick].serial)
                pick = i;
    }
    juno_voice_t *v = &s->voice[pick];
    float previous = v->serial ? v->pitch : (float)note;
    juno_zero(v, sizeof *v);
    v->pitch = s->glide > 0.0f ? previous : (float)note;
    v->active = v->gate = 1;
    v->note = (uint8_t)note;
    v->vel = clamp((float)velocity / 127.0f, 0, 1);
    v->serial = ++s->serial;
    stage(s, v, 0);
}
void alpha_juno_mono(alpha_juno_t *s, int note, int velocity, int legato)
{
    juno_voice_t *v = &s->voice[0];
    if (legato && v->active) {
        v->sequenced = 0; v->seq_left = 0;
        v->note = (uint8_t)note;
        v->vel = clamp((float)velocity / 127.0f, 0, 1);
        v->gate = 1; v->sustained = 0;
        if (s->glide == 0.0f) v->pitch = (float)note;
        return;
    }
    float previous = v->serial ? v->pitch : (float)note;
    juno_zero(v, sizeof *v);
    v->pitch = s->glide > 0.0f ? previous : (float)note;
    v->active = v->gate = 1; v->note = (uint8_t)note;
    v->vel = clamp((float)velocity / 127.0f, 0, 1);
    v->serial = ++s->serial; stage(s, v, 0);
}
void alpha_juno_off(alpha_juno_t *s, int note)
{
    for (int i = 0; i < JUNO_VOICES; i++) {
        juno_voice_t *v = &s->voice[i];
        if (v->active && v->note == note && v->gate) {
            v->gate = 0;
            if (s->sustain)
                v->sustained = 1;
            else
                stage(s, v, 3);
        }
    }
}
void alpha_juno_all_off(alpha_juno_t *s, int immediate)
{
    s->sustain = 0;
    for (int i = 0; i < JUNO_VOICES; i++) {
        juno_voice_t *v = &s->voice[i];
        v->gate = v->sustained = 0;
        v->seq_left = 0;
        if (immediate)
            v->active = 0;
        else if (v->active)
            stage(s, v, 3);
    }
    if (immediate)
        juno_zero(s->delay, sizeof s->delay);
}
void alpha_juno_midi(alpha_juno_t *s, int ty, int a, int b)
{
    if (ty == 0x90 && b)
        alpha_juno_note(s, a, b);
    else if (ty == 0x80 || ty == 0x90)
        alpha_juno_off(s, a);
    else if (ty == 0xe0)
        s->bend = ((float)(a + 128 * b) - 8192.0f) / 8192.0f;
    else if (ty == 0xd0)
        s->pressure = (float)a / 127.0f;
    else if (ty == 0xb0) {
        if (a == 1)
            s->wheel = (float)b / 127.0f;
        else if (a == 64) {
            s->sustain = b >= 64;
            if (!s->sustain)
                for (int i = 0; i < JUNO_VOICES; i++)
                    if (s->voice[i].sustained) {
                        s->voice[i].sustained = 0;
                        stage(s, &s->voice[i], 3);
                    }
        } else if (a == 120 || a == 123)
            alpha_juno_all_off(s, a == 120);
    }
}
int alpha_juno_active(const alpha_juno_t *s)
{
    for (int i = 0; i < JUNO_VOICES; i++)
        if (s->voice[i].active)
            return 1;
    return 0;
}
static float envmod(float e, int mode, float vel)
{
    return e * (mode & 1 ? -1.0f : 1.0f) * (mode >= 2 ? vel : 1.0f);
}
static float lowpass(juno_voice_t *v, int j, float x)
{
    float a = 1.0f / (1.0f + v->g * (v->g + v->k));
    float u = a * (v->ic1[j] + v->g * (x - v->ic2[j]));
    float y = v->ic2[j] + v->g * u;
    v->ic1[j] = 2 * u - v->ic1[j];
    v->ic2[j] = 2 * y - v->ic2[j];
    return y;
}
void alpha_juno_render(alpha_juno_t *s, float *out, int n)
{
    const uint8_t *p = s->value;
    float lr = 0.05f * fm_exp2f((float)p[J_LFORATE] * 8.5f / 127.0f) / 44100.0f;
    float pr = 0.03f * fm_exp2f((float)p[J_PWMRATE] * 9.0f / 127.0f) / 44100.0f;
    float cr = (0.1f + (float)p[J_CHORATE] * 0.025f) / 44100.0f;
    float hfc = p[J_HPF] ? 40.0f * fm_exp2f((float)p[J_HPF] * 1.7f) : 15.0f;
    float hp = 1.0f / (1.0f + 6.2831853f * hfc / 44100.0f);
    float glide = s->glide > 0.0f ? 1.0f - fm_expf(-1.0f / (44100.0f * s->glide)) : 1.0f;
    for (int k = 0; k < n; k++) {
        s->lfo = frac(s->lfo + lr);
        s->pwm = frac(s->pwm + pr);
        s->chorus = frac(s->chorus + cr);
        float lfo = p[J_LFOSHAPE] == 0   ? tri(s->lfo)
                    : p[J_LFOSHAPE] == 1 ? 2 * s->lfo - 1
                                         : (s->lfo < 0.5f ? 1 : -1);
        s->rng ^= s->rng << 13;
        s->rng ^= s->rng >> 17;
        s->rng ^= s->rng << 5;
        float noise = (float)(int32_t)s->rng * (1.0f / 2147483648.0f);
        float sum = 0;
        for (int j = 0; j < JUNO_VOICES; j++) {
            juno_voice_t *v = &s->voice[j];
            if (!v->active)
                continue;
            v->age += 1.0f / 44100.0f;
            if (v->sequenced) {
                if (v->seq_left) {
                    --v->seq_left;
                    v->pitch = v->seq_target - v->seq_delta * (float)v->seq_left;
                }
            } else v->pitch += ((float)v->note - v->pitch) * glide;
            if (v->stage < 4) {
                v->amp = clamp(v->amp + v->env_step, 0, 1);
                v->env = v->env_from + (v->env_to - v->env_from) * v->amp;
                if (v->amp >= 1) {
                    if (v->stage < 2)
                        stage(s, v, v->stage + 1);
                    else if (v->stage == 2)
                        v->stage = 4;
                    else {
                        v->active = 0;
                        continue;
                    }
                }
            }
            /* Sequencer boundaries split render calls: keep modulation cadence in synth time,
             * so ties cannot force extra pitch/filter updates. New voices update immediately. */
            if (!s->control_phase || v->inc == 0.0f) {
                float ld = clamp((v->age - (p[J_LFODELAY] ? seconds(p[J_LFODELAY]) : 0)) /
                                     seconds(p[J_LFOFADE]),
                                 0, 1);
                float mod = lfo * ld;
                float pitch = v->pitch + 12.0f * ((int)p[J_RANGE] - 2) +
                              (p[J_TUNE] - 50) * 0.02f + s->bend * p[J_BEND];
                pitch +=
                    envmod(v->env, p[J_DCOMODE], v->vel) * (float)p[J_DCOENV] * (24.0f / 127.0f);
                pitch += mod * ((float)p[J_DCOLFO] / 127.0f * 2.0f + s->wheel * 0.5f);
                v->inc =
                    clamp(440.0f * fm_exp2f((pitch - 69.0f) / 12.0f) / 44100.0f, 0.00001f, 0.45f);
                float cut = (float)p[J_CUTOFF] +
                            envmod(v->env, p[J_VCFMODE], v->vel) * (float)p[J_VCFENV] * 0.65f;
                cut += ((float)v->note - 60.0f) * (float)p[J_KEYTRACK] / 127.0f +
                       mod * (float)p[J_VCFLFO] * 0.4f;
                cut += (v->vel - 0.5f) * p[J_VCFVEL] * 0.5f + s->pressure * p[J_TOUCH] * 0.5f;
                float fc = clamp(20.0f * fm_exp2f(cut * 9.64f / 127.0f), 20, 16000);
                v->g = fm_tanf(3.14159265f * fc / 44100.0f);
                v->k = 2.0f - 1.75f * (float)p[J_RESO] / 127.0f;
            }
            v->phase = frac(v->phase + v->inc);
            v->subphase = frac(v->subphase + v->inc * 0.25f);
            float ph = v->phase;
            float pw = 0.05f + (float)p[J_PW] / 127.0f * 0.9f;
            float pwm = p[J_PWMSOURCE] == 0   ? tri(s->pwm)
                        : p[J_PWMSOURCE] == 1 ? 2 * v->env - 1
                                              : 0;
            pw = clamp(pw + pwm * (float)p[J_PWM] / 127.0f * 0.45f, 0.03f, 0.97f);
            float saw = juno_saw_wave(ph, pw, p[J_SAW]);
            float pulse = 0;
            if (p[J_PULSE]) {
                float width = p[J_PULSE] == 1 ? 0.5f : p[J_PULSE] == 2 ? 0.75f : pw;
                pulse = juno_pulse_wave(ph, width);
            }
            float sub = juno_sub_wave(v->subphase, ph, p[J_SUB]);
            float x = (saw + pulse + sub * (float)p[J_SUBLEVEL] / 127.0f +
                       noise * (float)p[J_NOISE] / 127.0f) *
                      0.3f;
            float drive = 1.0f + (float)p[J_DRIVE] / 127.0f * 7.0f;
            x = sat(x * drive) / (1.0f + drive * 0.15f);
            float y = lowpass(v, 1, lowpass(v, 0, x));
            float amp = (p[J_VCAMODE] & 1) ? ((v->gate || v->sustained) ? 1.0f : v->env) : v->env;
            if (p[J_VCAMODE] >= 2)
                amp *= 1.0f - (float)p[J_VCAVEL] / 127.0f * (1.0f - v->vel);
            sum += y * amp;
        }
        sum *= (float)p[J_LEVEL] / 127.0f * 0.35f;
        float high = hp * (s->hp_y + sum - s->hp_x);
        s->hp_x = sum;
        s->hp_y = high;
        sum = high;
        s->delay[s->write & (JUNO_CHORUS_LEN - 1u)] = sum;
        float d = 500.0f + tri(s->chorus) * (float)p[J_CHODEPTH] * 2.5f;
        float pos = (float)(s->write & (JUNO_CHORUS_LEN - 1u)) + (float)JUNO_CHORUS_LEN - d;
        int ix = (int)pos;
        float f = pos - (float)ix;
        float wet = s->delay[ix & (JUNO_CHORUS_LEN - 1u)] + f * (s->delay[(ix + 1) & (JUNO_CHORUS_LEN - 1u)] - s->delay[ix & (JUNO_CHORUS_LEN - 1u)]);
        s->write++;
        if (p[J_CHORUS]) {
            float m = (float)p[J_CHOMIX] / 127.0f * 0.65f;
            sum = sum * (1 - m) + wet * m;
        }
        out[k] = sum;
        s->control_phase = (uint8_t)((s->control_phase + 1u) & 15u);
    }
}
