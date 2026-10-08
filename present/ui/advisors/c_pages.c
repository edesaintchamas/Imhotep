/* Imhotep - the advisors, group c: pages 8 (0x00527BF0), 9 (0x00528360), 10 (0x00528950) and page
 * 11's festival box (0x00528F10, 0x005292B0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Every element is drawn through the layout helpers (present/draw) with the window origin
 * (0x00E38DA8, 0x00E38DA4); x, y are read again from the panel origin 0x00F53F0C / 0x00F53F08 the
 * page writes first. Text groups 0x38, 0x39, 0x3A are the pages' strings. */
#include "advisors.h"
#include "c.h"

#include "draw/draw.h"
#include "draw/text.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/records.h"

#define PX (S32(st, ADV_PANEL_X))
#define PY (S32(st, ADV_PANEL_Y))
#define DX (S32(st, ADV_DX))
#define DY (S32(st, ADV_DY))
#define FONT_SMALL_B (S32(st, 0x00F56F9Cu)) /* the rows' font */
#define FONT_NORMAL (S32(st, ADV_FONT_NORMAL))
#define FONT_LARGE (S32(st, ADV_FONT_LARGE))
#define TEXT_CURSOR 0x00E20778u /* i32 text cursor x after a draw */
#define TEXT_CURSOR_Y 0x00E20774u
#define SUFFIX 0x0057C048u /* the empty suffix string */
#define CITY (CURRENT_CITY(st))

/* @bin 0x00527BF0 (and the pages 9, 10) the page panel: origin the layout's, w x h pieces; the
 * opening count [0x00F53F48] <= 0 draws nothing, 1..4 a frame of 5 - count pieces in from each
 * side (rect_draw colour 0), else the page. Returns 1 to draw the page. */
static int page_panel(game_state *st, int32_t h)
{
    int32_t open = S32(st, ADV_OPENING);
    int32_t x = DX;
    int32_t y = DY;
    int32_t k = (int32_t) (5u - (uint32_t) open);
    S32(st, ADV_PANEL_X) = x;
    S32(st, ADV_PANEL_Y) = y;
    S32(st, ADV_PANEL_W) = 0x28;
    S32(st, ADV_PANEL_H) = h;
    if (k >= 5) {
        return 0;
    }
    if (k > 0) {
        rect_draw(st, x + k * 16, y + k * 8, (0x28 - 2 * k) * 16, (h - 2 * k) * 16, 0);
        return 0;
    }
    panel_draw(st, x, y, 0x28, h);
    return 1;
}

/* a rating's text: 0 -> zero, >= 100 -> full, else base + rating / 10 (signed division) */
static int32_t rating_text(int32_t v, int32_t zero, int32_t full, int32_t base)
{
    if (v == 0) {
        return zero;
    }
    if (v >= 100) {
        return full;
    }
    return v / 10 + base;
}

