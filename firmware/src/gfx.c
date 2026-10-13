/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Small-canvas renderer (no full framebuffer). Draw text/lines into
 * an off-screen strip, then blit it in one DMA transfer. Pixels are stored
 * byte-swapped (the panel takes RGB565 big-endian). */
typedef struct {               /* proportional, see tools/gen_font.py */
    uint8_t h;
    uint8_t pad;               /* bitmap starts this many pixels left of the pen */
    uint8_t first, last;
    const uint8_t *adv;        /* advance per glyph */
    const uint8_t *bw;         /* bitmap width per glyph (starts FONT_PAD left of the pen) */
    const uint16_t *off;       /* byte offset of each glyph */
    const uint8_t *data;
    const uint8_t *top;        /* first stored row in the original font cell */
    const uint8_t *rows;       /* stored rows; omitted rows are transparent */
} felucca_font_t;
#include "felucca_font.h"

#define CV_MAX (240u * 144u)      /* X0X: the main area, both bands, is 240 x 144 */
static uint16_t cv_px[CV_MAX] __attribute__((section(".pool")));
static uint32_t cv_w, cv_h;
static int32_t cv_oy;            /* y offset for graph drawing */
static int32_t cv_y0, cv_y1;     /* X0X: the rows drawing may touch (cv_band) */

#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#include "color_scheme.h"
/* Original LCD palettes remain available to DEFAULT for saved-project compatibility. */
typedef struct {
    const char *name;
    uint16_t c[5];
} palette_t;
/* X0X: the first three steps are neutral greys in every theme (structure: rules, empty cells,
 * labels); only the two brightest carry the theme's hue (values, highlights). Colour on the
 * screen then means something: a part, a value, an alarm. */
#define NEUTRALS RGB(34, 34, 40), RGB(74, 74, 82), RGB(150, 150, 158)
static const palette_t PALETTES[] = {
    {"GREEN", {NEUTRALS, RGB(80, 220, 120), RGB(150, 255, 175)}},
    {"AMBER", {NEUTRALS, RGB(232, 136, 30), RGB(255, 184, 70)}},
    {"CYAN", {NEUTRALS, RGB(60, 180, 235), RGB(150, 225, 255)}},
    {"RED", {NEUTRALS, RGB(232, 76, 60), RGB(255, 132, 112)}},
    {"MONO", {NEUTRALS, RGB(196, 196, 202), RGB(236, 236, 240)}},
};
#define NPALETTES (sizeof(PALETTES) / sizeof(PALETTES[0]))
typedef struct {
    uint16_t lcd[5];                 /* rules, inactive, labels, secondary, values */
    uint16_t part[5];                /* 909, 808, 303A, 303B, BREAK */
    uint16_t background, accent;
} color_scheme_t;
/* RGB565 in flash; each coordinated scheme occupies 24 bytes. Black backgrounds
 * keep the small LCD readable. Warm/cool and brightness differences separate parts. */
