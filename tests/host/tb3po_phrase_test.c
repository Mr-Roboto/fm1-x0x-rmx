/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/seq/tb3po.c"

static int sounding(const bpart_t *b)
{
    int n = 0;
    for (int i = 0; i < b->len; i++) n += bstep_gate(&b->step[i]) != G_REST;
    return n;
}

static void valid(const bpart_t *b)
{
    const scale_t *sc = &SCALES[b->gen.scale];
    int first = -1, prev = -1;
    for (int i = 0; i < NSTEPS; i++) {
        const bstep_t *s = &b->step[i];
        assert(s->note <= 127);
        assert(!(s->flags & ~(BS_GATE_MASK | BS_ACCENT | BS_SLIDE)));
        if (i >= b->len) { assert(s->flags == G_REST); continue; }
        if (bstep_gate(s) == G_REST) { assert(s->flags == G_REST); continue; }
        assert(bstep_gate(s) == G_NOTE);
        int pc = (s->note - b->gen.root + 120) % 12, found = 0;
        for (int d = 0; d < sc->len; d++) found |= pc == sc->deg[d];
        assert(found);
        assert(s->note >= note_of(&b->gen, 0, 0));
        assert(s->note < note_of(&b->gen, 0, 0) + b->gen.oct_range * 12);
        if (first < 0) first = i;
        if (prev >= 0) assert(distance(s->note, b->step[prev].note) <= 7);
        prev = i;
        if (s->flags & BS_SLIDE)
            assert(bstep_gate(&b->step[(i + 1) % b->len]) != G_REST);
        if (!b->gen.accent) assert(!(s->flags & BS_ACCENT));
        if (b->gen.accent == 100) assert(s->flags & BS_ACCENT);
        if (!b->gen.slide) assert(!(s->flags & BS_SLIDE));
    }
    assert(first >= 0);
    assert(distance(b->step[first].note, b->step[prev].note) <= 7);
}

/* Fixed seed corpus: musical tendencies, not a subjective quality score. */
static void test_phrasing(void)
{
    static const int lengths[] = {1, 8, 16, 18, 24, 32, 64};
    int varied = 0, developed = 0, syncopated = 0, joint = 0;
    int rhythm_edits = 0, articulation_edits = 0, pitch_edits = 0;
    int beat_acc = 0, beat_notes = 0, weak_acc = 0, weak_notes = 0;
    int target_slides = 0, target_pairs = 0, other_slides = 0, other_pairs = 0;
    for (int seed = 1; seed <= 128; seed++) {
        for (unsigned l = 0; l < sizeof lengths / sizeof lengths[0]; l++) {
            bpart_t b = {0};
            b.len = lengths[l];
            tb3po_defaults(&b.gen, seed * 7919u);
            tb3po_generate(&b);
            valid(&b);
            if (b.len >= 16) {
                int diff = 0;
                for (int i = 0; i < 8; i++)
                    diff |= bstep_gate(&b.step[i]) != bstep_gate(&b.step[i + 8]);
                varied += diff;
                /* The core of each question recurs in its answer. */
                for (int i = 0; i < 6; i++)
                    if (bstep_gate(&b.step[i]) == G_NOTE && bstep_gate(&b.step[i + 8]) == G_NOTE)
                        assert(b.step[i].note == b.step[i + 8].note);
            }
            if (b.len >= 32) {
                int diff = 0;
                for (int i = 0; i < 8; i++)
                    diff |= b.step[i].note != b.step[i + 16].note ||
                            bstep_gate(&b.step[i]) != bstep_gate(&b.step[i + 16]);
                developed += diff;
            }
            for (int i = 0; i < b.len; i++) {
                if (bstep_gate(&b.step[i]) != G_NOTE) continue;
                if (i % 4) syncopated++;
                if (i % 4 == 0) { beat_notes++; beat_acc += !!(b.step[i].flags & BS_ACCENT); }
                if (i % 4 == 1) { weak_notes++; weak_acc += !!(b.step[i].flags & BS_ACCENT); }
                joint += (b.step[i].flags & (BS_ACCENT | BS_SLIDE)) == (BS_ACCENT | BS_SLIDE);
                int nx = (i + 1) % b.len;
                if (nx == i || bstep_gate(&b.step[nx]) != G_NOTE) continue;
                int target = distance(b.step[i].note, b.step[nx].note) > 0 &&
                    (nx % 4 == 0 || (b.step[nx].note - b.gen.root + 120) % 12 == 0);
                if (target) { target_pairs++; target_slides += !!(b.step[i].flags & BS_SLIDE); }
                else { other_pairs++; other_slides += !!(b.step[i].flags & BS_SLIDE); }
            }
            uint32_t r = 0;
            for (int m = 0; m < 16; m++) {
                bpart_t before = b;
                tb3po_mutate(&b, &r);
                valid(&b);
                /* Inactive steps are not part of mutation, even if hand-written. */
                assert(!memcmp(b.step + b.len, before.step + b.len,
                               (NSTEPS - b.len) * sizeof(bstep_t)));
                for (int i = 0; i < b.len; i++) {
                    if (bstep_gate(&b.step[i]) != bstep_gate(&before.step[i])) rhythm_edits++;
                    else if (bstep_gate(&b.step[i]) == G_NOTE) {
                        pitch_edits += b.step[i].note != before.step[i].note;
                        articulation_edits += b.step[i].flags != before.step[i].flags;
                        if (b.step[i].note != before.step[i].note)
                            assert(distance(b.step[i].note, before.step[i].note) <= 3);
                    }
                }
            }
        }
    }
    assert(varied > 128 && developed > 128 && syncopated && joint);
    assert(beat_acc * weak_notes > weak_acc * beat_notes);
    assert(target_slides * other_pairs > other_slides * target_pairs);
    assert(rhythm_edits > pitch_edits && articulation_edits > pitch_edits && pitch_edits);
    printf("TB-3PO corpus: %d rhythmic answers, %d developed motifs; mutation rhythm/articulation/pitch %d/%d/%d\n",
           varied, developed, rhythm_edits, articulation_edits, pitch_edits);

    /* Partial final phrases return to the actual first step, not hidden step 0
     * of the next eight-step phrase. A one-step loop cannot slide into itself. */
    for (unsigned l = 0; l < sizeof lengths / sizeof lengths[0]; l++) {
        bpart_t b = {0};
        b.len = lengths[l];
        tb3po_defaults(&b.gen, 123);
        b.gen.density = b.gen.accent = b.gen.slide = 100;
        tb3po_generate(&b);
        assert(!!(b.step[b.len - 1].flags & BS_SLIDE) == (b.len > 1));
        for (int i = b.len; i < NSTEPS; i++) b.step[i] = (bstep_t){93, G_NOTE | BS_ACCENT};
        bpart_t before = b;
        uint32_t r = 1;
        tb3po_mutate(&b, &r);
        assert(!memcmp(b.step + b.len, before.step + b.len, (NSTEPS - b.len) * sizeof(bstep_t)));
    }
}

