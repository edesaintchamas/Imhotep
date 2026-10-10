/* Imhotep - drawing the tooltip (roadmap phase 7.5).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x004CABD0 draws the tooltip chosen by 0x004C8750 / 0x004C9750 next to the mouse: a box (frame
 * 0x004CD680, fill 0x004CD9D0) and its text through the multi-line text writer 0x004C8070 (package
 * blit), at 0x00D3A6D8, 0x00D3A6D4 (x, y), 0x00D3A6D0 x 0x00D3A6CC (size). By its kind 0x00D3A6C0:
 * 0 a help text (group 0x44, or the empire's goods, group 0x17), 1 the overlay text of the buffer
 * 0x00D3A4C0, 2 the city's ratings, 3 the open trade routes, 4 a figure's name. */
#include "sidebar.h"

#include "city/day_city.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

static void box(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t fill_w, int32_t fill_h)
{
    /* the frame and the fill (565: 0x58A0 / 0xBB88; 555: 0x2C40 / 0x5DC8) */
    int32_t edge = 0x2C40, fill = 0x5DC8;
    if (S32(st, G_PIXEL_FORMAT) == 2) {
        edge = 0x58A0;
        fill = 0xBB88;
    }
    GAME_CALL(st, FN_RECT_DRAW, x, y, w, h, edge);
    GAME_CALL(st, FN_RECT_FILL, S32(st, G_TIP_X) + 1, S32(st, G_TIP_Y) + 1, fill_w, fill_h, fill);
}

static void write_text(game_state *st, uint32_t text, int32_t dx, int32_t dy, int32_t width, int32_t measure)
{
    GAME_CALL(st, FN_RICH_TEXT_DRAW, (int32_t) text, S32(st, G_TIP_X) + dx, S32(st, G_TIP_Y) + dy, width, measure, 0);
}

void tooltip_draw_overlay(game_state *st)
{
    /* @bin 0x004CAF10 kind 1: the overlay text, 220 wide (300 past two lines), measured then
     * written; left of the mouse unless near the left edge, below unless low */
    S32(st, G_MESSAGE_WINDOW_FONT_TEXT) = S32(st, G_FONT_SMALL);
    S32(st, G_MESSAGE_WINDOW_FONT_LINK) = S32(st, G_FONT_SMALL);
    int32_t x = S32(st, G_TIP_X);
    S32(st, G_TIP_W) = 0xDC;
    S32(st, G_SCROLL) = 0;
    S32(st, G_MESSAGE_WINDOW_VISIBLE) = 0x1E;
    U16(st, G_GLYPH_COLOUR) = 0x4208;
    GAME_CALL(st, FN_RICH_TEXT_DRAW, G_TIP_TEXT, x + 5, S32(st, G_TIP_Y) + 5, 0xBE, 1, 0);
    S32(st, G_TIP_H) = S32(st, G_TEXT_LINES) * 16 + 10;
    if (S32(st, G_TEXT_LINES) > 2) {
        S32(st, G_TIP_W) = 300;
        S32(st, G_SCROLL) = 0;
        S32(st, G_MESSAGE_WINDOW_VISIBLE) = 0x1E;
        write_text(st, G_TIP_TEXT, 5, 5, 0x10E, 1);
        S32(st, G_TIP_H) = S32(st, G_TEXT_LINES) * 16 + 10;
    }
    int32_t w = S32(st, G_TIP_W);
    int32_t mx = S32(st, G_MOUSE_X);
    int32_t my = S32(st, G_MOUSE_Y);
    x = mx < w + 0x14 ? mx + 0x14 : mx - w - 0x14;
    S32(st, G_TIP_X) = x;
    int32_t y = my < 0xC8 ? my + 0x32 : my - 0x48;
    S32(st, G_TIP_Y) = y;
    int32_t h = S32(st, G_TIP_H);
    box(st, x, y, w + 0x14, h, S32(st, G_TIP_W) + 0x12, S32(st, G_TIP_H) - 2);
    write_text(st, G_TIP_TEXT, 5, 7, S32(st, G_TIP_W) - 0x1E, 0);
}