static const color_scheme_t SCHEMES[NSCHEMES] = {
    {{NEUTRALS, RGB(232, 136, 30), RGB(255, 184, 70)},
     {RGB(255, 150, 40), RGB(255, 72, 64), RGB(130, 240, 90), RGB(70, 205, 255), RGB(214, 120, 255)},
     RGB(0, 0, 0), RGB(255, 255, 255)},
    {{RGB(40, 36, 48), RGB(88, 80, 100), RGB(184, 176, 200), RGB(208, 184, 232), RGB(248, 232, 208)},
     {RGB(187, 143, 180), RGB(232, 152, 192), RGB(184, 160, 240), RGB(160, 184, 248), RGB(216, 128, 240)},
     RGB(0, 0, 0), RGB(248, 244, 232)},
    {{RGB(48, 32, 40), RGB(100, 76, 92), RGB(200, 168, 188), RGB(224, 160, 208), RGB(248, 208, 224)},
     {RGB(248, 176, 152), RGB(248, 112, 152), RGB(192, 160, 248), RGB(224, 144, 208), RGB(248, 208, 224)},
     RGB(0, 0, 0), RGB(248, 240, 240)},
    {{RGB(48, 36, 32), RGB(100, 80, 68), RGB(200, 176, 152), RGB(232, 152, 96), RGB(248, 212, 152)},
     {RGB(248, 192, 88), RGB(232, 120, 96), RGB(248, 216, 168), RGB(208, 152, 104), RGB(224, 144, 168)},
     RGB(0, 0, 0), RGB(248, 240, 216)},
    {{RGB(32, 44, 44), RGB(72, 96, 96), RGB(168, 192, 184), RGB(96, 200, 184), RGB(192, 240, 216)},
     {RGB(152, 240, 192), RGB(80, 192, 168), RGB(184, 216, 136), RGB(96, 208, 232), RGB(168, 184, 216)},
     RGB(0, 0, 0), RGB(232, 248, 240)},
    {{RGB(32, 40, 48), RGB(76, 88, 104), RGB(176, 188, 200), RGB(136, 176, 216), RGB(240, 232, 208)},
     {RGB(240, 224, 184), RGB(152, 176, 208), RGB(120, 192, 224), RGB(104, 144, 200), RGB(192, 200, 224)},
     RGB(0, 0, 0), RGB(248, 244, 232)},
};
static const char *const SCHEME_N[NSCHEMES] = {
    "DEFAULT", "WHITE LILAC", "PINK LILAC", "SUNSET ORANGE", "GRAPHITE MINT", "WHITE BLUE"
};
static const color_scheme_t *scheme = &SCHEMES[SCHEME_DEFAULT];
#define C_BLACK (scheme->background)
#define C_WHITE (scheme->accent)       /* touched / current position */
#define PART_COL (scheme->part)
/* Recording and warnings retain their original red meanings in every scheme.
 * Compressor text and meter share a vivid red, including in DEFAULT. */
#define C_RECORD_TEXT RGB(255, 90, 80)
#define C_RECORD_DOT RGB(255, 50, 50)
#define C_RECORD_PLAY RGB(255, 80, 70)
#define C_WARNING RGB(255, 80, 60)
#define C_GAIN RGB(255, 60, 60)
#define C_CPU_WARNING RGB(255, 60, 60)
#define C_CRASH_BG RGB(160, 0, 0)
static uint16_t pal[5];
#define C_LINE pal[0]                /* 1 rules, separators */
#define C_DIM pal[1]                 /* 2 inactive, empty steps, units */
/* Footer only: +16 per channel is safe for the five new schemes' dim colors. */
#define C_FOOT (scheme == &SCHEMES[SCHEME_DEFAULT] ? C_DIM : (uint16_t)(C_DIM + RGB(16, 16, 16)))
#define C_GRAY pal[2]                /* 3 labels */
#define C_AMB pal[3]                 /* 4 secondary text */
#define C_HI pal[4]                  /* 5 values, curves */

static void palette_set(uint32_t i)
{
    uint32_t k;
    for (k = 0; k < 5u; k++)
        pal[k] = PALETTES[i % NPALETTES].c[k];
}

static void scheme_set(uint32_t i, uint32_t legacy_palette)
{
    uint32_t k;
    if (i >= NSCHEMES)
        i = SCHEME_DEFAULT;
    scheme = &SCHEMES[i];
    if (i == SCHEME_DEFAULT)
        palette_set(legacy_palette);
    else
        for (k = 0; k < 5u; k++)
            pal[k] = scheme->lcd[k];
}

static inline uint16_t swap16(uint32_t c) { return (uint16_t)(((c >> 8) & 0xFFu) | ((c & 0xFFu) << 8)); }

static void cv_begin(uint32_t w, uint32_t h, uint16_t bg)
{
    uint32_t i, n;
    if (w * h > CV_MAX)
        h = CV_MAX / w;
    lcd_sync();                     /* the last blit may still read cv_px */
    cv_w = w;
    cv_h = h;
    cv_oy = cv_y0 = 0;
    cv_y1 = (int32_t)h;
    n = w * h;
    for (i = 0; i < n; i++)
        cv_px[i] = swap16(bg);
}

/* X0X: draw the next things as if the canvas were rows y0 .. y0+h-1 alone (one band of a
 * canvas that holds several, sent in one transfer so the bands change on screen together) */
static void cv_band(int32_t y0, int32_t h)
{
    cv_oy = cv_y0 = y0;
    cv_y1 = y0 + h;
}

static void cv_blit(uint32_t x, uint32_t y) { lcd_blit(x, y, cv_w, cv_h, cv_px); }

