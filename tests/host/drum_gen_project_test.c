/* SPDX-License-Identifier: GPL-3.0-only */
/* The core's ordinary steps use existing project storage and Undo unchanged. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include "../../firmware/src/seq/drum_gen.h"
#include <assert.h>

int main(void)
{
    pattern_t saved[2];
    drum_gen_cfg_t session[NKIT];
    drum_gen_edit_t edit;
    project_defaults();
    seq_init(&seq, proj.pat);
    drum_gen_init(1234);
    for (int kit = 0; kit < NKIT; kit++) {
        dpart_t *d = &proj.pat[kit].drum[kit];
        drum_gen_cfg_t *g = &drum_gen_session[kit];
        d->len = kit ? 64 : 24;
        d->rate = RATE_32;
        pattern_set_remix_selection(&proj.pat[kit], kit, 7, 11);
        pattern_set_remix_enabled(&proj.pat[kit], kit, 1);
        sm_set(d->hit[2], 4);
        sm_set(d->accent, 4);
        if (kit) g->mode = DG_EUCLID;
        else g->chaos = 60;
        assert(drum_gen_new(d, g, kit, &edit) == DG_CHANGED);
        drum_gen_apply(d, g, kit, &edit);
        saved[kit] = proj.pat[kit];
    }
    memcpy(session, drum_gen_session, sizeof session);
    assert(project_save() == 0);
    project_defaults();
    assert(project_load() == 0);
    for (int kit = 0; kit < NKIT; kit++) assert(!memcmp(&saved[kit], &proj.pat[kit], sizeof(pattern_t)));
    assert(!memcmp(session, drum_gen_session, sizeof session));

    /* Explicit test commits exercise existing Undo storage only. Production
     * button/transaction wiring is deferred to P6.3. No empty entry on NEW. */
    {
        dpart_t *d = &proj.pat[1].drum[1];
        drum_gen_cfg_t *g = &drum_gen_session[1];
        dpart_t original = *d, mutated;
        uint32_t lo, hi;
        undo_start();
        assert(drum_gen_new(d, g, 1, &edit) == DG_UNCHANGED);
        drum_gen_apply(d, g, 1, &edit);
        assert(undo_commit(&undo) == 0);
        assert(drum_gen_mutate(d, g, 1, &edit) == DG_CHANGED);
        drum_gen_apply(d, g, 1, &edit);
        mutated = *d;
        assert(undo_commit(&undo) == 1);
        session[1] = *g;
        assert(undo_undo(&undo, 0, &lo, &hi) == 1);
        assert(!memcmp(d, &original, sizeof original));
        assert(!memcmp(g, &session[1], sizeof *g));
        assert(drum_gen_mutate(d, g, 1, &edit) == DG_NEW_FIRST);
        assert(undo_redo(&undo, 0, &lo, &hi) == 1);
        assert(!memcmp(d, &mutated, sizeof mutated));
        assert(!memcmp(g, &session[1], sizeof *g));
    }
    puts("drum generator project: generated steps save/load, Remix metadata, session exclusion, existing Undo/Redo and no empty NEW passed");
    return 0;
}