void tooltip_draw_ratings(game_state *st)
{
    /* @bin 0x004CB0B0 kind 2: the city's ratings (texts 0x44: 0x87..0x8B, 0x8E; values city
     * +0x2BFC, +0x42CC, +0x42D0, +0x42D4, +0x42D8 and the +0x08, the first and last with "%"),
     * each line twice for the shadow */
    S32(st, G_TIP_W) = 0xB4;
    int32_t mx = S32(st, G_MOUSE_X);
    int32_t my = S32(st, G_MOUSE_Y);
    S32(st, G_TIP_H) = 0x78;
    int32_t x = mx < 0xC8 ? mx + 0x14 : mx - 0xC8;
    S32(st, G_TIP_X) = x;
    int32_t y = my < 0xC8 ? my + 10 : my - 0x20;
    S32(st, G_TIP_Y) = y;
    int32_t c1 = 0x1C82, c2 = 0x7F9A, fill = 0x5DC8, edge = 0x2C40;
    if (S32(st, G_PIXEL_FORMAT) == 2) {
        c1 = 0x3922, c2 = 0xFF3A, fill = 0xBB88, edge = 0x58A0;
    }
    int32_t font = S32(st, G_FONT_SMALL);
    uint32_t city = CURRENT_CITY(st);
    const int32_t texts[6] = {0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8E};
    const int32_t values[6] = {S32(st, city + CITY_UNEMPLOYMENT), S32(st, city + CITY_RATING_CULTURE), S32(st, city + CITY_RATING_PROSPERITY),
                               S32(st, city + CITY_RATING_MONUMENT), S32(st, city + CITY_RATING_KINGDOM), S32(st, city + CITY_TAX_RATE)};
    const uint32_t suffixes[6] = {G_PERCENT_STRING, G_NUMBER_SUFFIX, G_NUMBER_SUFFIX, G_NUMBER_SUFFIX, G_NUMBER_SUFFIX, G_PERCENT_STRING};
    GAME_CALL(st, FN_RECT_DRAW, x, y, 0xB4, 0x78, edge);
    GAME_CALL(st, FN_RECT_FILL, S32(st, G_TIP_X) + 1, S32(st, G_TIP_Y) + 1, S32(st, G_TIP_W) - 2, S32(st, G_TIP_H) - 2, fill);
    for (int32_t k = 0, row = 0; row < 0x54; k++, row += 0x0E) {
        GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x44, texts[k], S32(st, G_TIP_X) + 5, row + S32(st, G_TIP_Y) + 5, font, c1);
        GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x44, texts[k], S32(st, G_TIP_X) + 4, row + S32(st, G_TIP_Y) + 4, font, c2);
        GAME_CALL(st, FN_NUMBER_DRAW, values[k], 0x40, (int32_t) suffixes[k], S32(st, G_TIP_X) + 0x8C,
                  row + S32(st, G_TIP_Y) + 5, font, c1);
        GAME_CALL(st, FN_NUMBER_DRAW, values[k], 0x40, (int32_t) suffixes[k], S32(st, G_TIP_X) + 0x8B,
                  row + S32(st, G_TIP_Y) + 4, font, c2);
    }
}

