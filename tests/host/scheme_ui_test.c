/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared LCD renderer, real GLOBAL input, legacy appearance and project storage. */
#define engine_gr_db scheme_test_gr_db
#define engine_brk_slot_names scheme_test_brk_slot_names
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#undef engine_gr_db
#undef engine_brk_slot_names
const char *const *engine_brk_slot_names(void);
static int legacy_loop_names;
const char *const *scheme_test_brk_slot_names(void)
{
    static const char *const old_names[] = {"909 GR", "909 FL"};
    return legacy_loop_names ? old_names : engine_brk_slot_names();
}
static float test_gr;
float scheme_test_gr_db(void) { return test_gr; }
#include <assert.h>
#include "../../firmware/src/dsp/drum808.h"

static const int appearance_views[] = {V_HOME, V_PART, V_GEN, V_FX, V_MIX, V_SONG, V_PERF, V_SEQ};

static void fixture(int palette)
{
    test_gr = 0.0f;
    memset(&proj, 0, sizeof proj);
    memset(&ui, 0, sizeof ui);
    memset(enc_acc, 0, sizeof enc_acc);
    held_btn = held_keys = now_ms = 0;
    project_defaults();
    /* Explicit legacy fixture, independent of the current factory scheme. */
    settings_set_scheme(&proj.set, SCHEME_DEFAULT);
    drum_gen_init(0);
    proj.set.palette = (uint8_t)palette;
    proj.pat[0].drum[0].hit[DR_BD][0] = 0x1111;
    proj.pat[0].drum[0].hit[DR_SD][0] = 0x1010;
    proj.pat[0].drum[0].hit[DR_CH][0] = 0x5555;
    proj.pat[0].drum[1].hit[D8_BD][0] = 0x1111;
    proj.pat[0].drum[1].hit[D8_SD][0] = 0x1010;
    proj.pat[0].drum[1].hit[D8_CH][0] = 0x5555;
    proj.pat[0].brk.steps = 0x5555;
    for (int b = 0; b < NBASS; b++)
        for (int s = 0; s < 16; s++) {
            proj.pat[0].bass[b].step[s].note = 48 + b * 12 + s % 7;
            proj.pat[0].bass[b].step[s].flags = s % 4 == 3 ? G_REST : G_NOTE | (s % 3 == 0 ? BS_ACCENT : 0);
        }
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    ui_init();
}

static void render_scene(int part, int view)
{
    ui.part = (uint8_t)part;
    set_view(view);
    build_pages();
    draw_header(); draw_main(); draw_knobs();
}

static uint32_t screen_hash(void)
{
    uint32_t h = 2166136261u;
    for (unsigned i = 0; i < sizeof fb / sizeof fb[0]; i++) {
        h = (h ^ (fb[i] & 255u)) * 16777619u;
        h = (h ^ (fb[i] >> 8)) * 16777619u;
    }
    return h;
}

#ifdef SCHEME_BASELINE
/* Run this exact fixture against the pre-P5 local sources to record original pixels. */
int main(void)
{
    puts("/* SPDX-License-Identifier: GPL-3.0-only */");
    puts("/* Original local P4 LCD hashes: five palettes, five parts, eight views. */");
    puts("static const uint32_t original_lcd[5][5][8] = {");
    for (int p = 0; p < 5; p++) {
        puts("    {");
        for (int part = 0; part < 5; part++) {
            printf("        {");
            for (int v = 0; v < 8; v++) {
                fixture(p);
                render_scene(part, appearance_views[v]);
                printf("0x%08Xu%s", screen_hash(), v < 7 ? ", " : "");
            }
            puts("},");
        }
        puts("    },");
    }
    puts("};");
    return 0;
}
#else
#include "scheme_default_hashes.h"

static void tap_button(int b)
{
    held_btn = 1u << b; run_ms(40);
    held_btn = 0; run_ms(40);
}

static void turn_encoder(int role, int n)
{
    enc_acc[role] += n;
    run_ms(80);
}

static void select_global_row(int kind, int action)
{
    for (int k = 0; k < ui.list_n; k++)
        if (list_rows[k].kind == kind && (kind != R_ACT || list_rows[k].a == action)) {
            while (ui.list_sel < k) turn_encoder(EN_SELECT, 1);
            while (ui.list_sel > k) turn_encoder(EN_SELECT, -1);
            return;
        }
    assert(!"GLOBAL row missing");
}

