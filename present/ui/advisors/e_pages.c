/* Imhotep - the advisors: pages 11 (religion) and 14 (the monuments' grave goods).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "e.h"

#include "city/day_city.h"
#include "command/orders.h"
#include "draw/draw.h"
#include "draw/text.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"

#define TEXT_CURSOR_X 0x00E20778u
#define FONT_94 0x00F56F94u
#define FONT_9C 0x00F56F9Cu
#define SPACE_STR 0x0057C048u /* " " */

static void group_at(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, int32_t font)
{
    layout_group(st, g, i, x, y, font, 0, S32(st, ADV_DX), S32(st, ADV_DY));
}

static void centred_at(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, int32_t w, int32_t font)
{
    layout_group_centred(st, g, i, x, y, w, font, 0, S32(st, ADV_DX), S32(st, ADV_DY));
}

/* the panel of pages 11 and 14 (as page 4's, 0x00523EA0): 0 when the page is drawn whole */
static int page_panel(game_state *st)
{
    int32_t dx = S32(st, ADV_DX);
    int32_t dy = S32(st, ADV_DY);
    int32_t k = 5 - S32(st, 0x00F53F48u);
    S32(st, ADV_PANEL_X) = dx;
    S32(st, ADV_PANEL_Y) = dy;
    S32(st, ADV_PANEL_W) = 0x28;
    S32(st, ADV_PANEL_H) = 0x1B;
    if (k >= 5) {
        return 1;
    }
    if (k > 0) {
        rect_draw(st, k * 16 + dx, dy + k * 8, (0x28 - 2 * k) * 16, (0x1B - 2 * k) * 16, 0);
        return 1;
    }
    panel_draw(st, dx, dy, 0x28, 0x1B);
    return 0;
}

/* @bin 0x00529DA3 a signed byte of the god's row in the city record ([0x00EAAC28] x 0x49D8 + god) */
static int32_t god_byte(game_state *st, uint32_t base, int32_t god)
{
    return S8(st, base + U32(st, 0x00EAAC28u) * 0x49D8u + (uint32_t) god);
}

static void number_centred(game_state *st, int32_t v, int32_t x, int32_t y)
{
    number_draw_centred(st, v, 0x40, SPACE_STR, x, y, 0x32, S32(st, FONT_9C), 0);
}

/* @bin 0x00529CBD the count of the god's buildings of kind k: [0x00705AF0 + 4 god_building_type] (the
 * coordinates are read before the call) */
static void temple_count(game_state *st, int32_t god, int32_t kind, int32_t dxo, int32_t y)
{
    int32_t font = S32(st, FONT_9C);
    int32_t py = S32(st, ADV_PANEL_Y);
    int32_t x = S32(st, ADV_PANEL_X) + dxo;
    int32_t type = god_building_type(god, kind);
    number_draw_centred(st, S32(st, 0x00705AF0u + 4u * (uint32_t) type), 0x40, SPACE_STR, x, y + py, 0x32, font, 0);
}

