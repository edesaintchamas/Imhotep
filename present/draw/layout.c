/* Imhotep - drawing primitives: texts and panels placed through the layout registry (package blit).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The windows draw most texts and panels through wrappers that look the element up in a registry
 * of layout overrides by (window, x - dx, y - dy) (lists 0x00E94618 / 0x00E9461C / 0x00E94620 per
 * window, 16 bytes apart; node +0 x, +4 y, +8 / +0xC the new place, +0x10 / +0x14 the new size,
 * +0x18 next). The registry is filled on first use (0x004D5D90, while [0x00E94D9C] is 0). When the
 * layout mode [0x00E94D98] matches the element's kind and the cursor (0x00E38E34, 0x00E38E30) lies
 * inside it, the element is outlined (0x004D6090; assumed: a layout editor of the developers). */
#include "draw.h"
#include "text.h"

#include "mem.h"

#include "present_layout.h"
#include "sim/call.h"

int32_t layout_window_key(game_state *st, int32_t window)
{
    /* @bin 0x004D5D70 window 7 (the advisors) is keyed by its page [0x00605A44] + 100 */
    return window == 7 ? S32(st, G_PAGE) + 100 : window;
}

uint32_t layout_find(game_state *st, int32_t kind, int32_t key, int32_t x, int32_t y)
{
    /* @bin 0x004D5FE0 the override node of (x, y) in list `kind` (1 texts, 2, 3 / 4 panels) of key */
    uint32_t n;
    if (kind == 1) {
        n = U32(st, G_LAYOUT_LISTS + ((uint32_t) key << 4));
    } else if (kind == 2) {
        n = U32(st, G_LAYOUT_LISTS + 4u + ((uint32_t) key << 4));
    } else if (kind == 3 || kind == 4) {
        n = U32(st, G_LAYOUT_LISTS + 8u + ((uint32_t) key << 4));
    } else {
        return 0;
    }
    while (n != 0) {
        if (x == (int32_t) mem_rd32(st, n) && y == (int32_t) mem_rd32(st, n + 4)) {
            return n;
        }
        n = mem_rd32(st, n + 0x18);
    }
    return 0;
}

void layout_highlight(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t w, int32_t h, int32_t nx,
                      int32_t ny, int32_t dx, int32_t dy)
{
    /* @bin 0x004D6090 in layout mode `kind` with the cursor inside the element (strictly): the
     * panel 0x004D6050, the element's place kept (0x00E945F8 .. 0x00E9460C) and two frames around it */
    int32_t px = nx + dx;
    int32_t py = ny + dy;
    if (kind != S32(st, G_LAYOUT_MODE) || px >= S32(st, G_MOUSE_X) || px + w <= S32(st, G_MOUSE_X) ||
        py >= S32(st, G_MOUSE_Y) || py + h <= S32(st, G_MOUSE_Y)) {
        return;
    }
    layout_mode_panel(st);
    S32(st, G_LAYOUT_PANEL_X) = nx;
    S32(st, G_LAYOUT_PANEL_OX) = x - dx;
    S32(st, G_LAYOUT_PANEL_Y) = ny;
    S32(st, G_LAYOUT_PANEL_W) = w;
    S32(st, G_LAYOUT_PANEL_H) = h;
    S32(st, G_LAYOUT_PANEL_OY) = y - dy;
    panel_draw_frame(st, 1, px, py, w, h, 0xFFFF);
    panel_draw_frame(st, 0, px, py, w, h, 0xFFFF);
}

void layout_mode_panel(game_state *st)
{
    /* @bin 0x004D6050 the window panel 0x00536820 (package sidebar) at (0, 0), 6 x 6 pieces */
    GAME_CALL(st, FN_PANEL_DRAW, 0, 0, 6, 6);
}

/* the start of every wrapper: the registry filled once, the element looked up by its place
 * relative to (dx, dy); *x, *y, *w, *h replaced by the node's (w, h only when given) */
static void layout_place(game_state *st, int32_t kind, int32_t *x, int32_t *y, int32_t *w, int32_t *h)
{
    if (S32(st, G_LAYOUT_READY) == 0) {
        GAME_CALL0(st, FN_PLAY_LAYOUT_FILL);
    }
    uint32_t n = layout_find(st, kind, layout_window_key(st, U8(st, G_WINDOW)), *x, *y);
    if (n != 0) {
        if (w) {
            *w = (int32_t) mem_rd32(st, n + 0x10);
        }
        if (h) {
            *h = (int32_t) mem_rd32(st, n + 0x14);
        }
        *x = (int32_t) mem_rd32(st, n + 8);
        *y = (int32_t) mem_rd32(st, n + 0xC);
    }
}

