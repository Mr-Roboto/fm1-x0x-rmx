/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "alpha_juno.h"
#include "../seq/pattern.h"
enum {
    JX_MODE,
    JX_GLIDE,
    JX_TRANCE,
    JX_GATE0,
    JX_GATE1,
    JX_GATE2,
    JX_GATE3,
    JX_FILTER,
    JX_ARP,
    JX_ARP_PATTERN,
    JX_ARP_RATE,
    JX_ARP_OCT,
    JX_ARP_GATE,
    JX_LATCH,
    JX_NPARAMS
};
#define JUNO_EXTRA_BANK 3
#define JUNO_EXTRA_TAG 0xB7
#define JUNO_GATE_DEFAULT 0xADADu
extern alpha_juno_t part_juno;
const x0x_param_t *juno_part_param(int i);
void juno_part_set(int i, int value);
void juno_part_key(int note, int velocity);
void juno_part_midi(int type, int d1, int d2);
void juno_part_tick(void *ctx, unsigned tick);
void juno_part_transport(float bpm, int playing);
unsigned juno_part_division(void);
unsigned juno_part_gate_step(void);

void juno_part_sequence(int note, int samples);

void juno_part_sequence_step(int note, int samples, unsigned flags, int hold,
                             int target, int slide_samples);
