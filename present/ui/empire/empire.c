/* Imhotep - the empire map window (0x14) and its parts shared with 0x15 (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window 0x14: full redraw 0x0052D4D0 (frame + bottom panel), per frame (inline in 0x004D75D0) the
 * map 0x0052F8B0, 0x0052D4D0 again, the title 0x0052D940 and three buttons; input (inline in
 * 0x004D8310) scrolls with 0x00442730. Drawing goes through the ported primitives by address. */
#include "empire.h"

#include "city/events_military.h"
#include "draw/mem.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"

#define ARMY_SIZE 0x144u

static void image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, id, x, y);
}

static int32_t clamp_id(game_state *st, int32_t id)
{
    return id < S32(st, G_IMAGE_COUNT) ? id : 0;
}

static int32_t img_w(game_state *st, int32_t id)
{
    return S16(st, G_IMG_ENTRIES + 0x14u + ((uint32_t) clamp_id(st, id) << 6));
}

static int32_t img_h(game_state *st, int32_t id)
{
    return S16(st, G_IMG_ENTRIES + 0x16u + ((uint32_t) clamp_id(st, id) << 6));
}

static int32_t dx(game_state *st) { return S32(st, G_LAYOUT_DX); }
static int32_t dy(game_state *st) { return S32(st, G_LAYOUT_DY); }

static void group_centred(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t w,
                          int32_t font, int32_t colour)
{
    /* @bin 0x004D6250 */
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, group, index, x, y, w, font, colour, dx(st), dy(st));
}

static void group(game_state *st, int32_t g, int32_t index, int32_t x, int32_t y, int32_t font, int32_t colour)
{
    /* @bin 0x004D62F0 */
    GAME_CALL(st, FN_LAYOUT_GROUP, g, index, x, y, font, colour, dx(st), dy(st));
}

static void number_at(game_state *st, int32_t value, uint32_t suffix, int32_t x, int32_t y, int32_t font,
                      int32_t colour)
{
    /* @bin 0x004D6860 prefix character 0x40 */
    GAME_CALL(st, FN_LAYOUT_NUMBER_AT, value, 0x40, (int32_t) suffix, x, y, font, colour, dx(st), dy(st));
}

static uint32_t city_rec(game_state *st, int32_t c)
{
    (void) st;
    return G_EMPIRE_CITIES + 0x6Au * (uint32_t) c;
}

static int32_t name_group(game_state *st)
{
    return U8(st, G_EMPIRE_LANGUAGE) != 0 ? 0x15 : 0xC3;
}

/* ---- the frame ---- */

void empire_frame_draw(game_state *st)
{
    /* @bin 0x0052D630 the frame pieces of [0x00F5697C]: +3 the three bottom bands (70 wide), +1 the
     * top and two bottom borders (86 wide), +0 the sides (86 high), +2 the six corners */
    int32_t w = S32(st, G_SCREEN_W);
    int32_t n = w / 0x46;
    int32_t last = w - 0x46;
    int32_t x = 0;
    for (int32_t k = n; k > 0; k--) {
        image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 3, x, S32(st, G_SCREEN_H) - 0x78);
        image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 3, x, S32(st, G_SCREEN_H) - 0x50);
        image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 3, x, S32(st, G_SCREEN_H) - 0x28);
        x += 0x46;
    }
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 3, last, S32(st, G_SCREEN_H) - 0x78);
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 3, last, S32(st, G_SCREEN_H) - 0x50);
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 3, last, S32(st, G_SCREEN_H) - 0x28);
    w = S32(st, G_SCREEN_W);
    n = w / 0x56;
    last = w - 0x56;
    x = 0;
    for (int32_t k = n; k > 0; k--) {
        image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 1, x, 0);
        image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 1, x, S32(st, G_SCREEN_H) - 0x82);
        image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 1, x, S32(st, G_SCREEN_H) - 0x10);
        x += 0x56;
    }
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 1, last, 0);
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 1, last, S32(st, G_SCREEN_H) - 0x82);
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 1, last, S32(st, G_SCREEN_H) - 0x10);
    int32_t h = S32(st, G_SCREEN_H);
    n = h / 0x56;
    last = h - 0x56;
    int32_t y = 0x10;
    for (int32_t k = n; k > 0; k--) {
        image(st, S32(st, G_EMPIRE_FRAME_IMAGES), 0, y);
        image(st, S32(st, G_EMPIRE_FRAME_IMAGES), S32(st, G_SCREEN_W) - 0x10, y);
        y += 0x56;
    }
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES), 0, last);
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES), S32(st, G_SCREEN_W) - 0x10, last);
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 2, 0, 0);
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 2, 0, S32(st, G_SCREEN_H) - 0x82);
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 2, 0, S32(st, G_SCREEN_H) - 0x10);
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 2, S32(st, G_SCREEN_W) - 0x10, 0);
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 2, S32(st, G_SCREEN_W) - 0x10, S32(st, G_SCREEN_H) - 0x82);
    image(st, S32(st, G_EMPIRE_FRAME_IMAGES) + 2, S32(st, G_SCREEN_W) - 0x10, S32(st, G_SCREEN_H) - 0x10);
}

/* ---- the bottom panel ---- */

static int32_t city_trades_both(game_state *st, int32_t object, int32_t res)
{
    /* @bin 0x0052DAC0 the object sells and buys res and its city trades it */
    if (GAME_CALL(st, FN_EMPIRE_OBJECT_SELLS, object, res) != 0 && GAME_CALL(st, FN_EMPIRE_OBJECT_BUYS, object, res) != 0 &&
        (uint8_t) GAME_CALL(st, FN_EMPIRE_CITY_TRADES, S32(st, G_EMPIRE_SELECTED), res) != 0) {
        return 1;
    }
    return 0;
}

