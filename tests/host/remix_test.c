/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "seq/remix.h"
#include "x0x_remix_table.h"

int main(void)
{
    unsigned bank, pattern, slot, count = 0, repeated = 0, beyond_length = 0;
    unsigned char expected[256][16];
    const unsigned lengths[] = {8, 16, 24, 32, 40, 48, 64};
    char line[256];
    FILE *source = fopen("assets/remix/remixes.txt", "r");
    assert(source);
    while (fgets(line, sizeof line, source)) {
        char *p = line, *end;
        if (*p == '#')
            continue;
        assert(count < 256);
        for (slot = 0; slot < 16; slot++) {
            long value = strtol(p, &end, 10);
            assert(end != p && value >= 0 && value <= 15);
            expected[count][slot] = (unsigned char)value;
            p = end;
        }
        count++;
    }
    assert(!ferror(source));
    fclose(source);
    assert(count == 256);
    assert(sizeof remix_packed == 2048);
    for (bank = 0; bank < 16; bank++) {
        for (pattern = 0; pattern < 16; pattern++) {
            unsigned row = bank * 16 + pattern, seen = 0, length_index;
            assert(remix_lookup(bank, pattern, 0) == 0);
            for (slot = 0; slot < 16; slot++) {
                unsigned index = row * 16 + slot;
                unsigned value = expected[row][slot];
                assert(((remix_packed[index / 2] >> (4 * (index % 2))) & 15) == value);
                assert(remix_lookup(bank, pattern, slot) == value);
                if (seen & (1u << value))
                    repeated++;
                seen |= 1u << value;
                if (row == 0)
                    assert(value == slot);
            }
            for (length_index = 0; length_index < sizeof lengths / sizeof lengths[0]; length_index++) {
                unsigned length = lengths[length_index];
                for (slot = 0; slot < length; slot++) {
                    unsigned block_base = (slot / 16) * 16;
                    unsigned value = block_base + expected[row][slot % 16];
                    assert(remix_source_step(bank, pattern, slot) == value);
                    assert(value >= block_base && value < block_base + 16);
                    beyond_length += value >= length;
                    if (row == 0)
                        assert(value == slot);
                }
            }
        }
    }
    /* Historical A2 repeats slots 0..3 at 4..7. */
    assert(remix_source_step(0, 1, 0) == remix_source_step(0, 1, 4));
    assert(remix_source_step(0, 1, 16) == remix_source_step(0, 1, 20));
    assert(repeated > 0 && beyond_length > 0);
    puts("remix: all 4096 lookups, packed entries, lengths and four block offsets passed");
    return 0;
}