void tooltip_draw_routes(game_state *st)
{
    /* @bin 0x004CB300 kind 3: the kinds of open trade routes (0x00559FC0), names of group 0x17
     * from 0x27, separated by "," (0x005E0400), 60 pixels each */
    int32_t kinds[16] = {0};
    int32_t n = trade_open_routes_559fc0(st, kinds);
    int32_t my = S32(st, G_MOUSE_Y);
    S32(st, G_TIP_H) = 0x14;
    int32_t w = n * 60 + 0x32;
    int32_t mx = S32(st, G_MOUSE_X);
    S32(st, G_TIP_W) = w;
    int32_t x = mx < w + 0x14 ? mx + 0x14 : mx - w - 0x14;
    S32(st, G_TIP_X) = x;
    int32_t y = my < 0xC8 ? my + 10 : my - 0x20;
    uint16_t saved = U16(st, G_GLYPH_COLOUR);
    S32(st, G_TIP_Y) = y;
    int32_t edge = 0x2C40, fill = 0x5DC8;
    if (S32(st, G_PIXEL_FORMAT) == 2) {
        U16(st, G_GLYPH_COLOUR) = 0x3922;
        edge = 0x58A0, fill = 0xBB88;
    } else {
        U16(st, G_GLYPH_COLOUR) = 0x1C82;
    }
    GAME_CALL(st, FN_RECT_DRAW, x, y, w, 0x14, edge);
    GAME_CALL(st, FN_RECT_FILL, S32(st, G_TIP_X) + 1, S32(st, G_TIP_Y) + 1, S32(st, G_TIP_W) - 2, S32(st, G_TIP_H) - 2, fill);
    int32_t shown = 0;
    S32(st, G_TEXT_X) = 0;
    for (int32_t k = 0; k < 0x10; k++) {
        if (kinds[k] == 0) {
            continue;
        }
        if (shown != 0) {
            GAME_CALL(st, FN_TEXT_DRAW, G_STR_COMMA, S32(st, G_TEXT_X) + S32(st, G_TIP_X) + 5, S32(st, G_TIP_Y) + 5,
                      S32(st, G_FONT_SMALL), S16(st, G_GLYPH_COLOUR));
        }
        GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x17, k + 0x27, S32(st, G_TEXT_X) + S32(st, G_TIP_X) + 5, S32(st, G_TIP_Y) + 5,
                  S32(st, G_FONT_SMALL), S16(st, G_GLYPH_COLOUR));
        shown++;
    }
    U16(st, G_GLYPH_COLOUR) = saved;
}

void tooltip_draw_figure(game_state *st)
{
    /* @bin 0x004CA9A0 kind 4 (the editor): the name of the first figure of type 0x24 on the hovered
     * tile, by its +0x0B (group 0x30, or 0x31 text 3 for 1; texts 5, 6, 0x12, 0x13 for 2, 3,
     * 0x0C, 0x0D; b + 9 for 0x0E..0x19, b + 0x0D for 0x1A..0x1D, b + 0x1A for 0x1E..0x22, b - 0x28
     * below 0x3A, else b - 10) */
    int32_t f = S16(st, G_GRID_FIGURE + 2u * (uint32_t) S32(st, G_MOUSE_OFFSET));
    if (f == 0) {
        return;
    }
    while (U8(st, FIGURE_ADDR(f) + FIG_TYPE) != 0x24) {
        f = S16(st, FIGURE_ADDR(f) + FIG_NEXT_ON_TILE);
        if (f == 0) {
            return;
        }
    }
    uint8_t b = U8(st, FIGURE_ADDR(f) + FIG_RESOURCE);
    int32_t group = 0x30, id;
    if (b == 1) {
        group = 0x31, id = 3;
    } else if (b == 2) {
        id = 5;
    } else if (b == 3) {
        id = 6;
    } else if (b == 0x0C) {
        id = 0x12;
    } else if (b == 0x0D) {
        id = 0x13;
    } else if (b >= 0x0E && b <= 0x19) {
        id = b + 9;
    } else if (b >= 0x1A && b <= 0x1D) {
        id = b + 0x0D;
    } else if (b >= 0x1E && b <= 0x22) {
        id = b + 0x1A;
    } else if ((int32_t) b - 0x32 < 8) {
        id = b - 0x28;
    } else {
        id = b - 10;
    }
    GAME_CALL(st, FN_TEXT_SEEK, group, id);
    uint32_t text = U32(st, G_TEXT_CURSOR);
    int32_t w = GAME_CALL(st, FN_TEXT_WIDTH, (int32_t) text, S32(st, G_FONT_SMALL)) + 0x14;
    int32_t mx = S32(st, G_MOUSE_X);
    int32_t my = S32(st, G_MOUSE_Y);
    S32(st, G_TIP_W) = w;
    S32(st, G_TIP_H) = 0x14;
    int32_t x = mx < w + 0x14 ? mx + 0x14 : mx - w - 0x14;
    S32(st, G_TIP_X) = x;
    int32_t y = my < 0xC8 ? my + 10 : my - 0x20;
    S32(st, G_TIP_Y) = y;
    uint16_t saved = U16(st, G_GLYPH_COLOUR);
    int32_t edge = 0x2C40, fill = 0x5DC8;
    if (S32(st, G_PIXEL_FORMAT) == 2) {
        U16(st, G_GLYPH_COLOUR) = 0x3922;
        edge = 0x58A0, fill = 0xBB88;
    } else {
        U16(st, G_GLYPH_COLOUR) = 0x1C82;
    }
    GAME_CALL(st, FN_RECT_DRAW, x, y, w, 0x14, edge);
    GAME_CALL(st, FN_RECT_FILL, S32(st, G_TIP_X) + 1, S32(st, G_TIP_Y) + 1, S32(st, G_TIP_W) - 2, S32(st, G_TIP_H) - 2, fill);
    S32(st, G_TEXT_X) = 0;
    GAME_CALL(st, FN_TEXT_DRAW, (int32_t) text, S32(st, G_TIP_X) + 5, S32(st, G_TIP_Y) + 5, S32(st, G_FONT_SMALL),
              S16(st, G_GLYPH_COLOUR));
    U16(st, G_GLYPH_COLOUR) = saved;
}

