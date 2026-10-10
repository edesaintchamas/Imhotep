/* Imhotep - per-frame parts of the advisor pages, the hall dialogs and other player windows
 * (0x004D75D0's callees; roadmap 8.2, package "p8ui").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "ui.h"

#include "city/monument_rating.h"
#include "command/military.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"

#define AX() S32(st, G_ADVISOR_X)
#define AY() S32(st, G_ADVISOR_Y)

static void frame_rect(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, int32_t lit)
{
    /* 0x004D63D0(0, x, y, w, h, lit, dx, dy): a rectangle's frame, lit or not */
    GAME_CALL(st, FN_LAYOUT_FRAME, 0, x, y, w, h, lit, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

static void advisors_min_lines(game_state *st)
{
    /* @bin 0x00521060 every advisor page per frame: the panel's lines 0x00F53F48 at least 6 (a
     * redraw when raised) */
    if (S32(st, G_ADVISOR_SLIDE) <= 5) {
        S32(st, G_ADVISOR_SLIDE) = 6;
        U8(st, G_REDRAW) = 1;
    }
}

static void advisor5_frame(game_state *st)
{
    /* @bin 0x004F7770 advisor page 5: with at least 5 lines (0x00F53F48; 5 - n < 5 and <= 0), its four
     * buttons' frames (120 x 66 at x 0x50 + 120 k, y 0x11E), the hovered one (k + 1) lit */
    int32_t d = 5 - S32(st, G_ADVISOR_SLIDE);
    if (d >= 5 || d > 0) {
        return;
    }
    for (int32_t k = 1, x = 0x78; x <= 0x1E0; x += 0x78, k++) {
        frame_rect(st, x + AX() - 0x28, AY() + 0x11E, 0x78, 0x42, S32(st, G_HOVER) == k);
    }
}

static int32_t storage_rows(game_state *st)
{
    /* @bin 0x005027B0 the info panel's list rows (window 9 per frame): a storage yard or granary
     * (types 0x48, 0xC1) min(0x00502780, 0x00F53E6C); a market (0x46, 0xBE) the goods of
     * 0x00F53E70[0x00F53E6C] that 0x005013A0 keeps; else 0x00F53DD8 */
    int16_t t = S16(st, BUILDING_BASE + BLD_TYPE + U32(st, G_INFO_BUILDING) * 0x108u);
    if (t == 0x48 || t == 0xC1) {
        int32_t n = GAME_CALL0(st, FN_STORAGE_ROWS_VISIBLE);
        int32_t max = S32(st, G_STORAGE_LIST);
        if (n < max) {
            return GAME_CALL0(st, FN_STORAGE_ROWS_VISIBLE);
        }
        return max;
    }
    if (t != 0x46 && t != 0xBE) {
        return S32(st, G_GOODS_COUNT);
    }
    int32_t n = 0;
    for (int32_t i = 0; i < S32(st, G_STORAGE_LIST); i++) {
        if (GAME_CALL(st, FN_STORAGE_KIND, S32(st, G_GOODS_AVAILABLE + (uint32_t) i * 4u)) != 0) {
            n++;
        }
    }
    return n;
}

static void dialog2a_frame(game_state *st)
{
    /* @bin 0x005219C0 window 0x2A per frame: nine squares (27 x 27 at x 0xB2 + 32 k, y 0xDD), red
     * (0xF800) the hovered one (k = hover - 2), else black; then the bar (280 x 25 at 0xB4, 0x100)
     * red when the hover is 1 */
    int32_t k = 0;
    for (int32_t x = 0; x < 0x120; x += 0x20, k++) {
        int32_t colour = k == S32(st, G_HOVER) - 2 ? 0xF800 : 0;
        GAME_CALL(st, FN_RECT_DRAW, x + AX() + 0xB2, AY() + 0xDD, 0x1B, 0x1B, colour);
    }
    for (int32_t n = 0x0A - k; k < 0x0A && n != 0; n--) {
        GAME_CALL(st, FN_RECT_DRAW, AX() + 0xB4, AY() + 0x100, 0x118, 0x19, S32(st, G_HOVER) == 1 ? 0xF800 : 0);
    }
}

static void advisor7_frame(game_state *st)
{
    /* @bin 0x005271F0 advisor page 7: when the hover changed (0x005F825C), the two buttons' frames
     * (106 x 57 at 0x1F5, y 0x3C and 0xA0), the hovered one (1 or 2) lit; other hovers: nothing */
    int32_t h = S32(st, G_HOVER);
    if (h == S32(st, G_GRAPH_HOVER) || (h != 0 && h != 1 && h != 2)) {
        return;
    }
    S32(st, G_GRAPH_HOVER) = h;
    frame_rect(st, AX() + 0x1F5, AY() + 0x3C, 0x6A, 0x39, h == 1);
    frame_rect(st, AX() + 0x1F5, AY() + 0xA0, 0x6A, 0x39, h == 2);
}

static void text_at(game_state *st, int32_t group, int32_t idx, int32_t x, int32_t y, int32_t font)
{
    /* 0x004D62F0(group, idx, x, y, font, 0, dx, dy): a text of the layout */
    GAME_CALL(st, FN_LAYOUT_GROUP, group, idx, x, y, font, 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

static void dialog2d_frame(game_state *st)
{
    /* @bin 0x005253C0 window 0x2D per frame: five 64 x 20 frames at x 0x80 + 0x50 k, y 0xD8 (lit by
     * the hover 3..7), then 160 x 20 at 0x150 (hover 1) and 0x90 (hover 2), y 0x11B */
    for (int32_t k = 0; k < 5; k++) {
        frame_rect(st, AX() + 0x80 + 0x50 * k, AY() + 0xD8, 0x40, 0x14, S32(st, G_HOVER) == k + 3);
    }
    frame_rect(st, AX() + 0x150, AY() + 0x11B, 0xA0, 0x14, S32(st, G_HOVER) == 1);
    frame_rect(st, AX() + 0x90, AY() + 0x11B, 0xA0, 0x14, S32(st, G_HOVER) == 2);
}

static void dialog4c_frame(game_state *st)
{
    /* @bin 0x00537A50 window 0x4C per frame (a request's dialog): the request's amount (player
     * +0x41F4; times 100 unless its kind +0x41EC is 0x18..0x1A, 0x1E, 0x23 (table 0x00537CD8), 0x1C,
     * 0x0A or 0x24) after text 199:4 at (0x80, 0xD5) with 0x004D6860 (the string 0x007027D4 after
     * the text's end 0x00E20778); texts 199:5 (lit, font 0x00F56FA4, at hover 1), 199:8 / 199:9
     * (lit at the hover 0x007056B0 = 10 / 11) at x 0x152; 199:6 centred over 240 at y 0x155; the
     * frames 240 x 20 at (0x80, 0x150) hover 2 and 160 x 20 at (400, 0x150) hover 3 */
    uint32_t p = U32(st, G_PLAYER) * 0x49D8u;
    int32_t kind = S32(st, CITY_BASE + CITY_GRAVE_GOODS_RESOURCE + p);
    int32_t amount = S32(st, CITY_BASE + CITY_GRAVE_GOODS_AMOUNT + p);
    int plain = kind == 0x18 || kind == 0x19 || kind == 0x1A || kind == 0x1E || kind == 0x23 || kind == 0x1C ||
                kind == 0x0A || kind == 0x24;
    if (!plain) {
        amount *= 100;
    }
    S32(st, G_TEXT_X) = 0;
    text_at(st, 0xC7, 4, AX() + 0x80, AY() + 0xD5, S32(st, G_FONT_9C));
    GAME_CALL(st, FN_LAYOUT_NUMBER_AT, amount, 0x40, G_GOAL_SEPARATOR, AX() + S32(st, G_TEXT_X) + 0x80, AY() + 0xD5,
              S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    text_at(st, 0xC7, 5, AX() + 0x152, AY() + 0xD5, S32(st, S32(st, G_HOVER) == 1 ? G_FONT_RED : G_FONT_9C));
    text_at(st, 0xC7, 8, AX() + 0x152, AY() + 0xEE, S32(st, S32(st, G_SQUARE_BUTTON_HOVER) == 0x0A ? G_FONT_RED : G_FONT_9C));
    text_at(st, 0xC7, 9, AX() + 0x152, AY() + 0x107, S32(st, S32(st, G_SQUARE_BUTTON_HOVER) == 0x0B ? G_FONT_RED : G_FONT_9C));
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0xC7, 6, AX() + 0x80, AY() + 0x155, 0xF0, S32(st, G_FONT_LARGE), 0, S32(st, G_LAYOUT_DX),
              S32(st, G_LAYOUT_DY));
    GAME_CALL(st, FN_LAYOUT_FRAME, 1, AX() + 0x80, AY() + 0x150, 0xF0, 0x14, S32(st, G_HOVER) == 2, S32(st, G_LAYOUT_DX),
              S32(st, G_LAYOUT_DY));
    GAME_CALL(st, FN_LAYOUT_FRAME, 1, AX() + 400, AY() + 0x150, 0xA0, 0x14, S32(st, G_HOVER) == 3, S32(st, G_LAYOUT_DX),
              S32(st, G_LAYOUT_DY));
}

static void military_tail(game_state *st, int any, uint32_t table1, uint32_t table2)
{
    /* the common end of 0x00522A60 / 0x00523BB0: the panel 0x004CD1A0 of table1 (lit at hover 0x13);
     * with a unit in the field (`any`) the panel of table2 (hover 0x14) and texts 0x33:0x1E / 0x1F,
     * else 0x33:5 / 6, at x 0x226; 0x005F8518 the flag */
    GAME_CALL(st, FN_LAYOUT_PANEL, 0, (int32_t) table1, S32(st, G_HOVER) == 0x13, 0, 0);
    if (any) {
        GAME_CALL(st, FN_LAYOUT_PANEL, 0, (int32_t) table2, S32(st, G_HOVER) == 0x14, 0, 0);
        text_at(st, 0x33, 0x1E, AX() + 0x226, AY() + 0x28, S32(st, G_FONT_LARGE));
        text_at(st, 0x33, 0x1F, AX() + 0x226, AY() + 0x36, S32(st, G_FONT_LARGE));
        U8(st, G_DISPATCH_ENABLED) = 1;
        return;
    }
    text_at(st, 0x33, 5, AX() + 0x226, AY() + 0x2B, S32(st, G_FONT_LARGE));
    text_at(st, 0x33, 6, AX() + 0x226, AY() + 0x3A, S32(st, G_FONT_LARGE));
    U8(st, G_DISPATCH_ENABLED) = 0;
}

static void military_origin(game_state *st)
{
    S32(st, G_ADVISOR_X) = S32(st, G_LAYOUT_DX);
    S32(st, G_ADVISOR_Y) = S32(st, G_LAYOUT_DY);
    S32(st, G_PANEL_W) = 0x28;
    S32(st, G_PANEL_H) = 0x1A;
}

static void advisor2_frame(game_state *st)
{
    /* @bin 0x00522A60 advisor page 2 (legions) with at least 5 lines: the origin at the screen's
     * (0x00F53F0C / 0x00F53F08, 40 x 26), the legions 0x004B78F0 into 0x00F53DD4, the requests
     * (0x00521A60); per legion row (0x004B78B0; formation 0x90 bytes from 0x00BCB9C8) still to
     * place (i16 +0x82 < 0): three 30 x 30 frames at x 0x1B8, 0x1F4, 0x230, y 0x53 + 44 row, lit at
     * the hovers 3 row + 1..3, the third only when that request is open (kind 2 when 0x0040B5E0,
     * else the other); one with +0x67 set makes the tail's "any" */
    if (5 - S32(st, G_ADVISOR_SLIDE) > 0) {
        return;
    }
    military_origin(st);
    S32(st, G_LEGION_ROWS) = GAME_CALL0(st, 0x004B78F0u);
    uint8_t other = 0, kind2 = 0;
    military_requests_open(st, &other, &kind2);
    int any = 0;
    for (int32_t row = 0, h = 2, y = 0; row < S32(st, G_LEGION_ROWS); h += 3, y += 0x2C) {
        row++;
        int32_t f = GAME_CALL(st, FN_PLAYER_LEGION_BY_ROW, row);
        uint32_t r = (uint32_t) f * 0x90u;
        if (S16(st, G_FORMATION_U82 + r) >= 0) {
            continue;
        }
        frame_rect(st, AX() + 0x1B8, y + AY() + 0x53, 0x1E, 0x1E, S32(st, G_HOVER) == h - 1);
        frame_rect(st, AX() + 0x1F4, y + AY() + 0x53, 0x1E, 0x1E, S32(st, G_HOVER) == h);
        int away = GAME_CALL(st, FN_FORMATION_HAS_ACTION_0X12, f) != 0;
        int lit = 0;
        if (S16(st, G_FORMATION_U82 + r) < 0 && (away ? kind2 : other) != 0) {
            lit = S32(st, G_HOVER) == h + 1;
        }
        frame_rect(st, AX() + 0x230, y + AY() + 0x53, 0x1E, 0x1E, lit);
        if (U8(st, G_FORMATION_IN_FIELD + r) != 0) {
            any = 1;
        }
    }
    military_tail(st, any, G_ADVISOR2_PANEL_A, G_ADVISOR2_PANEL_B);
}

static void advisor3_frame(game_state *st)
{
    /* @bin 0x00523BB0 advisor page 3 (warships) with at least 5 lines: as page 2 over the player's
     * warships (count player +0x5ED8 into 0x00F53DD0, rows 0x004BC620; figure +0x13A < 0), the
     * figure's +0x0A making "any" (read after the first frame), the third frame lit only when the
     * kind-2 request is open */
    if (5 - S32(st, G_ADVISOR_SLIDE) > 0) {
        return;
    }
    uint8_t ships = U8(st, CITY_BASE + CITY_WARSHIPS + U32(st, G_PLAYER) * 0x49D8u);
    S32(st, G_ADVISOR_X) = S32(st, G_LAYOUT_DX);
    S32(st, G_ADVISOR_Y) = S32(st, G_LAYOUT_DY);
    uint8_t other = 0, kind2 = 0;
    S32(st, G_PANEL_W) = 0x28;
    S32(st, G_PANEL_H) = 0x1A;
    S32(st, G_FLEET_ROWS) = ships;
    military_requests_open(st, &other, &kind2);
    int any = 0;
    for (int32_t row = 0, h = 2, y = 0; row < S32(st, G_FLEET_ROWS); h += 3, y += 0x2C) {
        row++;
        int32_t f = GAME_CALL(st, FN_PLAYER_WARSHIP_BY_ROW, row);
        uint32_t r = (uint32_t) f * 0x184u;
        if (S16(st, FIGURE_BASE + FIG_U132 + r) >= 0) {
            continue;
        }
        frame_rect(st, AX() + 0x1B8, y + AY() + 0x53, 0x1E, 0x1E, S32(st, G_HOVER) == h - 1);
        if (U8(st, FIGURE_BASE + FIG_U02 + r) != 0) {
            any = 1;
        }
        frame_rect(st, AX() + 0x1F4, y + AY() + 0x53, 0x1E, 0x1E, S32(st, G_HOVER) == h);
        frame_rect(st, AX() + 0x230, y + AY() + 0x53, 0x1E, 0x1E, kind2 != 0 && S32(st, G_HOVER) == h + 1);
    }
    military_tail(st, any, G_ADVISOR3_PANEL_A, G_ADVISOR3_PANEL_B);
}

static void festival_square_frame(game_state *st)
{
    /* @bin 0x00516210 window 0x52 (the festival dialog from the festival square, 0x0040AA50) per
     * frame, at fixed places on the screen (its panel at 0x1E, 0x1E): the three festival sizes,
     * each a 430 x 26 frame (0x004D63D0 kind 1 relative to the info panel 0x00EADD90 / 0x00EADD8C,
     * lit at the hovers 6, 7, 8), its title 0x3A:0x1F..0x21 and its cost (player +0x5D74 / +0x5D78
     * / +0x5D7C, 0x004CCC30 after the title's end 0x00E20778); the grand one also its second value
     * (+0x5D80) and an image (0x00F571FC + 15, 0x0053E590 on 0x00F55BB0); then the sizes the
     * treasury (player +0x187C) cannot pay are greyed (0x004CF090): all three at -5000 or less,
     * else each costing more, the grand one also while +0x5D84 is set */
    int32_t px = S32(st, G_INFO_X), py = S32(st, G_INFO_Y);
    uint32_t p = U32(st, G_PLAYER) * 0x49D8u;
    static const int32_t fy[3] = {0xF6, 0x114, 0x132};
    for (int32_t i = 0; i < 3; i++) {
        GAME_CALL(st, FN_LAYOUT_FRAME, 1, 0x84, fy[i], 0x1AE, 0x1A, S32(st, G_HOVER) == 6 + i, px, py);
        px = S32(st, G_INFO_X);
        py = S32(st, G_INFO_Y);
        S32(st, G_TEXT_X) = 0;
        GAME_CALL(st, FN_LAYOUT_GROUP, 0x3A, 0x1F + i, 0x8C, fy[i] + 8, S32(st, G_FONT_LARGE), 0, px, py);
        p = U32(st, G_PLAYER) * 0x49D8u;
        int32_t w = S32(st, G_TEXT_X);
        GAME_CALL(st, FN_TEXT_COUNT_DRAW, 8, 0, S32(st, CITY_BASE + CITY_FESTIVAL_COST + p + 4u * (uint32_t) i), w + 0x8C, fy[i] + 8,
                  S32(st, G_FONT_LARGE), 0);
        if (i == 2) {
            int32_t sum = S32(st, G_TEXT_X) + w;
            S32(st, G_TEXT_X) = sum;
            p = U32(st, G_PLAYER) * 0x49D8u;
            GAME_CALL(st, FN_TEXT_COUNT_DRAW, 8, 0x0A, S32(st, CITY_BASE + CITY_FESTIVAL_BEER + p), sum + 0x96, 0x13A, S32(st, G_FONT_LARGE), 0);
            int32_t x = S32(st, G_TEXT_X) + sum;
            S32(st, G_TEXT_X) = x;
            GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_RESOURCE_ICONS) + 0x0F, x + 0x96, 0x135);
        }
        px = S32(st, G_INFO_X);
        py = S32(st, G_INFO_Y);
    }
    p = U32(st, G_PLAYER) * 0x49D8u;
    int32_t money = S32(st, CITY_BASE + CITY_TREASURY + p);
    if (money <= -5000) {
        for (int32_t i = 0; i < 3; i++) {
            GAME_CALL(st, FN_RECT_GREY, 0x86, fy[i] + 2, 0x1AA, 0x16, 0);
        }
        return;
    }
    if (money < S32(st, CITY_BASE + CITY_FESTIVAL_COST + p)) {
        GAME_CALL(st, FN_RECT_GREY, 0x86, 0xF8, 0x1AA, 0x16, 0);
    }
    p = U32(st, G_PLAYER) * 0x49D8u;
    if (S32(st, CITY_BASE + CITY_TREASURY + p) < S32(st, CITY_BASE + CITY_FESTIVAL_COST + 4 + p)) {
        GAME_CALL(st, FN_RECT_GREY, 0x86, 0x116, 0x1AA, 0x16, 0);
    }
    p = U32(st, G_PLAYER) * 0x49D8u;
    if (S32(st, CITY_BASE + CITY_TREASURY + p) < S32(st, CITY_BASE + CITY_FESTIVAL_COST + 8 + p) || S32(st, CITY_BASE + CITY_U4514 + p) != 0) {
        GAME_CALL(st, FN_RECT_GREY, 0x86, 0x134, 0x1AA, 0x16, 0);
    }
}

static void dialog2e_frame(game_state *st)
{
    /* @bin 0x00525720 window 0x2E per frame (opened from advisor page 4 by
     * 0x0040AD90): the treasury-allowed choices (player +0x5CC0 against the three costs +0x5DE4,
     * +0x5DE8, +0x5DEC, signed): each payable one its label 0x34:0x3F..0x41 at (0x80, y), its name
     * 0x34:(+0x5DD8..+0x5DE0) + 0x33 / 0x37 / 0x3B at 0xE0 lit (font 0x00F56FA4) at the hovers 1..3,
     * and its cost (0x004D6860, string 0x005F89CC) after it; the first one unpayable: the line
     * 0x34:0x46 instead (0x004D6160 over 0x160); when the first is payable: the note
     * 0x34:(+0x5DC8) + 0x42 centred over 240 at y 0x155 and its frame (hover 4); the last frame
     * 160 x 20 at (400, 0x150) (hover 5) */
    static const int32_t ys[3] = {0xDA, 0xEE, 0x102};
    for (uint32_t i = 0; i < 3; i++) {
        uint32_t p = U32(st, G_PLAYER) * 0x49D8u;
        int32_t cost = S32(st, CITY_BASE + CITY_GIFT_COST + p + 4u * i);
        if (cost > S32(st, CITY_BASE + CITY_SAVINGS + p)) {
            if (i == 0) {
                GAME_CALL(st, FN_LAYOUT_GROUP_LINES, 0x34, 0x46, AX() + 0xA0, AY() + 0xE0, 0x160, S32(st, G_FONT_9C), 0,
                          S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
            }
            continue;
        }
        GAME_CALL(st, FN_LAYOUT_GROUP, 0x34, 0x3F + (int32_t) i, AX() + 0x80, AY() + ys[i], S32(st, G_FONT_9C), 0,
                  S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        int32_t font = S32(st, S32(st, G_HOVER) == (int32_t) i + 1 ? G_FONT_RED : G_FONT_9C);
        S32(st, G_TEXT_X) = 0;
        p = U32(st, G_PLAYER) * 0x49D8u;
        GAME_CALL(st, FN_LAYOUT_GROUP, 0x34, S32(st, CITY_BASE + CITY_GIFT_COUNTS + p + 4u * i) + 0x33 + 4 * (int32_t) i, AX() + 0xE0,
                  AY() + ys[i], font, 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        p = U32(st, G_PLAYER) * 0x49D8u;
        GAME_CALL(st, FN_LAYOUT_NUMBER_AT, S32(st, CITY_BASE + CITY_GIFT_COST + p + 4u * i), 0x40, G_STR_DB_CAP,
                  S32(st, G_TEXT_X) + AX() + 0xE0, AY() + ys[i], font, 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    }
    uint32_t p = U32(st, G_PLAYER) * 0x49D8u;
    if (S32(st, CITY_BASE + CITY_GIFT_COST + p) <= S32(st, CITY_BASE + CITY_SAVINGS + p)) {
        GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x34, S32(st, CITY_BASE + CITY_GIFT_SIZE + p) + 0x42, AX() + 0x80, AY() + 0x155, 0xF0,
                  S32(st, G_FONT_LARGE), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        GAME_CALL(st, FN_LAYOUT_FRAME, 1, AX() + 0x80, AY() + 0x150, 0xF0, 0x14, S32(st, G_HOVER) == 4, S32(st, G_LAYOUT_DX),
                  S32(st, G_LAYOUT_DY));
    }
    GAME_CALL(st, FN_LAYOUT_FRAME, 1, AX() + 400, AY() + 0x150, 0xA0, 0x14, S32(st, G_HOVER) == 5, S32(st, G_LAYOUT_DX),
              S32(st, G_LAYOUT_DY));
}

static void advisor4_frame(game_state *st)
{
    /* @bin 0x00524840 advisor page 4 with more than 5 lines: the inner panel 32 x 6 at (0x40, 0x144)
     * (0x004D65B0); the rank 0x20:(player +0x5CBC) in font 0x00F56FBC; 0x34:1 and the savings
     * (+0x5CC0, string 0x005E03EC) after it; the salary (+0x5FF4, string 0x0057C048) at 0xFA then
     * 0x34:0x4F; the frame 250 x 20 at (0x140, 0x160) (hover 1) with 0x34:2 centred; no request
     * running (+0x620C = 0): 0x34:0x4E at (100, 0x18E), else its frame 500 x 20 at (0x46, 0x189)
     * (hover 2), 0x34:(+0x450C) + 4, its amount (+0x4508) and 0x34:3; the frame at (0x140, 0x14A)
     * (hover 3) with 0x34:0x31; then the five request rows that 0x0044D500 finds, 560 x 40 frames at
     * (0x26, 0x60 + 42 k) lit at the hovers 4..8 */
    if (S32(st, G_ADVISOR_SLIDE) <= 5) {
        return;
    }
    int32_t dx = S32(st, G_LAYOUT_DX), dy = S32(st, G_LAYOUT_DY);
#define P4() (U32(st, G_PLAYER) * 0x49D8u)
    GAME_CALL(st, FN_LAYOUT_INNER_PANEL, AX() + 0x40, AY() + 0x144, 0x20, 6, dx, dy);
    GAME_CALL(st, FN_LAYOUT_GROUP, 0x20, S32(st, CITY_BASE + CITY_RANK + P4()), AX() + 0x48, AY() + 0x152, S32(st, G_FONT_LARGE2), 0,
              S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    S32(st, G_TEXT_X) = 0;
    text_at(st, 0x34, 1, AX() + 0x48, AY() + 0x174, S32(st, G_FONT_9C));
    GAME_CALL(st, FN_LAYOUT_NUMBER_AT, S32(st, CITY_BASE + CITY_SAVINGS + P4()), 0x40, G_STR_SPACE_DB, S32(st, G_TEXT_X) + 0x50 + AX(),
              AY() + 0x174, S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    S32(st, G_TEXT_X) = 0;
    GAME_CALL(st, FN_LAYOUT_NUMBER_AT, S32(st, CITY_BASE + CITY_U4784 + P4()), 0x40, G_NUMBER_SUFFIX, AX() + 0xFA, AY() + 0x174,
              S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    text_at(st, 0x34, 0x4F, S32(st, G_TEXT_X) + 0xFA + AX(), AY() + 0x174, S32(st, G_FONT_9C));
    frame_rect(st, AX() + 0x140, AY() + 0x160, 0xFA, 0x14, S32(st, G_HOVER) == 1);
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x34, 2, AX() + 0x140, AY() + 0x165, 0xFA, S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX),
              S32(st, G_LAYOUT_DY));
    if (S32(st, CITY_BASE + CITY_MANSION_BUILT + P4()) == 0) {
        text_at(st, 0x34, 0x4E, AX() + 100, AY() + 0x18E, S32(st, G_FONT_9C));
    } else {
        frame_rect(st, AX() + 0x46, AY() + 0x189, 500, 0x14, S32(st, G_HOVER) == 2);
        S32(st, G_TEXT_X) = 0;
        text_at(st, 0x34, S32(st, CITY_BASE + CITY_SALARY_RANK + P4()) + 4, AX() + 0x78, AY() + 0x18E, S32(st, G_FONT_9C));
        GAME_CALL(st, FN_LAYOUT_NUMBER_AT, S32(st, CITY_BASE + CITY_SALARY + P4()), 0x40, G_NUMBER_SUFFIX, S32(st, G_TEXT_X) + 0x78 + AX(),
                  AY() + 0x18E, S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        text_at(st, 0x34, 3, S32(st, G_TEXT_X) + 0x78 + AX(), AY() + 0x18E, S32(st, G_FONT_9C));
    }
    frame_rect(st, AX() + 0x140, AY() + 0x14A, 0xFA, 0x14, S32(st, G_HOVER) == 3);
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x34, 0x31, AX() + 0x140, AY() + 0x14F, 0xFA, S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX),
              S32(st, G_LAYOUT_DY));
#undef P4
    for (int32_t k = 0; k < 5; k++) {
        int lit = S32(st, G_HOVER) == k + 4;
        if (GAME_CALL(st, FN_REQUEST_ROW, k) != 0) {
            frame_rect(st, AX() + 0x26, AY() + 0x60 + 0x2A * k, 0x230, 0x28, lit);
        }
    }
}

static void message_options_frame(game_state *st)
{
    /* @bin 0x00531480 window 0x56 (the message options, 0x00402BE0) per frame: the panel 26 x 28 at
     * (0x20, 0x26) (0x00536820); fourteen check boxes 0x004D6480(0x0F, 0x80, y, 14, 0, look, dx,
     * dy) at y 0x74 + 24 k (k < 12) and 0x1AC, 0x1C4, look 1 when hovered (k + 1), else 2; texts of
     * group 0x136: 0 (font 0x00F56FB4) and 1 (font 0x00F56F8C read first) centred over 384 at x 0x30,
     * the twelve options 2..13 at (0x80, 0x78 + 24 k) over 224 lit (font 0x00F56FA4) when their bit
     * 2 << k of 0x006DE510 is set, 14 and 15 at 0x1B0 / 0x1C8; then 0x00F53F4C = 50 */
    int32_t font1 = S32(st, G_FONT_LARGE);
    GAME_CALL(st, FN_PANEL_DRAW, S32(st, G_LAYOUT_DX) + 0x20, S32(st, G_LAYOUT_DY) + 0x26, 0x1A, 0x1C);
    for (int32_t k = 0; k < 14; k++) {
        int32_t y = k < 12 ? 0x74 + 0x18 * k : (k == 12 ? 0x1AC : 0x1C4);
        GAME_CALL(st, FN_LAYOUT_SMALL_BUTTON, 0x0F, S32(st, G_LAYOUT_DX) + 0x80, S32(st, G_LAYOUT_DY) + y, 0x0E, 0,
                  (S32(st, G_HOVER) != k + 1) + 1, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    }
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x136, 0, S32(st, G_LAYOUT_DX) + 0x30, S32(st, G_LAYOUT_DY) + 0x3C, 0x180, S32(st, G_FONT_TITLE), 0,
              S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x136, 1, S32(st, G_LAYOUT_DX) + 0x30, S32(st, G_LAYOUT_DY) + 0x5F, 0x180, font1, 0,
              S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    for (int32_t k = 0; k < 12; k++) {
        int32_t font = S32(st, (U32(st, G_MESSAGE_OPTIONS_EDIT) & (2u << k)) != 0 ? G_FONT_RED : G_FONT_LARGE);
        GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x136, k + 2, S32(st, G_LAYOUT_DX) + 0x80, S32(st, G_LAYOUT_DY) + 0x78 + 0x18 * k, 0xE0, font, 0,
                  S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    }
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x136, 0x0E, S32(st, G_LAYOUT_DX) + 0x80, S32(st, G_LAYOUT_DY) + 0x1B0, 0xE0, S32(st, G_FONT_LARGE), 0,
              S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x136, 0x0F, S32(st, G_LAYOUT_DX) + 0x80, S32(st, G_LAYOUT_DY) + 0x1C8, 0xE0, S32(st, G_FONT_LARGE), 0,
              S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    S32(st, G_UF53F4C) = 0x32;
}

/* ---- advisor page 0x0E (0x0052B6D0) ---- */


static int goal_in_hundreds(uint32_t i)
{
    /* @bin 0x0052C4DC (and the same tables 0x0052C4F0, 0x0052C504, 0x0052C518) the requirements
     * 0x18..0x1A, 0x1E, 0x23 count units; the others in range and 0x0A, 0x1C, 0x24 too (checked by
     * address: 0x00784E10, 0x00784E58, 0x00784E78) */
    return !(i == 0x18 || i == 0x19 || i == 0x1A || i == 0x1E || i == 0x23 || i == 0x0A || i == 0x1C || i == 0x24);
}

static void goals_text(game_state *st, int32_t group, int32_t idx, int32_t x, int32_t y)
{
    GAME_CALL(st, FN_LAYOUT_GROUP, group, idx, x, y, S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

static void goals_number(game_state *st, int32_t v, int32_t x, int32_t y)
{
    GAME_CALL(st, FN_LAYOUT_NUMBER_AT, v, 0x40, G_NUMBER_SUFFIX, x, y, S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

static void goals_requirements(game_state *st)
{
    /* @bin 0x0052B6ED the requirements 1..30 of the table 0x0052C4B4 (1, 8, 10, 13, 15, 17..20,
     * 23..26, 28, 30) still asked (0x00784DE8[i] > 0): a row each (42 pixels apart from y 0x42, rect
     * 0x005C5798 + 0x1C row lit at the hover row, rows from 1): the image 0x00F571FC + i at x 0x28
     * (0x0053E590 on 0x00F55BB0), the missing amount (asked less had, 0x00784DE8[i + 0x24]; times
     * 100 for the amounts in hundreds) at 0x3C or text 0xC7:2, its name 0x17:i (0x17:i + 0x36 for
     * the unit counts and, for i = 0x0A or 0x1C, when more than one), the stock (player +0x6160[i],
     * divided by 100 for the units: signed) at 0x12C and 0xC7:1 (units: 0xC7:0x39 for 1, else
     * 0xC7:0x3A) */
    int32_t row = 1, dy = 0;
    uint32_t rect = G_GOAL_RECTS;
    for (uint32_t i = 0; i < 0x24; i++) {
        uint32_t need = G_GOODS_NEEDED + i * 4u;
        if (i == 0 || i > 0x1E) {
            continue;
        }
        static const uint8_t shown[30] = {0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0,
                                          1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0};
        if (shown[i - 1] != 0 || S32(st, need) <= 0) {
            continue;
        }
        int32_t y = dy + AY() + 0x42;
        S32(st, G_TEXT_X) = 0;
        GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_RESOURCE_ICONS) + (int32_t) i, AX() + 0x28, y);
        int32_t missing = S32(st, need) - S32(st, need + 0x90);
        int units = i == 0x18 || i == 0x19 || i == 0x1A || i == 0x1E || i == 0x23;
        if (goal_in_hundreds(i)) {
            missing *= 100;
        }
        GAME_CALL(st, FN_LAYOUT_PANEL, 0, (int32_t) rect, S32(st, G_HOVER) == row, 0, 0);
        S32(st, G_TEXT_X) = 0;
        y = dy + AY() + 0x42;
        if (missing != 0) {
            goals_number(st, missing, AX() + 0x3C, y);
        } else {
            goals_text(st, 0xC7, 2, AX() + 0x3C, y);
        }
        int plural = units || ((i == 0x1C || i == 0x0A) && missing > 1);
        goals_text(st, 0x17, plural ? (int32_t) i + 0x36 : (int32_t) i, AX() + S32(st, G_TEXT_X) + 0x3C,
                   dy + AY() + 0x42);
        int32_t have = S32(st, CITY_BASE + CITY_STORAGE_STOCK + U32(st, G_PLAYER) * 0x49D8u + i * 4u);
        S32(st, G_TEXT_X) = 0;
        if (!goal_in_hundreds(i)) {
            have /= 100;
        }
        goals_number(st, have, AX() + 0x12C, dy + AY() + 0x42);
        int32_t label = units ? (have == 1 ? 0x39 : 0x3A) : 1;
        goals_text(st, 0xC7, label, AX() + S32(st, G_TEXT_X) + 0x12C, dy + AY() + 0x42);
        row++;
        rect += 0x1C;
        dy += 0x2A;
    }
}

static void goals_hover_lit(game_state *st, int32_t row)
{
    /* the lit panel of an unfinished monument's row (0x004CD1A0 kind 0 with the colour 0xF800) */
    if (S32(st, G_HOVER) == row + 1) {
        GAME_CALL(st, FN_LAYOUT_PANEL, 0, (int32_t) (G_GOAL_RECTS + (uint32_t) row * 0x1Cu), 0xF800, 0, 0);
    }
}

static int32_t goals_monument(game_state *st, uint32_t k, int32_t row, int32_t *text, int32_t *text2,
                              int32_t *cost, int32_t *cost2)
{
    /* @bin 0x0052BAF0 the monument k (0..2) of the scenario (kind i16 0x00784DE0[k] > 0, record 16
     * bytes at 0x005D19C8 + 16 kind: +0 1 for a pyramid, +2 its size or model); not placed (player
     * +0x5FA0[k] = 0): its text pair and costs; placed (building player +0x5FAC[k]): its progress
     * (percent) by the model's function, the "in progress" text when below 100 (with the lit panel at
     * the hover row + 1), else done. Returns the percent (100 when not placed or done) */
    uint32_t p = U32(st, G_PLAYER) * 0x49D8u;
    int16_t kind = S16(st, G_MONUMENT_SLOTS + k * 2u);
    uint32_t rec = G_MONUMENT_MODELS + (uint32_t) (int32_t) kind * 16u;
    int32_t v = S16(st, rec + 2);
    int32_t placed = S32(st, CITY_BASE + CITY_MONUMENT_CODE + p + k * 4u);
    int32_t b = S32(st, CITY_BASE + CITY_MONUMENT_ID + p + k * 4u);
    *text2 = 0;
    *cost = *cost2 = 0;
    if (S16(st, rec) == 1) {
        /* @bin 0x0052BB04 a pyramid (jump table 0x0052C524): size 1: 15, 2: 5, 3: 10, 4: 0, else 20 */
        int32_t base = v == 1 ? 0x0F : v == 2 ? 5 : v == 3 ? 0x0A : v == 4 ? 0 : 0x14;
        if (placed == 0) {
            *text = base + 0x0E;
            *text2 = base + 0x0F;
            return 100;
        }
        int32_t status[0x1C];
        pyramid_status(st, b, status);
        int32_t pct = status[0x34 / 4];
        if (pct < 100) {
            *text = base + 0x10;
            *text2 = 0x0D;
            goals_hover_lit(st, row);
            return pct;
        }
        p = U32(st, G_PLAYER) * 0x49D8u;
        *text = S32(st, CITY_BASE + CITY_U4820 + p) == 0 && S32(st, CITY_BASE + CITY_U4828 + p) >= 0x0C ? base + 0x11 : base + 0x12;
        return 100;
    }
    /* @bin 0x0052BBEC the other models (jump table 0x0052C534 on +2 - 1, 0..13; others: the default) */
    struct { uint32_t self, fn; int32_t text; } models[14] = {
        {0, 0, 0x27},                       /* 1: the player's +0x5BAA in sixteenths */
        {0, 0, 0x2B},                       /* 2: the building's +0x55 (u8) */
        {0, FN_MONUMENT_D5_STATUS, 0x2F},             /* 3 */
        {0, 0, 0},                          /* 4: default */
        {G_MONUMENT_E4, FN_MONUMENT_PROGRESS_4E6710, 0x43},   /* 5 */
        {G_MONUMENT_E1, FN_MONUMENT_PROGRESS_40D940, 0x3B},   /* 6 */
        {G_MONUMENT_E3, FN_MONUMENT_PROGRESS_417050, 0x3F},   /* 7 */
        {0, 0, 0}, {0, 0, 0},            /* 8, 9: default */
        {G_MONUMENT_E5, FN_MONUMENT_PROGRESS_PARTS_9, 0x4B},   /* 10 */
        {G_MONUMENT_EA, FN_MONUMENT_PROGRESS_PARTS_10, 0x4F},   /* 11 */
        {G_MONUMENT_EB, FN_MONUMENT_PROGRESS_PARTS_21, 0x53},   /* 12 */
        {G_MONUMENT_EC, FN_MONUMENT_PROGRESS_PARTS_31, 0x57},   /* 13 */
        {G_MONUMENT_E6, FN_MONUMENT_PROGRESS_401540, 0x47},   /* 14 */
    };
    uint32_t m = (uint32_t) (v - 1);
    if (m > 0x0D || models[m].text == 0) {
        /* @bin 0x0052C261 the default: 0x33 / 0x34 with the costs 0x5DC0 and player +0x61D8; placed:
         * 0x004DF040(b, 0), below 100 0x35 (no lit panel), else 0x36 when +0x6090 = 0 and +0x6098 >= 12,
         * else 0x37 */
        if (placed == 0) {
            *text = 0x33;
            *text2 = 0x34;
            *cost = 0x5DC0;
            *cost2 = S32(st, CITY_BASE + CITY_STORAGE_STOCK + 4 * 0x1E + p);
            return 100;
        }
        int32_t pct = GAME_CALL(st, FN_MONUMENT_DE_STATUS, b, 0);
        if (pct < 100) {
            *text = 0x35;
            *text2 = 0x0D;
            return pct;
        }
        p = U32(st, G_PLAYER) * 0x49D8u;
        *text = S32(st, CITY_BASE + CITY_U4820 + p) == 0 && S32(st, CITY_BASE + CITY_U4828 + p) >= 0x0C ? 0x36 : 0x37;
        return 100;
    }
    int32_t t = models[m].text;
    if (placed == 0) {
        *text = t;
        *text2 = t + 1;
        if (v == 2) {
            /* @bin 0x0052BC84 the costs: 20000 unless 0x004F52B0(k) is 0xCF (10000), player +0x61C8 */
            int32_t slot = GAME_CALL(st, FN_MONUMENT_SLOT_TYPE_4F52B0, (int32_t) k);
            *cost = slot != 0xCF ? 20000 : 10000;
            *cost2 = S32(st, CITY_BASE + CITY_STORAGE_STOCK + 4 * 0x1A + U32(st, G_PLAYER) * 0x49D8u);
        } else if (v == 3) {
            *cost = 0x55F0;
            *cost2 = S32(st, CITY_BASE + CITY_STORAGE_STOCK + 4 * 0x1E + p);
        }
        return 100;
    }
    int32_t pct;
    if (v == 1) {
        /* @bin 0x0052BC2C the player's +0x5BAA (i16) x 100 / 16, signed */
        pct = S16(st, CITY_BASE + CITY_U433A + p) * 100 / 16;
    } else if (v == 2) {
        pct = U8(st, BUILDING_BASE + BLD_U55 + (uint32_t) b * 0x108u);
    } else if (models[m].self == 0) {
        pct = GAME_CALL(st, models[m].fn, b, 0);
    } else {
        pct = GAME_CALLT(st, models[m].self, models[m].fn, b, 0);
    }
    if (pct < 100) {
        *text = t + 2;
        *text2 = 0x0D;
        goals_hover_lit(st, row);
        return pct;
    }
    *text = t + 3;
    return 100;
}

static void goals_monuments(game_state *st)
{
    /* @bin 0x0052BA84 the inner panel 35 x 8 at (0x26, 0x11A) (0x004D65B0), then the monuments' rows
     * from row 5 of the table (rect 0x005C5798 + 0x1C row): text 0xC7:text at (8, 8) in the rect
     * (0x004D6200, font 0x00F56F9C, 0x00E20774 cleared), the percent below 100 (0x004CD290, string
     * 0x005E03FC) and 0xC7:2 after it; the first cost / 100; the second line 0xC7:text2 at (8, 0x18)
     * and the second cost / 100 with 0xC7:0x39 (cost 100) or 0x3A */
    GAME_CALL(st, FN_LAYOUT_INNER_PANEL, AX() + 0x26, AY() + 0x11A, 0x23, 8, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    int32_t row = 5;
    for (uint32_t k = 0; k < 3; k++) {
        if (S16(st, G_MONUMENT_SLOTS + k * 2u) <= 0) {
            continue;
        }
        int32_t text, text2, cost, cost2;
        int32_t pct = goals_monument(st, k, row, &text, &text2, &cost, &cost2);
        int32_t font = S32(st, G_FONT_9C);
        int32_t rect = (int32_t) (G_GOAL_RECTS + (uint32_t) row * 0x1Cu);
        S32(st, G_TEXT_X) = 0;
        S32(st, G_TEXT_Y) = 0;
        GAME_CALL(st, FN_LAYOUT_GROUP_AT, 0xC7, text, rect, font, 0, 8, 8, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        if (pct < 100) {
            GAME_CALL(st, FN_LAYOUT_NUMBER, pct, 0x40, G_PERCENT_STRING, rect, font, 0, S32(st, G_TEXT_X) + 8, 8);
            GAME_CALL(st, FN_LAYOUT_GROUP_AT, 0xC7, 2, rect, font, 0, S32(st, G_TEXT_X) + 8, 8, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        }
        if (cost != 0) {
            GAME_CALL(st, FN_LAYOUT_NUMBER, cost / 100, 0x40, G_NUMBER_SUFFIX, rect, font, 0, S32(st, G_TEXT_X) + 8, 8);
        }
        S32(st, G_TEXT_X) = 0;
        if (text2 != 0) {
            GAME_CALL(st, FN_LAYOUT_GROUP_AT, 0xC7, text2, rect, font, 0, 8, 0x18, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        }
        if (cost != 0) {
            GAME_CALL(st, FN_LAYOUT_NUMBER, cost2 / 100, 0x40, G_NUMBER_SUFFIX, rect, font, 0, S32(st, G_TEXT_X) + 8, 0x18);
            GAME_CALL(st, FN_LAYOUT_GROUP_AT, 0xC7, cost2 == 100 ? 0x39 : 0x3A, rect, font, 0, S32(st, G_TEXT_X) + 8, 0x18,
                      S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        }
        row++;
    }
}

static void advisor14_frame(game_state *st)
{
    /* @bin 0x0052B6D0 advisor page 0x0E (the goals): the requirements' rows while grave goods are
     * asked (0x0040CC40(0) != 0), then the monuments' rows */
    if (GAME_CALL(st, FN_GRAVE_GOODS_COUNT, 0) != 0) {
        goals_requirements(st);
    }
    goals_monuments(st);
}

/* ---- port table ---- */

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { (void) a; fn(st); return 0; }

VOID0(run_min_lines, advisors_min_lines)
VOID0(run_adv5, advisor5_frame)
VOID0(run_2a, dialog2a_frame)
VOID0(run_adv7, advisor7_frame)
VOID0(run_2d, dialog2d_frame)
VOID0(run_4c, dialog4c_frame)
VOID0(run_adv2, advisor2_frame)
VOID0(run_adv3, advisor3_frame)
VOID0(run_52, festival_square_frame)
VOID0(run_2e, dialog2e_frame)
VOID0(run_adv4, advisor4_frame)
VOID0(run_56, message_options_frame)
VOID0(run_adv14, advisor14_frame)
static int32_t run_rows(game_state *st, const int32_t *a) { (void) a; return storage_rows(st); }

const ported_function ui_frame_ports[] = {
    {0x00521060u, "advisors_min_lines", 0, 0, 0, run_min_lines, 0},
    {0x004F7770u, "advisor5_frame", 0, 0, 0, run_adv5, 0},
    {0x005027B0u, "storage_rows", 0, 32, 0, run_rows, 0},
    {0x005219C0u, "dialog2a_frame", 0, 0, 0, run_2a, 0},
    {0x005271F0u, "advisor7_frame", 0, 0, 0, run_adv7, 0},
    {0x005253C0u, "dialog2d_frame", 0, 0, 0, run_2d, 0},
    {0x00537A50u, "dialog4c_frame", 0, 0, 0, run_4c, 0},
    {0x00522A60u, "advisor2_frame", 0, 0, 0, run_adv2, 0},
    {0x00523BB0u, "advisor3_frame", 0, 0, 0, run_adv3, 0},
    {0x00516210u, "festival_square_frame", 0, 0, 0, run_52, 0},
    {0x00525720u, "dialog2e_frame", 0, 0, 0, run_2e, 0},
    {0x00524840u, "advisor4_frame", 0, 0, 0, run_adv4, 0},
    {0x00531480u, "message_options_frame", 0, 0, 0, run_56, 0},
    {0x0052B6D0u, "advisor14_frame", 0, 0, 0, run_adv14, 0},
};
const int ui_frame_ports_count = sizeof(ui_frame_ports) / sizeof(ui_frame_ports[0]);