/* canvas rows r0 .. cv_h-1 only, to screen row y + r0 */
static void cv_blit_from(uint32_t x, uint32_t y, uint32_t r0)
{
    if (r0 < cv_h)
        lcd_blit(x, y + r0, cv_w, cv_h - r0, cv_px + r0 * cv_w);
}

static inline void cv_pset(int32_t x, int32_t y, uint16_t c)
{
    y += cv_oy;
    if ((uint32_t)x < cv_w && y >= cv_y0 && y < cv_y1)
        cv_px[(uint32_t)y * cv_w + (uint32_t)x] = swap16(c);
}

static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    int32_t x1 = x + w, y1 = y + h + cv_oy, i;
    uint16_t sc = swap16(c);
    y += cv_oy;                             /* clipped once, then filled row by row */
    if (x < 0)
        x = 0;
    if (y < cv_y0)
        y = cv_y0;
    if (x1 > (int32_t)cv_w)
        x1 = (int32_t)cv_w;
    if (y1 > cv_y1)
        y1 = cv_y1;
    for (; y < y1; y++)
        for (i = x; i < x1; i++)
            cv_px[(uint32_t)y * cv_w + (uint32_t)i] = sc;
}

static void cv_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int32_t dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy, guard = 2000;
    while (guard--) {                       /* bounded: a line is never longer than 480 px */
        int32_t e2 = 2 * err;               /* both tests use the same error value */
        cv_pset(x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* glyph index of a character: lower case folds to upper case when the font
 * has none, anything missing (and C1 controls) draws as '?' */
static uint32_t glyph(const felucca_font_t *f, uint32_t ch)
{
    if (ch >= 'a' && ch <= 'z' && f->last < 'a')
        ch -= 32u;
    if (ch < f->first || ch > f->last || (ch >= 127u && ch < 160u))
        ch = '?';
    return ch - f->first;
}

/* text in colour c, each pixel's coverage blended over what is already on the canvas (an
 * anti-aliased face on a coloured box gets no dark fringe); returns the end x */
static int32_t cv_text(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    uint32_t r = c >> 11, g = (c >> 5) & 63u, b = c & 31u;
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w, bpr;
        const uint8_t *gd;
        w = f->bw[gi];
        bpr = (w + 1u) / 2u;
        gd = f->data + f->off[gi];
        for (gy = 0; gy < f->rows[gi]; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t a = gd[gy * bpr + gx / 2u];
                int32_t px = x - f->pad + (int32_t)gx, py = y + f->top[gi] + (int32_t)gy + cv_oy;
                a = (gx & 1u) ? (a & 15u) : (a >> 4);
                if (!a || (uint32_t)px >= cv_w || py < cv_y0 || py >= cv_y1)
                    continue;
                if (a == 15u) {
                    cv_px[(uint32_t)py * cv_w + (uint32_t)px] = swap16(c);
                } else {
                    uint32_t bg = swap16(cv_px[(uint32_t)py * cv_w + (uint32_t)px]);
                    uint32_t br = bg >> 11, bgg = (bg >> 5) & 63u, bb = bg & 31u;
                    uint32_t nr = (r * a + br * (15u - a)) / 15u, ng = (g * a + bgg * (15u - a)) / 15u,
                             nb = (b * a + bb * (15u - a)) / 15u;
                    cv_px[(uint32_t)py * cv_w + (uint32_t)px] = swap16((nr << 11) | (ng << 5) | nb);
                }
            }
        x += f->adv[gi];
    }
    return x;
}

static int32_t text_w(const felucca_font_t *f, const char *s)
{
    int32_t w = 0;
    for (; *s; s++)
        w += f->adv[glyph(f, (uint8_t)*s)];
    return w;
}

/* one-shot: text in a box, cleared to black, blitted */
static void draw_text_box(uint32_t x, uint32_t y, uint32_t w, const felucca_font_t *f, const char *s,
                          uint16_t c, int align)
{
    int32_t tw = text_w(f, s), tx = 0;
    cv_begin(w, f->h, C_BLACK);
    if (align == 1)
        tx = ((int32_t)w - tw) / 2;
    else if (align == 2)
        tx = (int32_t)w - tw;
    cv_text(tx, 0, f, s, c);
    cv_blit(x, y);
    lcd_sync();                     /* one-shots (boot, crash, UBOOT, update) finish here */
}