static void legacy_appearance(void)
{
    /* Normalize only the intentionally renamed P9 loop in historical pixel checks. */
    legacy_loop_names = 1;
    for (int p = 0; p < 5; p++)
        for (int part = 0; part < 5; part++)
            for (int v = 0; v < 8; v++) {
                fixture(p);
                render_scene(part, appearance_views[v]);
                if (appearance_views[v] == V_GEN && part < NKIT) {
                    /* P6.3 replaces the formerly unreachable drum TB-3PO
                     * fixture with Grids; retain all other P4 pixel baselines. */
                    assert(pg.n == 3 && pg.r[0][0].kind == R_DGEN);
                    assert(!strcmp(pg.title[0], "MAP"));
                    assert(!strcmp(pref_desc(pg.r[0][0])->name, "X"));
                } else if (appearance_views[v] == V_PART && (part == PART_303A || part == PART_303B)) {
                    /* EXTRA adds a third page dot and shifts the tag. Normalize only
                     * that navigation strip to keep every original TONE pixel checked. */
                    assert(pg.n == 3 && !strcmp(pg.title[2], "EXTRA"));
                    pg.n = 2;
                    draw_knobs();
                    assert(screen_hash() == original_lcd[p][part][v]);
                } else if (appearance_views[v] == V_PART && part == PART_BRK) {
                    /* BREAK FILTER adds a SOUND page. Normalize only the new
                     * navigation dots; every original LOOPS pixel stays checked. */
                    assert(pg.n == 2 && !strcmp(pg.title[1], "SOUND"));
                    assert(pg.r[1][0].a == T_BRK && pg.r[1][0].c == 2);
                    pg.n = 1;
                    draw_knobs();
                    assert(screen_hash() == original_lcd[p][part][v]);
                } else if (appearance_views[v] == V_SEQ && (part == PART_303A || part == PART_303B)) {
                    /* Verify all five v1.0.1 DIR pips, then normalize just their
                     * 38x9 rectangle to the original four-pip FWD fixture.
                     * Every remaining pixel still matches the historical hash. */
                    assert(pref_desc(pg.r[0][2])->max == 4 && pref_get(pg.r[0][2]) == DIR_FWD);
                    for (int pass = 0; pass < 2; pass++) {
                        int n = pass ? 4 : 5, left = 150 - (n * 8 - 2) / 2;
                        cv_begin(240, 9, C_BLACK);
                        for (int k = 0; k < n; k++)
                            box(left + k * 8, 0, 6, 9, k ? C_LINE : part_col());
                        for (int y = 0; y < 9; y++)
                            for (int x = 131; x < 169; x++) {
                                uint16_t c = cv_px[y * 240 + x];
                                if (!pass) assert(fb[(KNOB_Y + 33 + y) * 240 + x] == c);
                                else fb[(KNOB_Y + 33 + y) * 240 + x] = c;
                            }
                    }
                    assert(screen_hash() == original_lcd[p][part][v]);
                } else {
                    if (screen_hash() != original_lcd[p][part][v])
                        fprintf(stderr, "LCD mismatch palette=%d part=%d view=%d\n", p, part, appearance_views[v]);
                    assert(screen_hash() == original_lcd[p][part][v]);
                }
            }
    for (int p = 0; p < 5; p++) {
        fixture(p); test_gr = 12.0f; render_scene(PART_909, V_MIX);
        assert(screen_hash() == compressor_lcd[p]);
        char path[96]; snprintf(path, sizeof path, "build/host/schemes/default-%d-comp-active.png", p);
        png_write(path);
    }
    legacy_loop_names = 0;
    puts("DEFAULT: legacy pixels (303/BREAK page dots and DIR pips normalized), five DIR pips, Grids and active COMP/GR references passed");
}