void advisor_religion_draw(game_state *st)
{
    /* @bin 0x005298E0 page 11, religion: the gods' rows (temples, shrines, festivals, mood, bolts),
     * the advice line, then the festival box 0x00528F10 */
    if (page_panel(st)) {
        return;
    }
    image_draw(st, S32(st, 0x00F5681Cu) + 9, S32(st, ADV_PANEL_X) + 10, S32(st, ADV_PANEL_Y) + 10);
    group_at(st, 0x3B, 0, S32(st, ADV_PANEL_X) + 0x3C, S32(st, ADV_PANEL_Y) + 0xC, S32(st, ADV_FONT_LARGE));
    centred_at(st, 0x3B, 5, S32(st, ADV_PANEL_X) + 0xB4, S32(st, ADV_PANEL_Y) + 0x20, 0x41, S32(st, ADV_FONT_NORMAL));
    group_at(st, 0x1C, 0x96, S32(st, ADV_PANEL_X) + 0x145, S32(st, ADV_PANEL_Y) + 0x2E, S32(st, ADV_FONT_NORMAL));
    group_at(st, 0x3B, 1, S32(st, ADV_PANEL_X) + 0xFF, S32(st, ADV_PANEL_Y) + 0x2E, S32(st, ADV_FONT_NORMAL));
    centred_at(st, 0x3B, 2, S32(st, ADV_PANEL_X) + 0xB4, S32(st, ADV_PANEL_Y) + 0x2E, 0x41, S32(st, ADV_FONT_NORMAL));
    group_at(st, 0x3B, 3, S32(st, ADV_PANEL_X) + 0x1EA, S32(st, ADV_PANEL_Y) + 0x2E, S32(st, ADV_FONT_NORMAL));
    S32(st, TEXT_CURSOR_X) = 0;
    centred_at(st, 0x3B, 6, S32(st, ADV_PANEL_X) + 0x186, S32(st, ADV_PANEL_Y) + 0x12, 0x41, S32(st, ADV_FONT_NORMAL));
    centred_at(st, 0x3B, 8, S32(st, ADV_PANEL_X) + 0x186, S32(st, ADV_PANEL_Y) + 0x20, 0x41, S32(st, ADV_FONT_NORMAL));
    centred_at(st, 0x3B, 7, S32(st, ADV_PANEL_X) + 0x186, S32(st, ADV_PANEL_Y) + 0x2E, 0x41, S32(st, ADV_FONT_NORMAL));
    layout_inner_panel(st, S32(st, ADV_PANEL_X) + 0x20, S32(st, ADV_PANEL_Y) + 0x3C, 0x24, 0xD, S32(st, ADV_DX),
                       S32(st, ADV_DY));

    /* @bin 0x00529BBB the five gods: known flags i16 0x007848E2[god] (+0x4CE: the patron's), name
     * rectangles 0x005C59E8 (0x1C bytes each), rows 0x28 px apart; 0x00EADD44 cleared once */
    S32(st, 0x00EADD44u) = 0;
    uint32_t known = 0x007848E2u;
    uint32_t rect = 0x005C59E8u;
    for (int32_t god = 0, row = 0; rect < 0x005C5A74u; god++, rect += 0x1C, known += 2, row += 0x28) {
        GAME_CALL(st, 0x004D6200u, 0x9D, god, (int32_t) rect, S32(st, FONT_9C), 0, 0, 0, S32(st, ADV_DX),
                  S32(st, ADV_DY));
        if (S32(st, 0x00EADD44u) != 0) {
            /* @bin 0x0052A05A the raw values: bytes +0xEA5C14, +0xEA6086, +0xEA5C00, +0xEA5C0A */
            number_centred(st, god_byte(st, 0x00EA5C14u, god), S32(st, ADV_PANEL_X) + 0xD2,
                           row + S32(st, ADV_PANEL_Y) + 0x42);
            number_centred(st, god_byte(st, 0x00EA6086u, god), S32(st, ADV_PANEL_X) + 0xF5,
                           row + S32(st, ADV_PANEL_Y) + 0x42);
            number_centred(st, god_byte(st, 0x00EA5C00u, god), S32(st, ADV_PANEL_X) + 0x136,
                           row + S32(st, ADV_PANEL_Y) + 0x42);
            number_centred(st, god_byte(st, 0x00EA5C0Au, god), S32(st, ADV_PANEL_X) + 0x168,
                           row + S32(st, ADV_PANEL_Y) + 0x42);
            continue;
        }
        group_at(st, 0xBB, S16(st, known), S32(st, ADV_PANEL_X) + 0x64, row + S32(st, ADV_PANEL_Y) + 0x43,
                 S32(st, FONT_9C));
        group_at(st, 0x9E, god, S32(st, ADV_PANEL_X) + 0x28, row + S32(st, ADV_PANEL_Y) + 0x57, S32(st, FONT_94));
        if (S16(st, known) != 0) {
            temple_count(st, god, 0, 0x104, row + 0x42);
            temple_count(st, god, 2, 0x14A, row + 0x42);
            /* @bin 0x00529D43 the festivals i32 [0x00EA5C50 + 4 ([0x00EAAC28] x 0x1276 + god)] */
            number_centred(st, S32(st, 0x00EA5C50u + (U32(st, 0x00EAAC28u) * 0x1276u + (uint32_t) god) * 4u),
                           S32(st, ADV_PANEL_X) + 0x186, row + S32(st, ADV_PANEL_Y) + 0x42);
            S32(st, TEXT_CURSOR_X) = 0;
            /* @bin 0x00529DA3 the mood's word: byte +0xEA5C0A / 10 (signed) + 0x14 */
            group_at(st, 0x3B, god_byte(st, 0x00EA5C0Au, god) / 10 + 0x14, S32(st, ADV_PANEL_X) + 0x1C2,
                     row + S32(st, ADV_PANEL_Y) + 0x42, S32(st, FONT_9C));
            /* @bin 0x00529DD9 one bolt [0x00F56794] per 10 of byte +0xEA5C14, then one [0x00F56464] per 10 of
             * byte +0xEA6086, 10 px apart after the word (the bound re-read each time) */
            int32_t x = 0x1C2;
            for (int32_t n = 0; n < god_byte(st, 0x00EA5C14u, god) / 10; n++, x += 0xA) {
                image_draw(st, S32(st, 0x00F56794u), S32(st, TEXT_CURSOR_X) + S32(st, ADV_PANEL_X) + x,
                           row + S32(st, ADV_PANEL_Y) + 0x3E);
            }
            x = 0x1C2;
            for (int32_t n = 0; n < god_byte(st, 0x00EA6086u, god) / 10; n++, x += 0xA) {
                image_draw(st, S32(st, 0x00F56464u), S32(st, TEXT_CURSOR_X) + x + S32(st, ADV_PANEL_X),
                           row + S32(st, ADV_PANEL_Y) + 0x3E);
            }
        } else {
            /* @bin 0x00529EE6 an unknown god: "-" (0x3B, 0x25) in the four columns */
            centred_at(st, 0x3B, 0x25, S32(st, ADV_PANEL_X) + 0x104, row + S32(st, ADV_PANEL_Y) + 0x42, 0x32,
                       S32(st, FONT_9C));
            centred_at(st, 0x3B, 0x25, S32(st, ADV_PANEL_X) + 0x14A, row + S32(st, ADV_PANEL_Y) + 0x42, 0x32,
                       S32(st, FONT_9C));
            centred_at(st, 0x3B, 0x25, S32(st, ADV_PANEL_X) + 0x186, row + S32(st, ADV_PANEL_Y) + 0x42, 0x32,
                       S32(st, FONT_9C));
            centred_at(st, 0x3B, 0x25, S32(st, ADV_PANEL_X) + 0x1CC, row + S32(st, ADV_PANEL_Y) + 0x42, 0x32,
                       S32(st, FONT_9C));
        }
        /* @bin 0x00529FC8 the complexes column: shown when i16 [known + 0x4CE] == 1 */
        if (S16(st, known + 0x4CE) == 1) {
            temple_count(st, god, 1, 0xC8, row + 0x42);
        } else {
            centred_at(st, 0x3B, 0x25, S32(st, ADV_PANEL_X) + 0xC8, row + S32(st, ADV_PANEL_Y) + 0x42, 0x32,
                       S32(st, FONT_9C));
        }
    }
    GAME_CALL0(st, 0x004E9F50u); /* @bin 0x0052A189 result unused */
    /* @bin 0x0052A18E the advice: the angriest god g = [city +0xEA5C78] with wrath byte
     * [city +0xEA5C13 + g] > 4: 0xE + g; else by [city +0xEA5C90] 1: 9 / 10 (by [city +0xEA5BCC]), 2: 0xB,
     * 3: 0xC, other: 0xD, or 0xE + g when [city +0xEA5BCC] is set */
    uint32_t c = U32(st, 0x00EAAC28u) * 0x49D8u;
    int32_t g = S32(st, 0x00EA5C78u + c);
    int32_t idx;
    if (g > 0 && S8(st, 0x00EA5C13u + c + (uint32_t) g) > 4) {
        idx = g + 0xE;
    } else {
        int32_t m = S32(st, 0x00EA5C90u + c);
        if (m == 1) {
            idx = 9 + (S32(st, 0x00EA5BCCu + c) != 0);
        } else if (m == 2) {
            idx = 0xB;
        } else if (m == 3) {
            idx = 0xC;
        } else {
            idx = S32(st, 0x00EA5BCCu + c) == 0 ? 0xD : g + 0xE;
        }
    }
    layout_group_lines(st, 0x3B, idx, S32(st, ADV_PANEL_X) + 0x3C, S32(st, ADV_PANEL_Y) + 0x110, 0x200,
                       S32(st, ADV_FONT_NORMAL), 0, S32(st, ADV_DX), S32(st, ADV_DY));
    GAME_CALL0(st, 0x00528F10u);
}