void layout_group_lines(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t width,
                        int32_t font, int32_t colour, int32_t dx, int32_t dy)
{
    /* @bin 0x004D6160 a string of Pharaoh_Text.eng wrapped in lines (0x004CCFD0) */
    int32_t nx = x - dx, ny = y - dy;
    layout_place(st, 1, &nx, &ny, &width, NULL);
    layout_highlight(st, 1, x, y, width, 0x18, nx, ny, dx, dy);
    text_group_draw_lines(st, group, index, nx + dx, ny + dy, width, font, colour);
}

void layout_group_centred(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t width,
                          int32_t font, int32_t colour, int32_t dx, int32_t dy)
{
    /* @bin 0x004D6250 centred (0x004CCB30) */
    int32_t nx = x - dx, ny = y - dy;
    layout_place(st, 1, &nx, &ny, &width, NULL);
    layout_highlight(st, 1, x, y, width, 0x18, nx, ny, dx, dy);
    text_group_draw_centred(st, group, index, nx + dx, ny + dy, width, font, colour);
}

void layout_group(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t font,
                  int32_t colour, int32_t dx, int32_t dy)
{
    /* @bin 0x004D62F0 (0x004CCBB0; outlined as 100 x 24) */
    int32_t nx = x - dx, ny = y - dy;
    layout_place(st, 1, &nx, &ny, NULL, NULL);
    layout_highlight(st, 1, x, y, 100, 0x18, nx, ny, dx, dy);
    text_group_draw(st, group, index, nx + dx, ny + dy, font, colour);
}

void layout_number_at(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, int32_t x, int32_t y,
                      int32_t font, int32_t colour, int32_t dx, int32_t dy)
{
    /* @bin 0x004D6860 a number (0x004CCE00; outlined as 24 x 24) */
    int32_t nx = x - dx, ny = y - dy;
    layout_place(st, 1, &nx, &ny, NULL, NULL);
    layout_highlight(st, 1, x, y, 0x18, 0x18, nx, ny, dx, dy);
    number_draw(st, value, prefix, suffix, nx + dx, ny + dy, font, colour);
}

void layout_frame(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t w, int32_t h, int32_t lit, int32_t dx,
                  int32_t dy)
{
    /* @bin 0x004D63D0 a frame 0x00536C30 (list 3) */
    int32_t nx = x - dx, ny = y - dy;
    layout_place(st, 3, &nx, &ny, &w, &h);
    layout_highlight(st, 3, x, y, w, h, nx, ny, dx, dy);
    panel_draw_frame(st, kind, nx + dx, ny + dy, w, h, lit);
}

void layout_inner_panel(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t dx, int32_t dy)
{
    /* @bin 0x004D65B0 an inner panel 0x00536A30 of w x h pieces (list 4; outlined in mode 4 as
     * 16 w x 16 h pixels, with x - dx, y - dy as given kept in 0x00E945F8 / 0x00E945FC) */
    int32_t ox = x - dx, oy = y - dy;
    int32_t nx = ox, ny = oy;
    layout_place(st, 4, &nx, &ny, &w, &h);
    int32_t px = nx + dx, py = ny + dy;
    if (S32(st, G_LAYOUT_MODE) == 4 && px < S32(st, G_MOUSE_X) && px + 16 * w > S32(st, G_MOUSE_X) &&
        py < S32(st, G_MOUSE_Y) && py + 16 * h > S32(st, G_MOUSE_Y)) {
        layout_mode_panel(st);
        S32(st, G_LAYOUT_PANEL_X) = nx;
        S32(st, G_LAYOUT_PANEL_Y) = ny;
        S32(st, G_LAYOUT_PANEL_W) = w;
        S32(st, G_LAYOUT_PANEL_H) = h;
        S32(st, G_LAYOUT_PANEL_OX) = ox;
        S32(st, G_LAYOUT_PANEL_OY) = oy;
        panel_draw_frame(st, 1, px, py, 16 * w, 16 * h, 0xFFFF);
        panel_draw_frame(st, 0, px, py, 16 * w, 16 * h, 0xFFFF);
    }
    panel_draw_inner(st, px, py, w, h);
}