static void global_memory(void)
{
    fixture(1);
    tap_button(B_GLO);
    assert(ui.list_sel == 1 && ui.list_top == 0);
    select_global_row(R_SCHEME, 0);
    const int exits[] = {B_GLO, B_HOME, B_EDIT, B_FX, B_LFO, B_ARP, B_SEQ};
    for (unsigned e = 0; e < sizeof exits / sizeof exits[0]; e++) {
        ui.part = PART_303A;
        int sel = ui.list_sel, top = ui.list_top;
        tap_button(exits[e]);
        assert(ui.view != V_GLO);
        tap_button(B_SEL); assert(ui.overlay == O_LIST);
        turn_encoder(EN_SELECT, 3);
        tap_button(B_HOME); tap_button(B_SEL);
        assert(ui.list_top == 0); /* other lists still reopen at the top */
        tap_button(B_GLO);
        assert(ui.list_sel == sel && ui.list_top == top);
        assert(list_rows[ui.list_sel].kind == R_SCHEME);
    }
    /* Every selectable row, including both scroll boundaries, survives reopen. */
    while (ui.list_sel > 1) turn_encoder(EN_SELECT, -1);
    for (;;) {
        int sel = ui.list_sel, top = ui.list_top;
        tap_button(B_GLO); tap_button(B_GLO);
        assert(ui.list_sel == sel && ui.list_top == top);
        assert(!list_skip(sel) && sel >= top && sel < top + 6);
        turn_encoder(EN_SELECT, 1);
        if (ui.list_sel == sel) break;
    }
    while (ui.list_sel > 1) turn_encoder(EN_SELECT, -1);
    assert(ui.list_sel == 1 && ui.list_top == 1);
    /* An action that changes views also preserves the GLOBAL cursor. */
    select_global_row(R_ACT, ACT_PERF);
    int sel = ui.list_sel, top = ui.list_top;
    tap_button(B_SEL); assert(ui.view == V_PERF);
    tap_button(B_GLO); assert(ui.list_sel == sel && ui.list_top == top);
    select_global_row(R_ACT, ACT_RESET);
    sel = ui.list_sel; top = ui.list_top;
    tap_button(B_SEL); assert(ui.overlay == O_ASK);
    tap_button(B_HOME); assert(ui.overlay == O_LIST);
    tap_button(B_GLO); tap_button(B_GLO);
    assert(ui.list_sel == sel && ui.list_top == top);
    /* Defensive restoration keeps even invalid remembered values visible. */
    tap_button(B_GLO); ui.global_sel = ui.global_top = 255;
    tap_button(B_GLO);
    assert(ui.list_sel < ui.list_n && !list_skip(ui.list_sel));
    assert(ui.list_sel >= ui.list_top && ui.list_sel < ui.list_top + 6);
    boot(); tap_button(B_GLO);
    assert(ui.list_sel == 1 && ui.list_top == 0);
    puts("GLOBAL memory: SCHEME across views/menus, every row, scrolling, actions, bounds and session reset passed");
}

static void compressor_rendering(void)
{
    for (int s = 0; s < NSCHEMES; s++) {
        fixture(1); pref_set(PR(R_SCHEME, 0, 0, 0), s);
        render_scene(PART_909, V_MIX);
        assert(C_GAIN == RGB(255, 60, 60));
        assert(C_WARNING == RGB(255, 80, 60));
        if (s == SCHEME_WHITE_LILAC) assert(PART_COL[PART_909] == RGB(187, 143, 180));
        const float levels[] = {0.0f, 0.5f, 0.6f, 12.0f, 30.0f};
        for (unsigned k = 0; k < sizeof levels / sizeof levels[0]; k++) {
            test_gr = levels[k];
            draw_main();
            uint16_t actual[240 * 240]; memcpy(actual, fb, sizeof fb);
            char label[24];
            int db = (int)(test_gr + 0.5f);
            if (db) put_s(put_i(put_s(label, "COMP -"), db), "dB");
            else put_s(label, "COMP 0dB");
            cv_begin(240, BAND_H, C_BLACK);
            cv_text(4, 20, &FONT_B, label, test_gr > 0.5f ? RGB(255, 60, 60) : C_GRAY);
            for (int y = 20; y < 36; y++)
                for (int x = 0; x < 120; x++)
                    assert(actual[(MAIN_Y + BAND_H + y) * 240 + x] == cv_px[y * 240 + x]);
            cv_begin(240, BAND_H, C_BLACK);
            rbox(206, 4, 8, 64, 4, C_LINE);
            int gr = (int)(test_gr * 64.0f / 24.0f); if (gr > 64) gr = 64;
            if (gr > 1) rbox(206, 4, 8, gr, 4, RGB(255, 60, 60));
            for (int y = 4; y < 68; y++)
                for (int x = 206; x < 214; x++)
                    assert(actual[(MAIN_Y + y) * 240 + x] == cv_px[y * 240 + x]);
            if (k == 0 || k == 3) {
                char path[96]; snprintf(path, sizeof path, "build/host/schemes/%d-comp-%s.png", s, k ? "active" : "inactive");
                png_write(path);
            }
        }
    }
    puts("COMP/GR: all six schemes, inactive/threshold/active/clamped meter pixels and 12 screenshots passed");
}

