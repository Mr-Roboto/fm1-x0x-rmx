/* SPDX-License-Identifier: GPL-3.0-only */
#include "remix.h"
#include "x0x_remix_table.h"

uint8_t remix_lookup(uint8_t bank, uint8_t pattern, uint8_t slot)
{
    unsigned index = ((unsigned)bank * 16u + pattern) * 16u + slot;
    uint8_t packed = remix_packed[index >> 1];
    return (uint8_t)((packed >> ((index & 1u) * 4u)) & 15u);
}

uint8_t remix_source_step(uint8_t bank, uint8_t pattern, uint8_t chronological_slot)
{
    return (uint8_t)((chronological_slot & 0x30u) +
                     remix_lookup(bank, pattern, chronological_slot & 15u));
}