static int32_t route_of(game_state *st)
{
    return U8(st, city_rec(st, S32(st, G_EMPIRE_SELECTED)) + 4);
}

static int32_t scaled(int32_t res, int32_t v)
{
    /* @bin 0x0052E17C jump table 0x0052E5E8: amounts in hundreds but for resources 0x18-0x1A, 0x1E,
     * 0x23, 0x1C, 0x0A, 0x24 */
    switch (res) {
    case 0x18: case 0x19: case 0x1A: case 0x1E: case 0x23: case 0x1C: case 0x0A: case 0x24:
        return v;
    default:
        return v * 100;
    }
}

static void quality_mark(game_state *st, int32_t limit, int32_t x, int32_t y)
{
    /* @bin 0x0052DD41 the trade size mark of [0x00F56974]: +2 above 25, +1 above 15, +0 above 0 */
    if (limit > 0x19) {
        image(st, S32(st, G_TRADE_SIZE_IMAGES) + 2, x, y);
    } else if (limit > 0x0F) {
        image(st, S32(st, G_TRADE_SIZE_IMAGES) + 1, x, y);
    } else if (limit > 0) {
        image(st, S32(st, G_TRADE_SIZE_IMAGES), x, y);
    }
}

void empire_city_panel_draw(game_state *st)
{
    /* @bin 0x0052DB10 the selected city: by its kind (city +2: 0 ours, 2 / 4 others, 1 / 3 / 5
     * trade cities): the goods it sells and buys, and either the route's cost and "open" button
     * frame (route closed, city +5 = 0) or this year's trade against the limits (route open) */
    int32_t object = S32(st, G_SELECTED) - 1;
    uint32_t c = city_rec(st, S32(st, G_EMPIRE_SELECTED));
    uint8_t kind = U8(st, c + 2);
    int32_t cx = (S32(st, G_SCREEN_W) - 0xF0) / 2;
    if (kind == 0) {
        group_centred(st, 0x2F, 1, cx, S32(st, G_SCREEN_H) - 0x2E, 0xF0, S32(st, G_FONT_LARGE), 0);
        return;
    }
    if (kind == 2) {
        group_centred(st, 0x2F, 0x13, cx, S32(st, G_SCREEN_H) - 0x2E, 0xF0, S32(st, G_FONT_LARGE), 0);
        return;
    }
    if (kind == 4) {
        group_centred(st, 0x2F, 0x0D, cx, S32(st, G_SCREEN_H) - 0x2E, 0xF0, S32(st, G_FONT_LARGE), 0);
        return;
    }
    if (kind != 1 && kind != 3 && kind != 5) {
        group_centred(st, 0x2F, 0, cx, S32(st, G_SCREEN_H) - 0x2E, 0xF0, S32(st, G_FONT_LARGE), 0);
        return;
    }
    int32_t x0 = (S32(st, G_SCREEN_W) - 0x1F4) / 2;
    int32_t top = S32(st, G_SCREEN_H) - 0x6C;
    if (U8(st, c + 5) == 0) {
        /* the route is closed: what it would trade, its cost, the "open route" frame */
        S32(st, G_TEXT_X) = 0;
        group(st, 0x2F, 5, x0 + 0x32, top + 0x21, S32(st, G_FONT_LARGE), 0);
        S32(st, G_TEXT_X) = 0;
        for (int32_t res = 1; res < 0x24; res++) {
            if (GAME_CALL(st, FN_EMPIRE_OBJECT_SELLS, object, res) == 0 ||
                (uint8_t) GAME_CALL(st, FN_EMPIRE_CITY_TRADES, S32(st, G_EMPIRE_SELECTED), res) == 0) {
                continue;
            }
            int32_t lim = GAME_CALL(st, FN_TRADE_LIMIT_ADJUSTED,
                                    S32(st, G_TRADE_ROUTE_LIMITS + 4u * ((uint32_t) res + 0x24u * (uint32_t) route_of(st))));
            GAME_CALL(st, FN_RECT_DRAW_SUNKEN, S32(st, G_TEXT_X) + x0 + 0x64, top + 0x1C, 0x13, 0x11);
            image(st, S32(st, G_RESOURCE_ICONS) + res, S32(st, G_TEXT_X) + x0 + 0x65, top + 0x1D);
            quality_mark(st, lim, S32(st, G_TEXT_X) + x0 + 0x71, top + 0x1D);
            S32(st, G_TEXT_X) += 0x20;
        }
        group(st, 0x2F, 4, x0 + 0x32, top + 0x35, S32(st, G_FONT_LARGE), 0);
        S32(st, G_TEXT_X) = 0;
        for (int32_t res = 1; res < 0x24; res++) {
            if (GAME_CALL(st, FN_EMPIRE_OBJECT_BUYS, object, res) == 0 ||
                (uint8_t) GAME_CALL(st, FN_EMPIRE_CITY_TRADES, S32(st, G_EMPIRE_SELECTED), res) == 0) {
                continue;
            }
            int32_t lim = GAME_CALL(st, FN_TRADE_LIMIT_ADJUSTED,
                                    S32(st, G_TRADE_ROUTE_LIMITS + 4u * ((uint32_t) res + 0x24u * (uint32_t) route_of(st))));
            GAME_CALL(st, FN_RECT_DRAW_SUNKEN, S32(st, G_TEXT_X) + x0 + 0x64, top + 0x32, 0x13, 0x11);
            image(st, S32(st, G_RESOURCE_ICONS) + res, S32(st, G_TEXT_X) + x0 + 0x65, top + 0x32);
            quality_mark(st, lim, S32(st, G_TEXT_X) + x0 + 0x71, top + 0x32);
            S32(st, G_TEXT_X) += 0x20;
        }
        GAME_CALL(st, FN_LAYOUT_FRAME, 0, x0 + 0x32, top + 0x44, 400, 0x14, S32(st, G_HOVER_2) != 0, dx(st), dy(st));
        GAME_CALL(st, FN_TEXT_COUNT_DRAW, 8, 0, (int32_t) S16(st, c + 0x4E), x0 + 0x3C, top + 0x49, S32(st, G_FONT_LARGE), 0);
        uint8_t route = U8(st, c + 4);
        if (route >= 0x32) {
            return;
        }
        int32_t index = U8(st, G_ROUTES + 0x140u + ARMY_SIZE * route) == 1 ? 6 : 7;
        group(st, 0x2F, index, S32(st, G_TEXT_X) + x0 + 0x3C, top + 0x49, S32(st, G_FONT_LARGE), 0);
        return;
    }
    /* the route is open: sold and bought this year against the limits */
    group(st, 0x2F, 0x0B, x0 + 0x1E, top, S32(st, G_FONT_LARGE), 0);
    int32_t col = 0, count = 0, line_y = top + 0x0F, row = 0x0F;
    for (int32_t res = 1; res < 0x24; res++) {
        if (GAME_CALL(st, FN_EMPIRE_OBJECT_SELLS, object, res) == 0 ||
            (uint8_t) GAME_CALL(st, FN_EMPIRE_CITY_TRADES, S32(st, G_EMPIRE_SELECTED), res) == 0) {
            continue;
        }
        int32_t ox = col * 0x87;
        int32_t lim = GAME_CALL(st, FN_TRADE_LIMIT_ADJUSTED,
                                S32(st, G_TRADE_ROUTE_LIMITS + 4u * ((uint32_t) res + 0x24u * (uint32_t) route_of(st))));
        GAME_CALL(st, FN_RECT_DRAW_SUNKEN, ox + x0, row + top + 1, 0x13, 0x11);
        image(st, S32(st, G_RESOURCE_ICONS) + res, ox + x0 + 1, row + top + 2);
        quality_mark(st, lim, ox + x0 + 0x0D, line_y);
        int32_t done = S32(st, G_TRADE_DONE + 4u * ((uint32_t) res + 0x24u * (uint32_t) route_of(st)));
        if (done > lim) {
            lim = done;
        }
        done = scaled(res, done);
        lim = scaled(res, lim);
        if (city_trades_both(st, object, res) == 0) {
            S32(st, G_TEXT_X) = 0;
            number_at(st, done, G_GOAL_SEPARATOR, ox + 0x1E + x0 - 0x0C, line_y + 8, S32(st, G_FONT_SMALL), 0);
            group(st, 0x2F, 0x0C, S32(st, G_TEXT_X) + ox + 0x1E + x0 - 0x0E, line_y + 8, S32(st, G_FONT_SMALL), 0);
            number_at(st, lim, G_GOAL_SEPARATOR, S32(st, G_TEXT_X) + ox + 0x1E + x0 - 0x14, line_y + 8, S32(st, G_FONT_SMALL), 0);
        }
        col++;
        count++;
        if ((count & 1) == 0) {
            col = 0;
            row += 0x14;
            line_y += 0x14;
        }
    }
    line_y = top + 0x0F;
    GAME_CALL(st, FN_LINE_DRAW, x0 + 0xF0, line_y, x0 + 0xF0, top + 0x5A, 0);
    col = 0;
    count = 0;
    group(st, 0x2F, 0x0A, x0 + 0x17C, top, S32(st, G_FONT_LARGE), 0);
    row = 0x0F;
    for (int32_t res = 1; res < 0x24; res++) {
        if (GAME_CALL(st, FN_EMPIRE_OBJECT_BUYS, object, res) == 0 ||
            (uint8_t) GAME_CALL(st, FN_EMPIRE_CITY_TRADES, S32(st, G_EMPIRE_SELECTED), res) == 0) {
            continue;
        }
        int32_t ox = col * 0x87 + 0x118;
        int32_t lim = GAME_CALL(st, FN_TRADE_LIMIT_ADJUSTED,
                                S32(st, G_TRADE_ROUTE_LIMITS + 4u * ((uint32_t) res + 0x24u * (uint32_t) route_of(st))));
        GAME_CALL(st, FN_RECT_DRAW_SUNKEN, ox + x0, line_y, 0x13, 0x11);
        image(st, S32(st, G_RESOURCE_ICONS) + res, ox + x0 + 1, row + top + 1);
        quality_mark(st, lim, ox + x0 + 0x0D, line_y);
        int32_t done = S32(st, G_TRADE_DONE + 4u * ((uint32_t) res + 0x24u * (uint32_t) route_of(st)));
        if (done > lim) {
            lim = done;
        }
        done = scaled(res, done);
        lim = scaled(res, lim);
        S32(st, G_TEXT_X) = 0;
        number_at(st, done, G_GOAL_SEPARATOR, ox + 0x1E + x0 - 0x0C, line_y + 8, S32(st, G_FONT_SMALL), 0);
        group(st, 0x2F, 0x0C, S32(st, G_TEXT_X) + ox + 0x1E + x0 - 0x0E, line_y + 8, S32(st, G_FONT_SMALL), 0);
        number_at(st, lim, G_GOAL_SEPARATOR, S32(st, G_TEXT_X) + ox + 0x1E + x0 - 0x14, line_y + 8, S32(st, G_FONT_SMALL), 0);
        col++;
        count++;
        if ((count & 1) == 0) {
            col = 0;
            row += 0x14;
            line_y += 0x14;
        }
    }
}