static void footer_refinement(void)
{
    const char *a, *b, *c;
    fixture(1);
    for (int s = 0; s < NSCHEMES; s++) {
        pref_set(PR(R_SCHEME, 0, 0, 0), s);
        render_scene(PART_909, V_SEQ);
        any_button = 1;
        uint16_t dim = C_DIM;
        assert(footer_text(&a, &b, &c) == (s ? dim + RGB(16, 16, 16) : dim));
        assert(C_DIM == (s ? SCHEMES[s].lcd[1] : PALETTES[1].c[1]));
        any_button = 0;
        assert(footer_text(&a, &b, &c) == C_GRAY);
        any_button = 1;
        set_view(V_GEN); ui.part = PART_303A;
        ui.gen_stale[0] = 0;
        assert(footer_text(&a, &b, &c) == C_AMB);
        ui.gen_stale[0] = 1;
        assert(footer_text(&a, &b, &c) == C_WHITE);
    }
    puts("Footer: five subtle lifts, unchanged DEFAULT/inactive colors/startup hint/TB-3PO emphasis passed");
}

static void factory_defaults(void)
{
    assert(SCHEME_DEFAULT == 0 && SCHEME_WHITE_LILAC == 1 && SCHEME_PINK_LILAC == 2);
    assert(SCHEME_SUNSET_ORANGE == 3 && SCHEME_GRAPHITE_MINT == 4 && SCHEME_WHITE_BLUE == 5 && NSCHEMES == 6);
    assert(SEQ_P[SQ_SCHEME].def == SCHEME_WHITE_LILAC);
    for (int i = 0; i < OBJ_NOBJ; i++) store_len[i] = -1;
    boot();
    assert(settings_scheme(&proj.set) == SCHEME_WHITE_LILAC);
    assert(scheme == &SCHEMES[SCHEME_WHITE_LILAC]);
    assert(project_save() == 0);
    boot();
    assert(settings_scheme(&proj.set) == SCHEME_WHITE_LILAC);
    puts("Factory: stable IDs, UI descriptor, empty-storage startup and WHITE LILAC save/reboot passed");
}

static void persistence(void)
{
    assert(sizeof(settings_t) == 36 && sizeof(project_t) == 30952);
    assert(__builtin_offsetof(settings_t, palette) == 8);
    assert(__builtin_offsetof(settings_t, rsv_bright) == 17);
    assert(__builtin_offsetof(settings_t, rsv) == 27);
    for (int byte = 0; byte < 256; byte++) {
        proj.set.rsv_bright = (uint8_t)byte;
        assert(settings_scheme(&proj.set) == (byte >= 0xD0 && byte <= 0xD5 ? byte - 0xD0 : 0));
    }
    /* Older full settings: legacy brightness 0..8 and each legacy palette. */
    for (int p = 0; p < 5; p++)
        for (int bright = 0; bright <= 8; bright++) {
            fixture(p);
            proj.set.rsv_bright = (uint8_t)bright;
            assert(project_save() == 0);
            boot();
            assert(settings_scheme(&proj.set) == SCHEME_DEFAULT && proj.set.palette == p);
            assert(!memcmp(pal, PALETTES[p].c, sizeof pal));
        }
    /* Historical 16-byte settings have neither autosave nor the scheme byte. */
    store_len[OBJ_SET] = 20;
    boot();
    assert(settings_scheme(&proj.set) == SCHEME_DEFAULT && proj.set.palette == 4);
    assert(!memcmp(pal, PALETTES[4].c, sizeof pal));
    for (int p = 0; p < 5; p++)
        for (int s = 0; s < NSCHEMES; s++) {
            fixture(p);
            pref_set(PR(R_SCHEME, 0, 0, 0), s);
            assert(project_save() == 0);
            pref_set(PR(R_SCHEME, 0, 0, 0), (s + 1) % NSCHEMES);
            boot();
            assert(settings_scheme(&proj.set) == s && proj.set.palette == p);
            assert(scheme == &SCHEMES[s]);
            assert(!memcmp(pal, s ? SCHEMES[s].lcd : PALETTES[p].c, sizeof pal));
        }
    /* Unknown tags/IDs cannot select an out-of-bounds table. */
    const int invalid[] = {0xD6, 0xD7, 0xFF};
    for (unsigned k = 0; k < sizeof invalid / sizeof invalid[0]; k++) {
        proj.set.rsv_bright = (uint8_t)invalid[k];
        assert(project_save() == 0);
        boot();
        assert(scheme == &SCHEMES[SCHEME_DEFAULT]);
    }
    puts("Storage: fixed layout, all 30 scheme/palette combinations, 45 older projects, short settings and invalid tags passed");
}