void advisor_draw_page8(game_state *st)
{
    /* @bin 0x00527BF0 page 8 (group 0x38): coverage rows (count 0x0070573C[k], people served,
     * rating 0x0078D5F0[j]) and the advice texts */
    if (!page_panel(st, 0x16)) {
        return;
    }
    image_draw(st, S32(st, 0x00F5681Cu) + 6, PX + 10, PY + 10);
    layout_group(st, 0x38, 0, PX + 0x3C, PY + 0xC, FONT_LARGE, 0, DX, DY);
    /* @bin 0x00527CE0 population (city +0x24) < 200: text 0x10, else +0x18 / 10 + 0x11 (signed) */
    uint32_t c = CITY;
    int32_t index = S32(st, c + 0x24u) < 0xC8 ? 0x10 : S32(st, c + 0x18u) / 10 + 0x11;
    layout_group_lines(st, 0x38, index, PX + 0x3C, PY + 0x2E, 0x200, FONT_NORMAL, 0, DX, DY);
    layout_group(st, 0x38, 3, PX + 0xB4, PY + 0x5E, FONT_NORMAL, 0, DX, DY);
    layout_group(st, 0x38, 4, PX + 0x122, PY + 0x5E, FONT_NORMAL, 0, DX, DY);
    layout_group(st, 0x38, 5, PX + 0x1DE, PY + 0x5E, FONT_NORMAL, 0, DX, DY);
    layout_inner_panel(st, PX + 0x20, PY + 0x6C, 0x24, 4, DX, DY);
    for (int32_t i = 0; i < 3; i++) {
        /* @bin 0x00527E49 rows: (count index, text, rating index) (0xCE, 0x27, 10), (0x31, 0x28, 8),
         * (0x2F, 0x2A, 11) */
        int32_t count, text, rating;
        if (i == 0) {
            count = 0xCE, text = 0x27, rating = 0xA;
        } else if (i == 1) {
            count = 0x31, text = 0x28, rating = 8;
        } else {
            count = 0x2F, text = 0x2A, rating = 0xB;
        }
        int32_t row = i * 20;
        layout_group(st, 0x38, text, PX + 0x28, PY + row + 0x70, FONT_SMALL_B, 0, DX, DY);
        number_draw_centred(st, S32(st, 0x0070573Cu + 4u * (uint32_t) count), 0x40, SUFFIX, PX + 0x96,
                            PY + row + 0x70, 0x64, FONT_SMALL_B, 0);
        int32_t r = S32(st, 0x0078D5F0u + 4u * (uint32_t) rating);
        S32(st, TEXT_CURSOR) = 0;
        int32_t pop = S32(st, CITY + 0x24u);
        /* @bin 0x00527F01 people served: population * rating % (0x004CDE40), 0 when either is 0 */
        int32_t served = (r != 0 && pop != 0) ? calc_percentage(pop, r) : 0;
        layout_number_at(st, served, 0x40, SUFFIX, PX + 0x118, PY + row + 0x70, FONT_SMALL_B, 0, DX, DY);
        layout_group(st, 0x38, 6, S32(st, TEXT_CURSOR) + PX + 0x118, PY + row + 0x70, FONT_SMALL_B, 0, DX, DY);
        r = S32(st, 0x0078D5F0u + 4u * (uint32_t) rating);
        layout_group_centred(st, 0x38, rating_text(r, 2, 0x35, 0x2B), PX + 0x1A4, PY + row + 0x70, 0xC8, FONT_SMALL_B,
                             0, DX, DY);
    }
    S32(st, TEXT_CURSOR) = 0;
    S32(st, TEXT_CURSOR_Y) = 0;
    number_draw_centred(st, S32(st, 0x007057F4u), 0x40, SUFFIX, PX + 0x3C, PY + 0xC2, 5, FONT_NORMAL, 0);
    layout_group_lines(st, 0x38, 0x3B, S32(st, TEXT_CURSOR) + PX + 0x39, PY + 0xC2, 0x200, FONT_NORMAL, 0, DX, DY);
    S32(st, TEXT_CURSOR) = 0;
    /* @bin 0x00528153 city +0x4750: 0 -> 0x36, <= 2 -> 0x37, <= 5 -> 0x38, <= 8 -> 0x39, else 0x3A */
    int32_t m = S32(st, CITY + 0x4750u);
    if (m == 0) {
        index = 0x36;
    } else if (m <= 2) {
        index = 0x37;
    } else if (m <= 5) {
        index = 0x38;
    } else {
        index = 0x39 + (m > 8);
    }
    int32_t ty = S32(st, TEXT_CURSOR_Y);
    layout_group_lines(st, 0x38, index, PX + 0x3C, PY + ty + 0xC2, 0x200, FONT_NORMAL, 0, DX, DY);
    int32_t tx = S32(st, TEXT_CURSOR);
    S32(st, TEXT_CURSOR_Y) = ty; /* @bin 0x005281FC the y cursor read before the call, written back */
    layout_group_lines(st, 0x38, 0x3C, tx + PX + 0x39, PY + ty + 0xC2, 0x200, FONT_NORMAL, 0, DX, DY);
    S32(st, TEXT_CURSOR) = 0;
    number_draw_centred(st, S32(st, CITY + 0x2970u), 0x40, SUFFIX, PX + 0x3C, PY + 0xFA, 5, FONT_NORMAL, 0);
    layout_group_lines(st, 0x38, 0x3D, S32(st, TEXT_CURSOR) + PX + 0x39, PY + 0xFA, 0x200, FONT_NORMAL, 0, DX, DY);
    /* @bin 0x005282A5 city +0x441C 1..4 (jump table 0x00528350): 7, 9, 0xB, 0xD + (flag != 0)
     * with the flags +0x4348, +0x4344, +0x434C, +0x4748; else 0xF */
    c = CITY;
    switch (S32(st, c + 0x441Cu)) {
    case 1:
        index = 7 + (S32(st, c + 0x4348u) != 0);
        break;
    case 2:
        index = 9 + (S32(st, c + 0x4344u) != 0);
        break;
    case 3:
        index = 0xB + (S32(st, c + 0x434Cu) != 0);
        break;
    case 4:
        index = 0xD + (S32(st, c + 0x4748u) != 0);
        break;
    default:
        index = 0xF;
        break;
    }
    layout_group_lines(st, 0x38, index, PX + 0x3C, PY + 0x113, 0x200, FONT_NORMAL, 0, DX, DY);
}

