/* SPDX-License-Identifier: GPL-3.0-only */
/* JUNO fifth-part bridge: keep mixer/project layout while replacing BREAK. */
#include "alpha_juno.h"
#include "breaks.h"
#include "../app/plat.h"
alpha_juno_t part_juno X0X_POOL;
void breaks_init(breaks_t *b)
{
    b->unused = 0;
    alpha_juno_init(&part_juno);
}
void breaks_live(breaks_t *b, int key, int down)
{
    (void)b;
    if (down)
        alpha_juno_note(&part_juno, key, 100);
    else
        alpha_juno_off(&part_juno, key);
}
void breaks_stop(breaks_t *b)
{
    (void)b;
    alpha_juno_all_off(&part_juno, 0);
}
void breaks_render(breaks_t *b, float *out, int n)
{
    (void)b;
    alpha_juno_render(&part_juno, out, n);
}
int breaks_silent(const breaks_t *b)
{
    (void)b;
    return 0; /* include chorus/release tails */
}
void breaks_set_loop(breaks_t *b, int which, const brk_loop_t *loop)
{
    (void)b;
    (void)which;
    (void)loop;
}
void breaks_step(breaks_t *b, int step, int bar, float samples, int enabled)
{
    (void)b;
    (void)step;
    (void)bar;
    (void)samples;
    (void)enabled;
}
void breaks_pin(breaks_t *b, int slice)
{
    (void)b;
    (void)slice;
}
void breaks_set_quiet(breaks_t *b, int quiet)
{
    (void)b;
    (void)quiet;
}
int breaks_nparams(void)
{
    return 0;
}
const x0x_param_t *breaks_param(int i)
{
    (void)i;
    return 0;
}
void breaks_set(breaks_t *b, int i, int value)
{
    (void)b;
    (void)i;
    (void)value;
}
int breaks_get(const breaks_t *b, int i)
{
    (void)b;
    (void)i;
    return 0;
}
float breaks_send(const breaks_t *b, int which)
{
    (void)b;
    (void)which;
    return 0;
}