static void selection_and_screenshots(void)
{
    fixture(1);
    /* Actual GLOBAL input, including list navigation and one-detent selection. */
    tap_button(B_GLO);
    select_global_row(R_SCHEME, 0);
    int count = 0;
    for (int r = 0; r < ui.list_n; r++) count += list_rows[r].kind == R_SCHEME;
    assert(count == 1);
    assert(!strcmp(pref_desc(list_rows[ui.list_sel])->name, "SCHEME"));
    assert(pref_desc(list_rows[ui.list_sel])->max == 5);
    for (int s = 0; s < NSCHEMES; s++) {
        char num[16], unit[8], path[96];
        if (s) turn_encoder(EN_ALGO, 1);
        assert(pref_get(list_rows[ui.list_sel]) == s);
        pref_value(list_rows[ui.list_sel], num, unit);
        assert(!strcmp(num, SCHEME_N[s]) && !unit[0]);
        /* Names must fit beside SCHEME in both selected/unselected LCD fonts. */
        assert(8 + tw(&FONT_B, "SCHEME") + 6 <= 226 - tw(&FONT_B, num));
        assert(8 + tw(&FONT_S, "SCHEME") + 6 <= 226 - tw(&FONT_S, num));
        draw_header(); draw_main(); draw_knobs();
        snprintf(path, sizeof path, "build/host/schemes/%d-global.png", s);
        png_write(path);
        for (int part = 0; part < NPARTS; part++) {
            assert(PART_COL[part] != C_BLACK && PART_COL[part] != C_WARNING);
            for (int other = part + 1; other < NPARTS; other++) assert(PART_COL[part] != PART_COL[other]);
            ui.overlay = O_NONE;
            render_scene(part, V_SEQ);
            snprintf(path, sizeof path, "build/host/schemes/%d-%s.png", s, PART_N[part]);
            png_write(path);
        }
        render_scene(PART_909, V_HOME);
        snprintf(path, sizeof path, "build/host/schemes/%d-home.png", s);
        png_write(path);
        render_scene(PART_909, V_MIX);
        snprintf(path, sizeof path, "build/host/schemes/%d-mix.png", s);
        png_write(path);
        ui.rec = 1;
        draw_header();
        assert(fb[9 * 240 + 231] == swap16(C_RECORD_DOT));
        snprintf(path, sizeof path, "build/host/schemes/%d-record.png", s);
        png_write(path);
        ui.rec = 0;
        /* Incremental frame redraw must equal an explicitly invalidated redraw. */
        uint16_t incremental[240 * 240];
        ui_frame(); memcpy(incremental, fb, sizeof fb);
        memset(blit_hash, 0, sizeof blit_hash); ui_frame();
        assert(!memcmp(incremental, fb, sizeof fb));
        set_view(V_GLO); open_global(); select_global_row(R_SCHEME, 0);
    }
    turn_encoder(EN_ALGO, 1); assert(settings_scheme(&proj.set) == 5);
    for (int s = 4; s >= 0; s--) {
        turn_encoder(EN_ALGO, -1); assert(settings_scheme(&proj.set) == s);
    }
    turn_encoder(EN_ALGO, -1); assert(settings_scheme(&proj.set) == 0);
    /* Changes at unchanged knob values still recolor the cached knob strip. */
    ui.overlay = O_NONE; render_scene(PART_303A, V_SEQ);
    uint32_t before = screen_hash();
    pref_set(PR(R_SCHEME, 0, 0, 0), SCHEME_WHITE_BLUE); ui_frame();
    assert(screen_hash() != before);
    uint16_t changed[240 * 240]; memcpy(changed, fb, sizeof fb);
    memset(blit_hash, 0, sizeof blit_hash); ui_frame();
    assert(!memcmp(changed, fb, sizeof fb));
    /* SAVE and confirmed FACTORY RESET through the actual buttons. */
    tap_button(B_SAVE);
    boot(); assert(settings_scheme(&proj.set) == SCHEME_WHITE_BLUE);
    set_view(V_GLO); open_global(); select_global_row(R_ACT, ACT_RESET);
    tap_button(B_SEL); assert(ui.overlay == O_ASK);
    tap_button(B_SEL);
    assert(settings_scheme(&proj.set) == SCHEME_WHITE_LILAC && proj.set.palette == 1);
    assert(scheme == &SCHEMES[SCHEME_WHITE_LILAC]);
    assert(!memcmp(pal, SCHEMES[SCHEME_WHITE_LILAC].lcd, sizeof pal));
    tap_button(B_SAVE); boot();
    assert(settings_scheme(&proj.set) == SCHEME_WHITE_LILAC);
    puts("GLOBAL: exactly six choices, real selection/save/load/reset, label widths, redraws and 54 LCD screenshots passed");
}

int main(void)
{
    factory_defaults();
    legacy_appearance();
    global_memory();
    compressor_rendering();
    footer_refinement();
    persistence();
    selection_and_screenshots();
    return 0;
}
#endif