int main(void)
{
    test_phrasing();
    for (int scale = 0; scale < TB3PO_NSCALES; scale++) {
        for (int len = 1; len <= NSTEPS; len++) {
            bpart_t b = {0};
            b.len = len;
            tb3po_defaults(&b.gen, 7919u * len + scale);
            b.gen.scale = scale;
            b.gen.root = len % 12;
            b.gen.base_oct = len % 5;
            b.gen.oct_range = 1 + len % 3;
            b.gen.accent = b.gen.slide = len % 2 ? 0 : 100;
            int last_count = 0;
            for (int density = 0; density <= 100; density += 10) {
                b.gen.density = density;
                tb3po_generate(&b);
                bpart_t same = b;
                tb3po_generate(&same);
                assert(!memcmp(&b, &same, sizeof b));
                valid(&b);
                for (int i = 0; i < len; i++)
                    if (len > 1 && b.gen.slide == 100 && bstep_gate(&b.step[i]) != G_REST &&
                        bstep_gate(&b.step[(i + 1) % len]) != G_REST)
                        assert(b.step[i].flags & BS_SLIDE);
                assert(sounding(&b) >= last_count);
                last_count = sounding(&b);
                if (!density) assert(last_count == 1);
                if (density == 100) assert(last_count == len);
                uint32_t r = 0, sr = 0;
                for (int m = 0; m < 4; m++) {
                    bpart_t before = b;
                    tb3po_mutate(&b, &r);
                    tb3po_mutate(&same, &sr);
                    assert(r == sr && !memcmp(&b, &same, sizeof b));
                    assert(!memcmp(&b.gen, &before.gen, sizeof b.gen));
                    int changed = 0;
                    for (int i = 0; i < len; i++) {
                        if (i % 4 < 2) {
                            assert(b.step[i].note == before.step[i].note);
                            assert(bstep_gate(&b.step[i]) == bstep_gate(&before.step[i]));
                        }
                        changed += memcmp(&b.step[i], &before.step[i], sizeof(bstep_t)) != 0;
                    }
                    /* One neighbour's outgoing slide may also be cleared per edit. */
                    assert(changed <= 2 * ((len + 7) / 8));
                    valid(&b);
                }
            }
        }
    }
    /* Clamp corrupt lengths for accesses without rewriting persistent settings. */
    for (int length = 0; length <= 255; length += 255) {
        bpart_t invalid = {0}, bounded;
        tb3po_defaults(&invalid.gen, 0);
        invalid.len = length;
        bounded = invalid;
        bounded.len = length ? NSTEPS : 1;
        tb3po_generate(&invalid);
        tb3po_generate(&bounded);
        assert(invalid.len == length);
        assert(!memcmp(invalid.step, bounded.step, sizeof invalid.step));
        uint32_t ir = 0, br = 0;
        tb3po_mutate(&invalid, &ir);
        tb3po_mutate(&bounded, &br);
        assert(ir == br && !memcmp(invalid.step, bounded.step, sizeof invalid.step));
    }
    bpart_t motif = {0};
    motif.len = 64;
    tb3po_defaults(&motif.gen, 12345);
    motif.gen.density = 100;
    tb3po_generate(&motif);
    /* Short phrases recur, with a second motif and an answer every eight steps. */
    for (int i = 0; i < 6; i++) {
        assert(motif.step[i].note == motif.step[8 + i].note);
        assert(motif.step[i].note == motif.step[32 + i].note);
        assert(motif.step[16 + i].note == motif.step[48 + i].note);
    }
    /* Exercise actual mutation, rather than passing with a no-op implementation. */
    bpart_t before = motif;
    uint32_t r = 0;
    for (int m = 0; m < 16; m++) tb3po_mutate(&motif, &r);
    assert(memcmp(before.step, motif.step, sizeof motif.step));
    puts("TB-3PO phrases: all scales/lengths, density endpoints, bounded movement, motifs, articulation, deterministic generation/mutation and retained anchors passed");
    return 0;
}
