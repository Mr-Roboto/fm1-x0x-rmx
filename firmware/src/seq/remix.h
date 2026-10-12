/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>

/* Preconditions: bank and pattern 0..15 (A..P, 1..16), slot 0..15.
 * Returns the zero-based source step within a 16-step block. */
uint8_t remix_lookup(uint8_t bank, uint8_t pattern, uint8_t slot);

/* Forward mapping, chronological_slot 0..63. Pattern length controls which
 * chronological slots the caller visits, never the available source choices.
 * A partial block may therefore select a source beyond chronological length. */
uint8_t remix_source_step(uint8_t bank, uint8_t pattern, uint8_t chronological_slot);
