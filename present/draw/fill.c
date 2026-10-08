/* Imhotep - drawing primitives: lines, rectangles, fills (package blit, 7.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The line 0x004CD510 steps with the error terms the original keeps in globals (0x00D3B4BC ..
 * 0x00D3B4DC); the rectangles and fills are made of its lines or of single pixels, clamped to the
 * screen (width 0x00E38D9C, height 0x00E38D98) as each function does. */
#include "draw.h"

#include "mem.h"

#define LINE_ERR 0x00D3B4BCu    /* i32 the error term */
#define LINE_END_X 0x00D3B4C0u  /* i32 */
#define LINE_END_Y 0x00D3B4C4u  /* i32 */
#define LINE_X 0x00D3B4C8u      /* i32 the start */
#define LINE_Y 0x00D3B4CCu      /* i32 */
#define LINE_DX 0x00D3B4D0u     /* i32 |x1 - x2|, counted down */
#define LINE_DY 0x00D3B4D4u     /* i32 |y1 - y2|, counted down */
#define LINE_STEP_X 0x00D3B4D8u /* i32 1 or -1 */
#define LINE_STEP_Y 0x00D3B4DCu /* i32 1 or -1 */
#define MARCH_COLOUR 0x00D3B4F0u   /* i32 the colour of the next dot of a marching rectangle */
#define MARCH_COUNT 0x00D3B4E8u    /* i32 dots drawn in that colour */
#define MARCH_START 0x00D3B4F4u    /* i32 the first dot's colour (cycled by 0x004CD810) */
#define MARCH_START_COUNT 0x00E36C84u
#define FORMAT_MASKS 0x00E3914Cu   /* i32 0x565 or 0x555 */

void line_setup(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x004CD3E0 the deltas, the start (the end with the smaller coordinate along the longer
     * axis) and the steps */
    S32(st, LINE_STEP_Y) = 1;
    S32(st, LINE_STEP_X) = 1;
    int32_t dx = x1 > x2 ? x1 - x2 : x2 - x1;
    S32(st, LINE_DX) = dx;
    int32_t dy = y1 > y2 ? y1 - y2 : y2 - y1;
    S32(st, LINE_DY) = dy;
    int swap = dy > dx ? y1 > y2 : x1 > x2;
    int32_t sx, sy, ex, ey;
    if (swap) {
        sy = y2;
        sx = x2;
        ey = y1;
        ex = x1;
    } else {
        sy = y1;
        ey = y2;
        sx = x1;
        ex = x2;
    }
    S32(st, LINE_Y) = sy;
    S32(st, LINE_END_Y) = ey;
    S32(st, LINE_X) = sx;
    S32(st, LINE_END_X) = ex;
    if (ex < sx) {
        S32(st, LINE_STEP_X) = -1;
    }
    if (ey < sy) {
        S32(st, LINE_STEP_Y) = -1;
    }
}

void line_error_step(game_state *st, int32_t along_y)
{
    /* @bin 0x004CD490 the error term after a step: + 2 (minor - major) when it is >= 0, else + 2
     * minor (minor: dx when stepping along y, else dy) */
    int32_t e = S32(st, LINE_ERR);
    int32_t minor = along_y ? S32(st, LINE_DX) : S32(st, LINE_DY);
    int32_t major = along_y ? S32(st, LINE_DY) : S32(st, LINE_DX);
    int32_t d = e < 0 ? minor : minor - major;
    S32(st, LINE_ERR) = (int32_t) ((uint32_t) e + 2u * (uint32_t) d);
}

void pixel_put_screen(game_state *st, int32_t x, int32_t y, int32_t colour)
{
    /* @bin 0x004CD4E0 the pixel when (x, y) is on the screen */
    if (x < 0 || x >= S32(st, SCREEN_W) || y < 0 || y >= S32(st, SCREEN_H)) {
        return;
    }
    pixel_put(st, x, y, colour);
}