void empire_army_panel_draw(game_state *st)
{
    /* @bin 0x0052E610 in window 0x14, an army marker under the mouse (0x005D2E88): its event's
     * status line (+0x30 = 5: 0x10, else 0x11; not active: 0x0F) */
    if (U8(st, G_WINDOW) != 0x14) {
        return;
    }
    int32_t x = (S32(st, G_SCREEN_W) - 0xF0) / 2;
    int32_t ev = S32(st, G_MARKERS + 20u * (uint32_t) S32(st, G_ARMY_SEL));
    uint32_t e = (ev >= 0 && ev < S32(st, G_EVENTS)) ? G_KINGDOM_EVENTS + 0x7Cu * (uint32_t) ev : 0;
    int32_t index;
    /* @bin 0x0052E662 an event out of range reads address 2 (as the original) */
    if (mem_rd8(st, e + 2) == 1) {
        index = (int16_t) mem_rd16(st, e + 0x30) == 5 ? 0x10 : 0x11;
    } else {
        index = 0x0F;
    }
    GAME_CALL(st, FN_LAYOUT_GROUP_LINES, 0x2F, index, x, S32(st, G_SCREEN_H) - 0x44, 0xF0, S32(st, G_FONT_LARGE), 0, dx(st), dy(st));
}

void empire_panel_draw(game_state *st)
{
    /* @bin 0x0052DA40 the bottom panel: the selected object (kind 1: a city's panel, 3: nothing
     * more), else the army marker, else the help line */
    int32_t sel = S32(st, G_SELECTED);
    if (sel <= 0) {
        if (S32(st, G_ARMY_SEL) >= 0) {
            empire_army_panel_draw(st);
            return;
        }
        group_centred(st, 0x2F, 9, S32(st, G_LAYOUT_DX), S32(st, G_SCREEN_H) - 0x4C, 0x280, S32(st, G_FONT_LARGE), 0);
        return;
    }
    uint32_t o = G_EMPIRE_OBJECTS + 0x62u * (uint32_t) (sel - 1);
    if (U8(st, o) == 1) {
        empire_city_panel_draw(st);
    }
    if (U8(st, o) == 3) {
        GAME_CALL0(st, FN_NOTHING);
    }
}

