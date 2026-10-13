/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline drum generation. Never call from the sequencer or audio ISR. */
#pragma once
#include "pattern.h"

enum { DG_MAP, DG_EUCLID };
enum { DG_KICK, DG_SNARE, DG_HAT, DG_CHANNELS };
enum { DG_NEW, DG_MUTATE };
enum { DG_UNCHANGED, DG_CHANGED, DG_NEW_FIRST, DG_INVALID };

/* Session state only: never embed this in a pattern/project or Undo region.
 * X/Y are bytes; CHAOS/FILL are percentages. LEN is 1..min(32, drum length).
 * OFFSET is 0..15 in MAP, 0..LEN-1 in EUCLID; positive shifts later. */
typedef struct {
    uint16_t rng;
    uint8_t mode, x, y, chaos;
    uint8_t fill[DG_CHANNELS], len[DG_CHANNELS], offset[DG_CHANNELS];
    uint8_t reserved;                 /* transient padding, no persistence semantics */
} drum_gen_cfg_t;

extern drum_gen_cfg_t drum_gen_session[NKIT]; /* 909 and 808, independent; restart defaults */
void drum_gen_init(uint16_t seed);
void drum_gen_defaults(drum_gen_cfg_t *cfg, uint16_t seed);
void drum_gen_normalize(drum_gen_cfg_t *cfg, int pattern_len);

/* Prepared replacement, including hidden steps copied from the original.
 * No pattern writes or session changes during preparation. P6.3 can commit
 * pending Undo before apply, then commit this edit immediately afterward.
 * Publish only with audio interrupts excluded (or while audio is stopped).
 * Keep rhythm calculation/Undo outside that short critical section.
 * A NEW may advance RNG even if its persistent output is unchanged; apply
 * that session update without creating an Undo entry. A failed/ineffective
 * MUTATE leaves both session and pattern unchanged: do not apply it.
 * Do not edit the destination between prepare and apply. */
typedef struct {
    uint32_t hit[DG_CHANNELS][NSW], accent[NSW];
    drum_gen_cfg_t cfg;
} drum_gen_edit_t;

int drum_gen_new(const dpart_t *drum, const drum_gen_cfg_t *cfg, int kit, drum_gen_edit_t *edit);
int drum_gen_mutate(const dpart_t *drum, const drum_gen_cfg_t *cfg, int kit, drum_gen_edit_t *edit);
void drum_gen_apply(dpart_t *drum, drum_gen_cfg_t *cfg, int kit, const drum_gen_edit_t *edit);
