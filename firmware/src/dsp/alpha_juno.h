/* SPDX-License-Identifier: GPL-3.0-only */
/* Raw six-voice Alpha Juno-inspired synth. No allocation or oversampling.
 * Design reference: mikerodd/june-21 (GPL-3.0), especially its DCO wave
 * combinations, T1/L1/T2/L2/T3/L3/T4 envelope and modulation routing.
 * This is an original compact implementation, not a Csound port. */
#pragma once
#include "x0x_param.h"
#include <stdint.h>
#define JUNO_VOICES 6
#define JUNO_PRESET_COUNT 64
#define JUNO_CHORUS_LEN 1024u /* maximum modulated delay is 817.5 samples */
enum {
    J_SAW,
    J_PULSE,
    J_SUB,
    J_SUBLEVEL,
    J_NOISE,
    J_RANGE,
    J_TUNE,
    J_LEVEL,
    J_PW,
    J_PWM,
    J_PWMRATE,
    J_PWMSOURCE,
    J_CUTOFF,
    J_RESO,
    J_VCFENV,
    J_VCFMODE,
    J_HPF,
    J_KEYTRACK,
    J_VCFLFO,
    J_VCFVEL,
    J_T1,
    J_L1,
    J_T2,
    J_L2,
    J_T3,
    J_L3,
    J_T4,
    J_ENVKEY,
    J_DCOENV,
    J_DCOMODE,
    J_DCOLFO,
    J_BEND,
    J_LFORATE,
    J_LFODELAY,
    J_LFOFADE,
    J_LFOSHAPE,
    J_VCAMODE,
    J_VCAVEL,
    J_TOUCH,
    J_DRIVE,
    J_CHORUS,
    J_CHORATE,
    J_CHODEPTH,
    J_CHOMIX,
    J_NPARAMS
};
typedef struct {
    float phase, subphase, env, env_from, env_to, env_step, vel;
    float ic1[2], ic2[2], g, k, inc, amp, age, pitch;
    float seq_target, seq_delta;
    uint32_t seq_left;
    uint32_t serial;
    uint8_t note, gate, stage, active, sustained, sequenced;
} juno_voice_t;
typedef struct {
    juno_voice_t voice[JUNO_VOICES];
    uint8_t value[J_NPARAMS], sustain, control_phase;
    float lfo, pwm, chorus, hp_x, hp_y, bend, wheel, pressure, glide;
    float delay[JUNO_CHORUS_LEN];
    uint32_t serial, rng, write;
} alpha_juno_t;
const x0x_param_t *alpha_juno_param(int i);
void alpha_juno_init(alpha_juno_t *s);
void alpha_juno_set(alpha_juno_t *s, int i, int value);
void alpha_juno_note(alpha_juno_t *s, int note, int velocity);
void alpha_juno_off(alpha_juno_t *s, int note);
void alpha_juno_all_off(alpha_juno_t *s, int immediate);
void alpha_juno_midi(alpha_juno_t *s, int type, int d1, int d2);
void alpha_juno_render(alpha_juno_t *s, float *out, int n);
int alpha_juno_active(const alpha_juno_t *s);
const char *alpha_juno_preset_name(int index);
const uint8_t *alpha_juno_preset_values(int index);
void alpha_juno_preset(alpha_juno_t *s, int index);
void alpha_juno_mono(alpha_juno_t *s, int note, int velocity, int legato);