void empire_draw(game_state *st)
{
    /* @bin 0x0052D4D0 */
    empire_frame_draw(st);
    empire_panel_draw(st);
}

void empire_title_draw(game_state *st)
{
    /* @bin 0x0052D940 the selected city's name above the panel (not in the editor) */
    if (S32(st, G_EDITOR) == 1) {
        return;
    }
    int32_t sel = S32(st, G_SELECTED);
    if (sel <= 0) {
        return;
    }
    /* @bin 0x0052D967 object (sel - 1)'s kind (+0): 0x0077F6E6 + 0x62 * sel */
    if (U8(st, G_EMPIRE_OBJECTS - 0x62u + 0x62u * (uint32_t) sel) != 1) {
        return;
    }
    group_centred(st, name_group(st), U8(st, city_rec(st, S32(st, G_EMPIRE_SELECTED)) + 3), (S32(st, G_SCREEN_W) - 0x14C) / 2,
                  S32(st, G_SCREEN_H) - 0x76, 0x14C, S32(st, G_FONT_TITLE), 0);
}

void empire_scroll_key(game_state *st, int32_t direction)
{
    /* @bin 0x00442730 arrow direction 0..7 (8: none) moves the map 20 pixels, then the clamp
     * 0x00442800 */
    if (direction == 8) {
        return;
    }
    switch (direction) {
    case 0: S32(st, G_SCROLL_Y) -= 0x14; break;
    case 1: S32(st, G_SCROLL_Y) -= 0x14; S32(st, G_SCROLL_X) += 0x14; break;
    case 2: S32(st, G_SCROLL_X) += 0x14; break;
    case 3: S32(st, G_SCROLL_Y) += 0x14; S32(st, G_SCROLL_X) += 0x14; break;
    case 4: S32(st, G_SCROLL_Y) += 0x14; break;
    case 5: S32(st, G_SCROLL_Y) += 0x14; S32(st, G_SCROLL_X) -= 0x14; break;
    case 6: S32(st, G_SCROLL_X) -= 0x14; break;
    case 7: S32(st, G_SCROLL_Y) -= 0x14; S32(st, G_SCROLL_X) -= 0x14; break;
    default: break;
    }
    GAME_CALL0(st, FN_EMPIRE_SCROLL_CLAMP);
}

int32_t empire_city_of_object(game_state *st, int32_t object)
{
    /* @bin 0x00444920 the first city in use (+0) whose object (+0x56) is `object`; 0 for none */
    int32_t i = 0;
    for (uint32_t c = G_EMPIRE_CITIES + 0x56u; c < G_EMPIRE_CITIES + 0x1998u; c += 0x6A, i++) {
        if (U8(st, c - 0x56) != 0 && S16(st, c) == object) {
            return i;
        }
    }
    return 0;
}