void line_draw(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t colour)
{
    /* @bin 0x004CD510 vertical and horizontal lines directly, others by steps of one along the
     * longer axis; pixels off the screen are skipped */
    line_setup(st, x1, y1, x2, y2);
    int32_t y = S32(st, LINE_Y);
    int32_t x = S32(st, LINE_X);
    if (x1 == x2) {
        for (; y <= S32(st, LINE_END_Y); y++) {
            pixel_put_screen(st, x1, y, colour);
        }
        return;
    }
    if (y1 == y2) {
        for (; x <= S32(st, LINE_END_X); x++) {
            pixel_put_screen(st, x, y1, colour);
        }
        return;
    }
    int32_t dy = S32(st, LINE_DY);
    int32_t dx = S32(st, LINE_DX);
    if (dy > dx) {
        S32(st, LINE_ERR) = 2 * dx - dy;
        for (; y <= S32(st, LINE_END_Y); y++) {
            pixel_put_screen(st, x, y, colour);
            line_error_step(st, 1);
            S32(st, LINE_DY) -= 1;
            if (S32(st, LINE_ERR) >= 1) {
                S32(st, LINE_DX) -= 1;
                x += S32(st, LINE_STEP_X);
            }
        }
        return;
    }
    S32(st, LINE_ERR) = 2 * dy - dx;
    for (; x <= S32(st, LINE_END_X); x++) {
        pixel_put_screen(st, x, y, colour);
        line_error_step(st, 0);
        S32(st, LINE_DX) -= 1;
        if (S32(st, LINE_ERR) >= 1) {
            S32(st, LINE_DY) -= 1;
            y += S32(st, LINE_STEP_Y);
        }
    }
}

/* the clamp of the rectangle functions: x, y at least 0 (jg), the size cut at the screen's edge */
static void clamp(game_state *st, int32_t *x, int32_t *y, int32_t *w, int32_t *h, int bevel)
{
    if (*x <= 0) {
        *x = 0;
        if (!bevel && *x + *w >= S32(st, SCREEN_W)) {
            *w = S32(st, SCREEN_W) - *x;
        }
    } else if (*x + *w >= S32(st, SCREEN_W)) {
        *w = S32(st, SCREEN_W) - *x;
    }
    if (*y <= 0) {
        *y = 0;
        if (!bevel && *y + *h >= S32(st, SCREEN_H)) {
            *h = S32(st, SCREEN_H) - *y;
        }
    } else if (*y + *h >= S32(st, SCREEN_H)) {
        *h = S32(st, SCREEN_H) - *y;
    }
}

void rect_draw(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t colour)
{
    /* @bin 0x004CD680 the outline: top, bottom, left, right */
    clamp(st, &x, &y, &w, &h, 0);
    int32_t x2 = x + w - 1;
    int32_t y2 = y + h - 1;
    line_draw(st, x, y, x2, y, colour);
    line_draw(st, x, y2, x2, y2, colour);
    line_draw(st, x, y, x, y2, colour);
    line_draw(st, x2, y, x2, y2, colour);
}

void rect_draw_sunken(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x004CD890 grey (0x8410) top and left, white (0xFFFF) right and bottom; a coordinate
     * <= 0 is set to 0 without cutting the size */
    clamp(st, &x, &y, &w, &h, 1);
    int32_t x2 = x + w - 1;
    int32_t y2 = y + h - 1;
    line_draw(st, x, y, x2, y, 0x8410);
    line_draw(st, x2, y, x2, y2, 0xFFFF);
    line_draw(st, x, y2, x2, y2, 0xFFFF);
    line_draw(st, x, y, x, y2, 0x8410);
}

void rect_draw_raised(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x004CD930 white top and left, grey right and bottom */
    clamp(st, &x, &y, &w, &h, 1);
    int32_t x2 = x + w - 1;
    int32_t y2 = y + h - 1;
    line_draw(st, x, y, x2, y, 0xFFFF);
    line_draw(st, x2, y, x2, y2, 0x8410);
    line_draw(st, x, y2, x2, y2, 0x8410);
    line_draw(st, x, y, x, y2, 0xFFFF);
}

void rect_fill(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t colour)
{
    /* @bin 0x004CD9D0 a filled rectangle as horizontal lines (nothing when w or h <= 0) */
    if (w <= 0 || h <= 0) {
        return;
    }
    clamp(st, &x, &y, &w, &h, 0);
    int32_t x2 = x + w - 1;
    for (int32_t yy = y; yy < y + h; yy++) {
        line_draw(st, x, yy, x2, yy, colour);
    }
}

void rect_fill_and(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t mask)
{
    /* @bin 0x004CDA40 the rectangle's pixels ANDed with mask (0x0040FA70) */
    if (w <= 0 || h <= 0) {
        return;
    }
    clamp(st, &x, &y, &w, &h, 0);
    for (int32_t yy = y; yy < y + h; yy++) {
        for (int32_t xx = x; xx < x + w; xx++) {
            pixel_and(st, xx, yy, mask);
        }
    }
}