void tooltip_draw(game_state *st)
{
    /* @bin 0x004CABD0 (see the file comment); kind 0: a help text 200 wide (300 past two lines)
     * written in 0x1C82 (565: 0x3922), placed by the mouse 0x00E38E2C, 0x00E38E28 and the window
     * (advisors 7, 0x2A, 0x2D their own places) */
    int32_t kind = S32(st, G_TIP_KIND);
    if (kind == 1) {
        tooltip_draw_overlay(st);
        return;
    }
    if (kind == 2) {
        tooltip_draw_ratings(st);
        return;
    }
    if (kind == 3) {
        tooltip_draw_routes(st);
        return;
    }
    if (kind == 4) {
        tooltip_draw_figure(st);
        return;
    }
    S32(st, G_MESSAGE_WINDOW_FONT_TEXT) = S32(st, G_FONT_SMALL);
    S32(st, G_MESSAGE_WINDOW_FONT_LINK) = S32(st, G_FONT_SMALL);
    S32(st, G_TIP_W) = 0xC8;
    S32(st, G_SCROLL) = 0;
    S32(st, G_MESSAGE_WINDOW_VISIBLE) = 0x1E;
    U16(st, G_GLYPH_COLOUR) = 0x1C82;
    if (S32(st, G_PIXEL_FORMAT) == 2) {
        U16(st, G_GLYPH_COLOUR) = 0x3922;
    }
    uint8_t window = U8(st, G_WINDOW);
    uint8_t flag = 0;
    int32_t object = 0, r = 0, group = 0x44, id = S32(st, G_TIP_ID);
    int asked = 0;
    if (window == 0x14) {
        object = S32(st, G_SELECTED) - 1;
        r = tooltip_empire_city_goods(st, &flag);
        asked = 1;
    } else if (window == 0x15) {
        object = S32(st, G_EDITOR_OBJECT);
        r = tooltip_trade_city_goods(st, &flag);
        asked = 1;
    } else if (window == 0x4E) {
        flag = 1;
        r = tooltip_family_hover(st);
        asked = 1;
    }
    if (asked && r != 0) {
        /* @bin 0x004CACA3 a good's name (group 0x17); 0x13 names the city's kind when sold
         * (table 0x005D2A3C by the city's +1 of the 0x62 records at 0x0077F760) */
        group = 0x17;
        if (r == 0x13) {
            id = flag == 0 ? S32(st, G_CITY_KIND_TEXTS + 8u * U8(st, G_EMPIRE_OBJECTS + EMPOBJ_U19 + 0x62u * (uint32_t) object)) : 0x26;
        } else {
            id = r;
        }
    } else {
        id = S32(st, G_TIP_ID);
    }
    GAME_CALL(st, FN_TEXT_SEEK, group, id);
    write_text(st, U32(st, G_TEXT_CURSOR), 5, 5, S32(st, G_TIP_W) - 0x14, 1);
    int32_t lines = S32(st, G_TEXT_LINES);
    int32_t h = lines * 16 + 10;
    S32(st, G_TIP_H) = h;
    if (lines > 2) {
        S32(st, G_TIP_W) = 300;
        S32(st, G_SCROLL) = 0;
        S32(st, G_MESSAGE_WINDOW_VISIBLE) = 0x1E;
        GAME_CALL(st, FN_TEXT_SEEK, 0x44, S32(st, G_TIP_ID));
        write_text(st, U32(st, G_TEXT_CURSOR), 5, 5, S32(st, G_TIP_W) - 0x14, 1);
        h = S32(st, G_TEXT_LINES) * 16 + 10;
        S32(st, G_TIP_H) = h;
    }
    int32_t w = S32(st, G_TIP_W);
    int32_t mx = S32(st, G_HIT_X);
    int32_t my = S32(st, G_HIT_Y);
    int near_left = mx < S32(st, G_LAYOUT_DX) + w + 0x64;
    int high = my < S32(st, G_LAYOUT_DY) + 0xC8;
    window = U8(st, G_WINDOW);
    int32_t x, y;
    if (near_left && window == 7) {
        x = mx + 0x32;
        S32(st, G_TIP_X) = x;
    } else {
        x = near_left ? mx + 0x14 : mx - w - 0x14;
        S32(st, G_TIP_X) = x;
    }
    if (window == 7) {
        /* @bin 0x004CADFD the advisors: above the mouse, by advisor, unless low on the screen */
        y = S32(st, G_LAYOUT_DY) + 0x1B0;
        if (my < y) {
            int32_t advisor = S32(st, G_PAGE);
            y = advisor == 1 ? my - 0x4A : advisor == 6 ? my - 0x36 : advisor == 7 ? my - 0x3A : my - 0x40;
        }
        S32(st, G_TIP_Y) = y;
    } else if (window == 0x2D) {
        y = my - 0x34;
        S32(st, G_TIP_Y) = y;
    } else if (window == 0x2A) {
        y = high ? my + 0x28 : my - 0x48;
        S32(st, G_TIP_Y) = y;
        x = mx - w / 2 - 10;
        S32(st, G_TIP_X) = x;
    } else {
        y = high ? my + 0x28 : my - 0x3E;
        S32(st, G_TIP_Y) = y;
    }
    box(st, x, y, w + 0x14, h, S32(st, G_TIP_W) + 0x12, S32(st, G_TIP_H) - 2);
    write_text(st, U32(st, G_TEXT_CURSOR), 5, 7, S32(st, G_TIP_W) - 0x14, 0);
}