int32_t image_anim_step(game_state *st, int32_t id, int32_t frame)
{
    /* @bin 0x0052F790 the next frame of image id's animation when its timer (0x00605B38 + entry
     * +0x3A) fired: frames 1..n (+0x1E) looping, or back and forth (entry +0x30) with bit 0x80 for
     * going back */
    int32_t f = frame;
    if (f <= 0) {
        f = 1;
    }
    uint32_t e = G_IMG_ENTRIES + ((uint32_t) clamp_id(st, id) << 6);
    if (U8(st, G_ANIM_FLAGS + U8(st, e + 0x3A)) == 0) {
        return f;
    }
    int32_t n = S16(st, e + 0x1E);
    if (U8(st, e + 0x30) == 0) {
        f++;
        return f <= n ? f : 1;
    }
    int back = (f & 0x80) != 0;
    f &= 0x7F;
    if (!back) {
        f++;
        if (f <= n) {
            return f;
        }
        return n | 0x80;
    }
    f--;
    if (f >= 1) {
        return f | 0x80;
    }
    return 1;
}

static int32_t army_progress(game_state *st, uint32_t e, int32_t *army, int32_t *pct, uint8_t *ours, int32_t *c)
{
    /* @bin 0x00449750 (thiscall on an event) an active army event (+2 = 1, +0x32, type 0x25): its
     * army (+0x1C), the part of the way done (status +0x30: 5 going, 6-7 coming back; inverted for
     * the city's own army), and whether the army is ours (army +0x140 = 1); 0 otherwise */
    *army = -1;
    *pct = 0;
    *ours = 0;
    *c = 0;
    if (U8(st, e + 2) != 1) {
        return 0;
    }
    if (U8(st, e + 0x32) == 0) {
        return 0;
    }
    if (S16(st, e + 4) != 0x25) {
        return 0;
    }
    uint8_t own = 0;
    army_of_city(st, S16(st, e + 0x1C), &own);
    *army = S16(st, e + 0x1C);
    int32_t months = event_army_months(st, e);
    if (months < 1) {
        months = 1;
    }
    int32_t status = S16(st, e + 0x30);
    int32_t done100 = (int32_t) ((uint32_t) (int32_t) S16(st, e + 0x2E) * 100u);
    if (status == 5) {
        *pct = 100 - done100 / months;
    } else if (status >= 6 && status <= 7) {
        *pct = done100 / months;
    } else {
        return 0;
    }
    if (*pct < 0) {
        *pct = 0;
    } else if (*pct > 100) {
        *pct = 100;
    }
    if (own != 0) {
        *pct = 100 - *pct;
    }
    *ours = U8(st, G_ARMIES + ARMY_SIZE * (uint32_t) *army) == 1;
    *c = 0;
    return 1;
}

static void route_point(game_state *st, uint32_t army, int32_t pct, int32_t extra, int32_t *x, int32_t *y)
{
    /* the point pct % along the army's route (+ extra points), clamped to the last */
    int32_t n = S32(st, army + 0x134);
    if (n < 1) {
        *x = 0;
        *y = 0;
        return;
    }
    if (pct < 0) {
        pct = 0;
    } else if (pct > 100) {
        pct = 100;
    }
    int32_t i = (int32_t) ((uint32_t) n * (uint32_t) pct) / 100 + extra;
    if (i >= n) {
        i = n - 1;
    }
    uint32_t p = U32(st, army + 4) + 6u * (uint32_t) i;
    *x = (int16_t) mem_rd16(st, p) - S32(st, G_SCROLL_X) + S32(st, G_MAP_X);
    *y = (int16_t) mem_rd16(st, p + 2) - S32(st, G_SCROLL_Y) + S32(st, G_MAP_Y);
}

static void marker_add(game_state *st, uint32_t e, int32_t img, int32_t x, int32_t y)
{
    /* @bin 0x005303C3 the marker's rectangle for the mouse (at most 100) */
    int32_t w = img_w(st, img), h = img_h(st, img);
    int32_t n = S32(st, G_MARKER_COUNT);
    if (n < 100) {
        uint32_t m = G_MARKERS + 20u * (uint32_t) n;
        S32(st, m) = S16(st, e);
        S32(st, m + 4) = x;
        S32(st, m + 8) = y;
        S32(st, m + 12) = w;
        S32(st, m + 16) = h;
        S32(st, G_MARKER_COUNT) = n + 1;
    }
}

static void route_dots(game_state *st, uint32_t army, int32_t image_id)
{
    /* the route's points as dots */
    for (int32_t i = 0; i < S32(st, army + 0x134); i++) {
        uint32_t p = U32(st, army + 4) + 6u * (uint32_t) i;
        image(st, image_id, (int16_t) mem_rd16(st, p) - S32(st, G_SCROLL_X) + S32(st, G_MAP_X),
              (int16_t) mem_rd16(st, p + 2) - S32(st, G_SCROLL_Y) + S32(st, G_MAP_Y));
    }
}