void rect_grey(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t shift)
{
    /* @bin 0x004CF090 each pixel to the grey (r + g + b) / 3 >> shift (imul 0x55555556: the
     * quotient rounded towards 0; sar), the channels read with 0x0040FAA0 (5 bits each: green's
     * top 5 bits in 565 [0x00E3914C] == 0x565) */
    for (int32_t yy = y; yy < y + h; yy++) {
        for (int32_t xx = x; xx < x + w; xx++) {
            int32_t r, g;
            if (S32(st, FORMAT_MASKS) == 0x565) {
                r = pixel_get_masked(st, xx, yy, 0xF800) >> 11;
                g = pixel_get_masked(st, xx, yy, 0x7E0) >> 6;
            } else {
                r = pixel_get_masked(st, xx, yy, 0x7C00) >> 10;
                g = pixel_get_masked(st, xx, yy, 0x3E0) >> 5;
            }
            int32_t b = pixel_get_masked(st, xx, yy, 0x1F);
            int32_t sum = b + g + r;
            int64_t q = (int64_t) sum * 0x55555556;
            int32_t hi = (int32_t) (q >> 32);
            int32_t v = (int32_t) ((uint32_t) hi + ((uint32_t) hi >> 31)) >> (shift & 31);
            pixel_and(st, xx, yy, 0);
            if (S32(st, FORMAT_MASKS) == 0x565) {
                pixel_or(st, xx, yy, (int32_t) ((uint32_t) v << 11));
                pixel_or(st, xx, yy, (int32_t) ((uint32_t) v << 6));
            } else {
                pixel_or(st, xx, yy, (int32_t) ((uint32_t) v << 10));
                pixel_or(st, xx, yy, (int32_t) ((uint32_t) v << 5));
            }
            pixel_or(st, xx, yy, v);
        }
    }
}

static int32_t march_next(int32_t c)
{
    /* 0xFFFF -> 0xF800 -> 0 -> 0xFFFF: neg / sbb / and 0xFFFFF801 / add 0xFFFF */
    if (c == 0xF800) {
        return 0;
    }
    return c != 0 ? 0xF800 : 0xFFFF;
}

void march_step_start(game_state *st)
{
    /* @bin 0x004CD810 every 20 calls the first colour of the marching rectangles moves on */
    int32_t n = S32(st, MARCH_START_COUNT) + 1;
    S32(st, MARCH_START_COUNT) = n;
    if (n < 0x14) {
        return;
    }
    S32(st, MARCH_START_COUNT) = 0;
    S32(st, MARCH_START) = march_next(S32(st, MARCH_START));
}

void march_step(game_state *st)
{
    /* @bin 0x004CD850 every 4 dots the colour moves on */
    int32_t n = S32(st, MARCH_COUNT) + 1;
    S32(st, MARCH_COUNT) = n;
    if (n < 4) {
        return;
    }
    S32(st, MARCH_COUNT) = 0;
    S32(st, MARCH_COLOUR) = march_next(S32(st, MARCH_COLOUR));
}

void rect_marching(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x004CD710 a dotted outline in colours cycling every 4 dots from [0x00D3B4F4]: top,
     * bottom, left, right edges; each dot drawn when inside the clip rectangle (0x004CDC30) and
     * the screen less 32 rows (0x0040FB10). The right edge's clip test uses x + 2 w - 1 (the
     * bottom edge's loop end + w - 1) while its dots are drawn at x + w - 1. */
    S32(st, MARCH_COLOUR) = S32(st, MARCH_START);
    S32(st, MARCH_COUNT) = 0;
    int32_t xe = x + w;
    for (int32_t xx = x; xx < xe; xx++) {
        if (clip_contains(st, xx, y)) {
            pixel_put_checked(st, xx, y);
        }
        march_step(st);
    }
    int32_t bx = x;
    int32_t yb = y + h - 1;
    for (; bx < xe; bx++) {
        if (clip_contains(st, bx, yb)) {
            pixel_put_checked(st, bx, yb);
        }
        march_step(st);
    }
    int32_t ye = y + h;
    for (int32_t yy = y; yy < ye; yy++) {
        if (clip_contains(st, x, yy)) {
            pixel_put_checked(st, x, yy);
        }
        march_step(st);
    }
    int32_t cx = w + bx - 1;
    for (int32_t yy = y; yy < ye; yy++) {
        if (clip_contains(st, cx, yy)) {
            pixel_put_checked(st, xe - 1, yy);
        }
        march_step(st);
    }
}