/* ---- port table ---- */

static int32_t run_draw(game_state *st, const int32_t *a) { (void) a; tooltip_draw(st); return 0; }
static int32_t run_overlay(game_state *st, const int32_t *a) { (void) a; tooltip_draw_overlay(st); return 0; }
static int32_t run_ratings(game_state *st, const int32_t *a) { (void) a; tooltip_draw_ratings(st); return 0; }
static int32_t run_routes(game_state *st, const int32_t *a) { (void) a; tooltip_draw_routes(st); return 0; }
static int32_t run_figure(game_state *st, const int32_t *a) { (void) a; tooltip_draw_figure(st); return 0; }

const ported_function tooltip_draw_ports[] = {
    {0x004CABD0u, "tooltip_draw", 0, 0, 0, run_draw, 0},
    {0x004CAF10u, "tooltip_draw_overlay", 0, 0, 0, run_overlay, 0},
    {0x004CB0B0u, "tooltip_draw_ratings", 0, 0, 0, run_ratings, 0},
    {0x004CB300u, "tooltip_draw_routes", 0, 0, 0, run_routes, 0},
    {0x004CA9A0u, "tooltip_draw_figure", 0, 0, 0, run_figure, 0},
};
const int tooltip_draw_ports_count = sizeof(tooltip_draw_ports) / sizeof(tooltip_draw_ports[0]);