void empire_map_draw(game_state *st)
{
    /* @bin 0x0052F8B0 */
    S32(st, G_EMPIRE_ANIM) += 1;
    if (S32(st, G_EMPIRE_ANIM) > 100) {
        S32(st, G_EMPIRE_ANIM) = 0;
    }
    GAME_CALL0(st, FN_MARCH_STEP_START);
    int32_t mx = S32(st, G_MAP_X);
    int32_t my = S32(st, G_MAP_Y);
    int32_t view_w = S32(st, G_UE38D9C) - 2 * mx;
    S32(st, G_PIC_W) = view_w;
    int32_t view_h = S32(st, G_UE38D98) - S32(st, G_MAP_BOTTOM) - my;
    S32(st, G_PIC_H) = view_h;
    int32_t sx = S32(st, G_SCROLL_X);
    if (sx >= S32(st, G_MAP_W) - view_w) {
        sx = S32(st, G_MAP_W) - view_w - 1;
        S32(st, G_SCROLL_X) = sx;
    }
    int32_t sy = S32(st, G_SCROLL_Y);
    if (sy >= S32(st, G_MAP_H) - view_h) {
        sy = S32(st, G_MAP_H) - view_h - 1;
        S32(st, G_SCROLL_Y) = sy;
    }
    S32(st, G_PIC_X) = sx;
    S32(st, G_PIC_Y) = sy;
    image(st, S32(st, G_EMPIRE_MAP_IMAGE), mx, my);
    S32(st, G_CLIP_RIGHT) = S32(st, G_UE38D9C) - S32(st, G_MAP_X);
    S32(st, G_CLIP_LEFT) = S32(st, G_MAP_X);
    S32(st, G_CLIP_TOP) = S32(st, G_MAP_Y);
    S32(st, G_CLIP_BOTTOM) = S32(st, G_UE38D98) - S32(st, G_MAP_BOTTOM);
    int32_t name_colour, shadow;
    if (S32(st, G_PIXEL_FORMAT) == 2) {
        name_colour = 0x81C4;
        shadow = 0xAB0A;
    } else {
        name_colour = 0x40E4;
        shadow = 0x558A;
    }
    /* the objects */
    int32_t k = 0;
    for (uint32_t o = G_EMPIRE_OBJECTS; o < G_EMPIRE_OBJECTS_END; o += 0x62, k++) {
        if (U8(st, o + 1) == 0) {
            break;
        }
        uint8_t t = U8(st, o);
        if (t >= 4) {
            continue;
        }
        int32_t oy = S16(st, o + 6);
        int32_t ox = S16(st, o + 4);
        int32_t img = U16(st, o + 0x0C);
        if (t == 2) {
            group_centred(st, 0xC4, U8(st, o + 0x19), S32(st, G_MAP_X) - S32(st, G_SCROLL_X) + ox,
                          S32(st, G_MAP_Y) - S32(st, G_SCROLL_Y) + oy, 0x5A, S32(st, G_FONT_SMALL), 0);
            group_centred(st, 0xC4, U8(st, o + 0x19), S32(st, G_MAP_X) - S32(st, G_SCROLL_X) - 1 + ox,
                          S32(st, G_MAP_Y) - S32(st, G_SCROLL_Y) - 1 + oy, 0x5A, S32(st, G_FONT_SMALL), shadow);
            continue;
        }
        if (t == 3) {
            if (U8(st, G_WINDOW) == 0x14) {
                continue;
            }
            number_at(st, U8(st, o + 0x3C), G_NUMBER_SUFFIX, S32(st, G_MAP_X) - S32(st, G_SCROLL_X) + ox - 8,
                      S32(st, G_MAP_Y) - S32(st, G_SCROLL_Y) + oy - 10, S32(st, G_FONT_SMALL), 0);
            number_at(st, U8(st, o + 0x3C), G_NUMBER_SUFFIX, S32(st, G_MAP_X) - S32(st, G_SCROLL_X) + ox - 9,
                      S32(st, G_MAP_Y) - S32(st, G_SCROLL_Y) + oy - 9, S32(st, G_FONT_SMALL), 0xFFFF);
            number_at(st, U8(st, o + 0x3D), G_NUMBER_SUFFIX, S32(st, G_MAP_X) - S32(st, G_SCROLL_X) + ox + 0x10,
                      S32(st, G_MAP_Y) - S32(st, G_SCROLL_Y) + oy - 10, S32(st, G_FONT_SMALL), 0);
            number_at(st, U8(st, o + 0x3D), G_NUMBER_SUFFIX, S32(st, G_MAP_X) - S32(st, G_SCROLL_X) + ox + 0x0F,
                      S32(st, G_MAP_Y) - S32(st, G_SCROLL_Y) + oy - 9, S32(st, G_FONT_SMALL), 0xF800);
        }
        image(st, img, S32(st, G_MAP_X) - S32(st, G_SCROLL_X) + ox, S32(st, G_MAP_Y) - S32(st, G_SCROLL_Y) + oy);
        if (U8(st, o) != 1 || img == 0) {
            continue;
        }
        empire_city_of_object(st, k);
        int32_t tw = GAME_CALL(st, FN_TEXT_GROUP_WIDTH, name_group(st), U8(st, o + 0x19), S32(st, G_FONT_SMALL));
        int32_t lx, ly;
        switch (U8(st, o + 0x13)) {
        case 0:
            lx = S32(st, G_MAP_X) - S32(st, G_SCROLL_X) - tw + ox;
            ly = img_h(st, img) / 2 - S32(st, G_SCROLL_Y) + S32(st, G_MAP_Y) + oy;
            break;
        case 1:
            lx = (img_w(st, img) - tw) / 2 - S32(st, G_SCROLL_X) + S32(st, G_MAP_X) + ox;
            ly = S32(st, G_MAP_Y) - S32(st, G_SCROLL_Y) + oy - 10;
            break;
        case 2:
            lx = img_w(st, img) - S32(st, G_SCROLL_X) + S32(st, G_MAP_X) + ox;
            ly = img_h(st, img) / 2 - S32(st, G_SCROLL_Y) + S32(st, G_MAP_Y) + oy;
            break;
        default:
            lx = (img_w(st, img) - tw) / 2 - S32(st, G_SCROLL_X) + S32(st, G_MAP_X) + ox;
            ly = img_h(st, img) - S32(st, G_SCROLL_Y) + S32(st, G_MAP_Y) + oy + 5;
            break;
        }
        group(st, name_group(st), U8(st, o + 0x19), lx, ly, S32(st, G_FONT_SMALL), name_colour);
        uint8_t flag = U8(st, o + 0x18);
        if (flag == 1 || flag == 3 || flag == 5) {
            /* @bin 0x0052FDF2 the animated flag of [0x00F56B0C] on the city's top centre */
            int32_t ax = S32(st, G_MAP_X) - S32(st, G_SCROLL_X) + ox;
            int32_t ay = S32(st, G_MAP_Y) - S32(st, G_SCROLL_Y) + oy;
            int32_t w = img_w(st, img);
            int32_t h = img_h(st, img);
            int32_t base = S32(st, G_FLAG_IMAGES);
            int32_t fr = image_anim_step(st, base, U8(st, o + 2));
            U8(st, o + 2) = (uint8_t) fr;
            int32_t id = base + (int32_t) (uint8_t) fr;
            image(st, id, ax + w / 2, ay + h - img_h(st, id));
        }
    }
    if (U8(st, G_WINDOW) == 0x15) {
        /* the editor: the selected object framed, the routes (the chosen one with its points) */
        int32_t sel = S32(st, G_EDITOR_OBJECT);
        uint32_t o = G_EMPIRE_OBJECTS + 0x62u * (uint32_t) sel;
        int32_t bx, by;
        if (S32(st, G_U784CB4) == 0) {
            bx = S16(st, o + 4);
            by = S16(st, o + 6);
        } else {
            bx = S16(st, o + 0x14);
            by = S16(st, o + 0x16);
        }
        if (U8(st, o + 1) != 0) {
            int32_t img = U16(st, o + 0x0C);
            int32_t w, h;
            if (U8(st, o) == 2) {
                w = 0x5A;
                h = 0x14;
            } else {
                h = img_h(st, img) + 8;
                w = img_w(st, img) + 8;
            }
            GAME_CALL(st, FN_RECT_MARCHING, S32(st, G_MAP_X) - S32(st, G_SCROLL_X) + bx - 4,
                      S32(st, G_MAP_Y) - S32(st, G_SCROLL_Y) + by - 4, w, h);
            if (U8(st, G_WINDOW) != 0x15) {
                goto routes;
            }
        }
        k = 0;
        for (uint32_t q = G_ROUTES + 0x134u; q < G_ROUTES + 0x32u * ARMY_SIZE + 0x134u; q += ARMY_SIZE, k++) {
            uint32_t army = q - 0x134;
            if (U8(st, q + 0x0D) <= 1) {
                U8(st, q + 0x0E) = 0;
            }
            if (U8(st, q + 0x0E) == 0) {
                continue;
            }
            if (k == S32(st, G_ROUTE_SELECTED) && (S32(st, G_EDITOR_MODE) == 0x0E || S32(st, G_EDITOR_MODE) == 0x0F)) {
                if (S32(st, q) > 0 && U32(st, q - 0x130) != 0) {
                    route_dots(st, army, S32(st, G_ROUTE_IMAGES) + 1);
                    for (int32_t i = 0; i < (int32_t) U8(st, q + 0x0D); i++) {
                        uint32_t p = q - 0x12C + 6u * (uint32_t) i;
                        image(st, S32(st, G_ROUTE_IMAGES) + 2, S16(st, p) - S32(st, G_SCROLL_X) + S32(st, G_MAP_X),
                              S16(st, p + 2) - S32(st, G_SCROLL_Y) + S32(st, G_MAP_Y));
                    }
                }
                S32(st, G_ROUTE_ANIM) += 1;
                if (S32(st, G_ROUTE_ANIM) > 100) {
                    S32(st, G_ROUTE_ANIM) = 0;
                }
                int32_t x, y;
                route_point(st, army, S32(st, G_ROUTE_ANIM), 0, &x, &y);
                int32_t img = U8(st, q + 0x0C) == 2 ? S32(st, G_ARMY_IMAGE_OTHER) : S32(st, G_ARMY_IMAGE_OURS);
                image(st, img, x, y);
            } else if (S32(st, q) > 0 && U32(st, q - 0x130) != 0) {
                route_dots(st, army, S32(st, G_ROUTE_IMAGES) + 1);
            }
        }
    } else {
    routes:
        /* @bin 0x005301C9 the routes of the open cities as dots */
        k = 0;
        for (uint32_t q = G_ROUTES + 0x142u; q < G_ROUTES + 0x32u * ARMY_SIZE + 0x142u; q += ARMY_SIZE, k++) {
            uint32_t army = q - 0x142;
            if (U8(st, q - 1) <= 1) {
                U8(st, q) = 0;
            }
            if (U8(st, q) != 0 && GAME_CALL(st, FN_EMPIRE_CITY_OPEN, k) != 0 && S32(st, q - 0x0E) > 0 && U32(st, q - 0x13E) != 0) {
                route_dots(st, army, S32(st, G_ROUTE_IMAGES) + 1);
            }
        }
    }
    /* @bin 0x00530265 the armies on their way (events with an army), their markers recorded */
    S32(st, G_MARKER_COUNT) = 0;
    uint32_t e = (uint32_t) GAME_CALLT(st, G_EVENTS, FN_EVENTS_NEXT, 0, 0);
    while (e != 0) {
        if ((uint8_t) GAME_CALLT(st, e, FN_EVENT_PENDING) != 0) {
            int32_t left = GAME_CALLT(st, e, FN_EVENT_MONTHS_LEFT);
            int32_t a = S16(st, e + 0x58);
            if (a >= 0 && a < 0x32) {
                uint32_t army = G_ROUTES + ARMY_SIZE * (uint32_t) a;
                int32_t months = GAME_CALLT(st, army, FN_ARMY_TRAVEL_MONTHS);
                if (left <= months && months > 0) {
                    int32_t pct = 100 - (int32_t) ((uint32_t) left * 100u) / months;
                    if (pct > 100) {
                        pct = 0;
                    }
                    int32_t c = S32(st, G_EMPIRE_ANIM) - 0x32;
                    int32_t extra = (c + ((c >> 31) & 3)) >> 2;
                    if (extra < 0) {
                        extra = 0;
                    }
                    int32_t x, y;
                    route_point(st, army, pct, extra, &x, &y);
                    int32_t img = U8(st, army + 0x140) == 2 ? S32(st, G_ARMY_IMAGE_OTHER) : S32(st, G_ARMY_IMAGE_OURS);
                    image(st, img, x, y);
                    marker_add(st, e, img, x, y);
                }
            }
        }
        e = (uint32_t) GAME_CALLT(st, G_EVENTS, FN_EVENTS_NEXT, (int32_t) e, 0);
    }
    e = (uint32_t) GAME_CALLT(st, G_EVENTS, FN_EVENTS_NEXT_ACTIVE, 0);
    while (e != 0) {
        if (S16(st, e + 4) == 0x25) {
            int32_t a, pct, c;
            uint8_t ours;
            if (army_progress(st, e, &a, &pct, &ours, &c) && a >= 0 && a < 0x32) {
                uint32_t army = G_ROUTES + ARMY_SIZE * (uint32_t) a;
                int32_t x, y;
                route_point(st, army, pct, 0, &x, &y);
                int32_t img = ours ? S32(st, G_ARMY_IMAGE_OURS) : S32(st, G_ARMY_IMAGE_OTHER);
                image(st, img, x, y);
                marker_add(st, e, img, x, y);
            }
        }
        e = (uint32_t) GAME_CALLT(st, G_EVENTS, FN_EVENTS_NEXT_ACTIVE, (int32_t) e);
    }
    S32(st, G_CLIP_LEFT) = 0;
    S32(st, G_CLIP_RIGHT) = S32(st, G_SCREEN_W);
    S32(st, G_CLIP_TOP) = 0;
    S32(st, G_CLIP_BOTTOM) = S32(st, G_SCREEN_H);
}