void advisor_draw_page9(game_state *st)
{
    /* @bin 0x00528360 page 9 (group 0x39): two coverage rows (counts 0x00705808, 0x00705810,
     * people served count * 300, * 800; ratings 0x0078D604, 0x0078D608) and the advice */
    if (!page_panel(st, 0x10)) {
        return;
    }
    image_draw(st, S32(st, 0x00F5681Cu) + 7, PX + 10, PY + 10);
    layout_group(st, 0x39, 0, PX + 0x3C, PY + 0xC, FONT_LARGE, 0, DX, DY);
    S32(st, TEXT_CURSOR) = 0;
    layout_group(st, 0x39, 1, PX + 0xB4, PY + 0x56, FONT_NORMAL, 0, DX, DY);
    layout_group(st, 0x39, 2, PX + 0x122, PY + 0x56, FONT_NORMAL, 0, DX, DY);
    layout_group(st, 0x39, 3, PX + 0x1DE, PY + 0x56, FONT_NORMAL, 0, DX, DY);
    layout_inner_panel(st, PX + 0x20, PY + 0x64, 0x24, 3, DX, DY);
    static const struct {
        int32_t text, served_text, y;
        uint32_t count, rating;
        uint32_t per;
    } rows[2] = {
        /* @bin 0x00528510 row 1: text 0x1C, count 0x00705808 (served * 300), rating 0x0078D604 */
        {0x1C, 4, 0x69, 0x00705808u, 0x0078D604u, 300},
        /* @bin 0x005286C7 row 2: text 0x1D, count 0x00705810 (served * 800), rating 0x0078D608 */
        {0x1D, 6, 0x7D, 0x00705810u, 0x0078D608u, 800},
    };
    for (int i = 0; i < 2; i++) {
        int32_t y = rows[i].y;
        layout_group(st, 0x39, rows[i].text, PX + 0x28, PY + y, FONT_SMALL_B, 0, DX, DY);
        number_draw_centred(st, S32(st, rows[i].count), 0x40, SUFFIX, PX + 0x96, PY + y, 0x64, FONT_SMALL_B, 0);
        S32(st, TEXT_CURSOR) = 0;
        layout_number_at(st, (int32_t) (U32(st, rows[i].count) * rows[i].per), 0x40, SUFFIX, PX + 0x118, PY + y,
                         FONT_SMALL_B, 0, DX, DY);
        layout_group(st, 0x39, rows[i].served_text, S32(st, TEXT_CURSOR) + PX + 0x118, PY + y, FONT_SMALL_B, 0, DX,
                     DY);
        /* @bin 0x00528601 rating 0 -> 7, >= 100 -> 0x12, else rating / 10 + 8 */
        layout_group_centred(st, 0x39, rating_text(S32(st, rows[i].rating), 7, 0x12, 8), PX + 0x1A4, PY + y, 0xC8,
                             FONT_SMALL_B, 0, DX, DY);
    }
    /* @bin 0x0052886A the advice: city +0x4424 1 -> 0x13 + (+0x4318 != 0), 2 -> 0x15 + (+0x431C != 0),
     * 3 -> 0x17; else +0x4318 == 0 -> 0x18; +0x431C != 0: 0x1A when rating 1 <= rating 2, else 0x1B;
     * else 0x19 + (rating 1 < 100) */
    uint32_t c = CITY;
    int32_t k = S32(st, c + 0x4424u);
    int32_t index;
    if (k == 1) {
        index = 0x13 + (S32(st, c + 0x4318u) != 0);
    } else if (k == 2) {
        index = 0x15 + (S32(st, c + 0x431Cu) != 0);
    } else if (k == 3) {
        index = 0x17;
    } else if (S32(st, c + 0x4318u) == 0) {
        index = 0x18;
    } else {
        int32_t b = S32(st, c + 0x431Cu);
        int32_t r1 = S32(st, 0x0078D604u);
        int32_t r2 = S32(st, 0x0078D608u);
        if (b != 0) {
            index = r1 <= r2 ? 0x1A : 0x19 + ((((int32_t) (r2 > r1)) - 1) & 2);
        } else {
            index = 0x19 + (r1 < 100);
        }
    }
    layout_group_lines(st, 0x39, index, PX + 0x3C, PY + 0xB4, 0x200, FONT_NORMAL, 0, DX, DY);
}