void advisor_monuments_draw(game_state *st)
{
    /* @bin 0x0052B520 page 14: the title (0xC7, 0) and, when some grave goods are wanted
     * (0x0040CC40(0) != 0), line 3, else line 0xC */
    if (page_panel(st)) {
        return;
    }
    layout_inner_panel(st, S32(st, ADV_PANEL_X) + 0x20, S32(st, ADV_PANEL_Y) + 0x35, 0x24, 0xE, S32(st, ADV_DX),
                       S32(st, ADV_DY));
    image_draw(st, S32(st, 0x00F5681Cu) + 0xC, S32(st, ADV_PANEL_X) + 10, S32(st, ADV_PANEL_Y) + 10);
    S32(st, TEXT_CURSOR_X) = 0;
    group_at(st, 0xC7, 0, S32(st, ADV_PANEL_X) + 0x3C, S32(st, ADV_PANEL_Y) + 0xC, S32(st, ADV_FONT_LARGE));
    if (grave_goods_count(st, 0) != 0) {
        group_at(st, 0xC7, 3, S32(st, TEXT_CURSOR_X) + S32(st, ADV_PANEL_X) + 0x3C, S32(st, ADV_PANEL_Y) + 0x1E,
                 S32(st, ADV_FONT_NORMAL));
    } else {
        group_at(st, 0xC7, 0xC, S32(st, ADV_PANEL_X) + 0x54, S32(st, ADV_PANEL_Y) + 0x8C, S32(st, FONT_9C));
    }
}