/* ---- port table ---- */

static int32_t run_draw(game_state *st, const int32_t *a) { (void) a; empire_draw(st); return 0; }
static int32_t run_frame(game_state *st, const int32_t *a) { (void) a; empire_frame_draw(st); return 0; }
static int32_t run_panel(game_state *st, const int32_t *a) { (void) a; empire_panel_draw(st); return 0; }
static int32_t run_city(game_state *st, const int32_t *a) { (void) a; empire_city_panel_draw(st); return 0; }
static int32_t run_army(game_state *st, const int32_t *a) { (void) a; empire_army_panel_draw(st); return 0; }
static int32_t run_title(game_state *st, const int32_t *a) { (void) a; empire_title_draw(st); return 0; }
static int32_t run_map(game_state *st, const int32_t *a) { (void) a; empire_map_draw(st); return 0; }
static int32_t run_scroll(game_state *st, const int32_t *a) { empire_scroll_key(st, a[0]); return 0; }
static int32_t run_city_of(game_state *st, const int32_t *a) { return empire_city_of_object(st, a[0]); }
static int32_t run_anim(game_state *st, const int32_t *a) { return image_anim_step(st, a[0], a[1]); }
static int32_t run_both(game_state *st, const int32_t *a) { return city_trades_both(st, a[0], a[1]); }