void advisor_draw_page10(game_state *st)
{
    /* @bin 0x00528950 page 10 (group 0x3A): the gods' moods (0x004E9890(0)) and the city ratings
     * (0x004550E0) refreshed first; five rows (count 0x0070573C[k], the city's figure, people
     * served count * per, rating 0x0078D5F0[j]) and the advice */
    GAME_CALL(st, 0x004E9890u, 0);
    GAME_CALL0(st, 0x004550E0u);
    if (!page_panel(st, 0x13)) {
        return;
    }
    image_draw(st, S32(st, 0x00F5681Cu) + 8, PX + 10, PY + 10);
    layout_group(st, 0x3A, 0, PX + 0x3C, PY + 0xC, FONT_LARGE, 0, DX, DY);
    layout_group(st, 0x3A, 1, PX + 0xB4, PY + 0x2A, FONT_NORMAL, 0, DX, DY);
    layout_group(st, 0x3A, 0x37, PX + 0xB4, PY + 0x3E, FONT_NORMAL, 0, DX, DY);
    layout_group(st, 0x3A, 2, PX + 0x104, PY + 0x3E, FONT_NORMAL, 0, DX, DY);
    layout_group(st, 0x3A, 3, PX + 0x154, PY + 0x3E, FONT_NORMAL, 0, DX, DY);
    layout_group(st, 0x3A, 4, PX + 0x1E0, PY + 0x3E, FONT_NORMAL, 0, DX, DY);
    layout_inner_panel(st, PX + 0x20, PY + 0x4C, 0x24, 8, DX, DY);
    /* @bin 0x00528B90 rows (jump table 0x00528F00, row 4 the default): count index, text, rating
     * index, city field, people per count */
    static const struct {
        int32_t count, text, rating;
        uint32_t field;
        int32_t per;
    } rows[5] = {
        {0x1F, 0x2F, 0, 0x4360u, 400},  {0x1E, 0x30, 1, 0x4368u, 700},   {0x21, 0x31, 2, 0x4370u, 1200},
        {0x20, 0x32, 3, 0x4378u, 5000}, {0xE2, 0x38, 4, 0x44C8u, 7500},
    };
    for (int i = 0; i < 5; i++) {
        int32_t field = S32(st, CITY + rows[i].field);
        int32_t y = i * 24 + 0x50;
        uint32_t count = 0x0070573Cu + 4u * (uint32_t) rows[i].count;
        layout_group(st, 0x3A, rows[i].text, PX + 0x28, PY + y, FONT_SMALL_B, 0, DX, DY);
        number_draw_centred(st, S32(st, count), 0x40, SUFFIX, PX + 0x96, PY + y, 0x64, FONT_SMALL_B, 0);
        number_draw_centred(st, field, 0x40, SUFFIX, PX + 0xE6, PY + y, 0x64, FONT_SMALL_B, 0);
        int32_t served = (int32_t) (U32(st, count) * (uint32_t) rows[i].per);
        S32(st, TEXT_CURSOR) = 0;
        layout_number_at(st, served, 0x40, SUFFIX, PX + 0x154, PY + y, FONT_SMALL_B, 0, DX, DY);
        layout_group(st, 0x3A, 5, S32(st, TEXT_CURSOR) + PX + 0x154, PY + y, FONT_SMALL_B, 0, DX, DY);
        /* @bin 0x00528D9C rating 0 -> 0x23, >= 100 -> 0x2E, else rating / 10 + 0x24 */
        int32_t r = S32(st, 0x0078D5F0u + 4u * (uint32_t) rows[i].rating);
        layout_group_centred(st, 0x3A, rating_text(r, 0x23, 0x2E, 0x24), PX + 0x1D6, PY + y, 0x64, FONT_SMALL_B, 0,
                             DX, DY);
    }
    /* @bin 0x00528E48 the advice: city +0x4388 > +0x430C -> 0xA; +0x430C != 0: +0x4380 0 -> 9,
     * 1 -> 0xB, 2 -> 0xC, 3 -> 0xD, else 0xE; else 7 + (+0x4384 != 0) */
    uint32_t c = CITY;
    int32_t a = S32(st, c + 0x430Cu);
    int32_t index;
    if (S32(st, c + 0x4388u) > a) {
        index = 0xA;
    } else if (a != 0) {
        int32_t e = S32(st, c + 0x4380u);
        if (e == 0) {
            index = 9;
        } else if (e == 1) {
            index = 0xB;
        } else if (e == 2) {
            index = 0xC;
        } else {
            index = 0xD + (e != 3);
        }
    } else {
        index = 7 + (S32(st, c + 0x4384u) != 0);
    }
    layout_group_lines(st, 0x3A, index, PX + 0x3C, PY + 0xDC, 0x200, FONT_NORMAL, 0, DX, DY);
}