const ported_function empire_ports[] = {
    {0x0052D4D0u, "empire_draw", 0, 0, 0, run_draw, 0},
    {0x0052D630u, "empire_frame_draw", 0, 0, 0, run_frame, 0},
    {0x0052DA40u, "empire_panel_draw", 0, 0, 0, run_panel, 0},
    {0x0052DB10u, "empire_city_panel_draw", 0, 0, 0, run_city, 0},
    {0x0052E610u, "empire_army_panel_draw", 0, 0, 0, run_army, 0},
    {0x0052D940u, "empire_title_draw", 0, 0, 0, run_title, 0},
    {0x0052F8B0u, "empire_map_draw", 0, 0, 0, run_map, 0},
    {0x00442730u, "empire_scroll_key", 1, 0, 0, run_scroll, 0},
    {0x00444920u, "empire_city_of_object", 1, 32, 0, run_city_of, 0},
    {0x0052F790u, "image_anim_step", 2, 32, 0, run_anim, 0},
    {0x0052DAC0u, "empire_city_trades_both", 2, 8, 0, run_both, 0},
    {0x00449750u, "army_progress (output in the caller's stack, in empire.c)", 4, 8, 0, NULL, 0},
};
const int empire_ports_count = sizeof(empire_ports) / sizeof(empire_ports[0]);