void advisor_draw_festival_button(game_state *st)
{
    /* @bin 0x005292B0 the "hold a festival" frame (page 11), drawn while the opening count > 5 and
     * no festival is planned (city +0x44F8 == 0); lit when city +0x4814 (i16) != 0 and the
     * hovered button 0x007056C8 is 1 */
    if (S32(st, ADV_OPENING) <= 5) {
        return;
    }
    uint32_t c = CITY;
    if (S32(st, c + 0x44F8u) != 0) {
        return;
    }
    int32_t lit = U16(st, c + 0x4814u) == 0 ? 0 : S32(st, 0x007056C8u) == 1;
    layout_frame(st, 0, PX + 0x66, PY + 0x15C, 0x12C, 0x14, lit, DX, DY);
}

void advisor_draw_festival_panel(game_state *st)
{
    /* @bin 0x00528F10 page 11's festival box: months since the last festival (city +0x438C), the
     * planned festival (city +0x44F8 size, +0x44FC months left) or the button, and the advice of
     * group 0x127 */
    layout_inner_panel(st, PX + 0x30, PY + 0x140, 0x22, 6, DX, DY);
    image_draw(st, S32(st, 0x00F55C5Cu) + 0xF, PX + 0x1CC, PY + 0x143);
    S32(st, TEXT_CURSOR) = 0;
    text_count_draw(st, 8, 4, S32(st, CITY + 0x438Cu), PX + 0x70, PY + 0x148, FONT_SMALL_B, 0);
    layout_group(st, 0x3A, 0xF, S32(st, TEXT_CURSOR) + PX + 0x70, PY + 0x148, FONT_SMALL_B, 0, DX, DY);
    uint32_t c = CITY;
    int32_t size = S32(st, c + 0x44F8u);
    int32_t index;
    if (size != 0) {
        /* @bin 0x00529009 size 1: (2, 0xA), 2: (3, 0x14), else (4, 0x1F): months left > first ->
         * text 0x35, else 0x22; advice second + months - 1 */
        int32_t limit, base;
        if (size == 1) {
            limit = 2, base = 0xA;
        } else if (size == 2) {
            limit = 3, base = 0x14;
        } else {
            limit = 4, base = 0x1F;
        }
        int32_t left = S32(st, c + 0x44FCu);
        S32(st, TEXT_CURSOR) = 0;
        layout_group(st, 0x3A, left > limit ? 0x35 : 0x22, PX + 0x66, PY + 0x160, FONT_SMALL_B, 0, DX, DY);
        left = S32(st, CITY + 0x44FCu);
        index = base + left - 1;
        if (left == 1) {
            layout_group(st, 0x3A, 0x36, S32(st, TEXT_CURSOR) + PX + 0x66, PY + 0x160, FONT_SMALL_B, 0, DX, DY);
        } else {
            /* @bin 0x00529129 the month's name: group 0xA0, (months left + [0x0078D7C0]) % 12 (idiv) */
            int32_t tx = S32(st, TEXT_CURSOR);
            int32_t month = (left + S32(st, 0x0078D7C0u)) % 12;
            layout_group(st, 0xA0, month, tx + PX + 0x66, PY + 0x160, FONT_SMALL_B, 0, DX, DY);
        }
    } else {
        int32_t m;
        if (S32(st, c + 0x4528u) != 0 && S32(st, c + 0x452Cu) != 0) {
            layout_group_centred(st, 0x3A, 0x34, PX + 0x66, PY + 0x160, 0x12C, FONT_SMALL_B, 0, DX, DY);
            /* @bin 0x005291AA months <= 1 -> 7, <= 6 -> 8, else 9 */
            m = S32(st, CITY + 0x438Cu);
            index = m <= 1 ? 7 : 8 + (m > 6);
        } else {
            layout_group_centred(st, 0x3A, 0x10, PX + 0x66, PY + 0x160, 0x12C, FONT_SMALL_B, 0, DX, DY);
            /* @bin 0x00529219 months <= 1, 6, 12, 18, 24, 30 -> 0..5, else 6 */
            m = S32(st, CITY + 0x438Cu);
            if (m <= 1) {
                index = 0;
            } else if (m <= 6) {
                index = 1;
            } else if (m <= 0xC) {
                index = 2;
            } else if (m <= 0x12) {
                index = 3;
            } else if (m <= 0x18) {
                index = 4;
            } else {
                index = 5 + (m > 0x1E);
            }
        }
    }
    layout_group_lines(st, 0x127, index, PX + 0x38, PY + 0x175, 0x17C, FONT_SMALL_B, 0, DX, DY);
    advisor_draw_festival_button(st);
}
