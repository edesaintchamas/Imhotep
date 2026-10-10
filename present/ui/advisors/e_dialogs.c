/* Imhotep - the advisors: the dialogs over the hall of group e: the governor's salary (windows 0x2C,
 * 0x4D), the donation to the city (0x2D), the gift to the king (0x2E), and the "open a trade route"
 * dialog (0x26).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "e.h"

#include "draw/draw.h"
#include "draw/text.h"
#include "present_layout.h"
#include "sidebar/sidebar.h"

#define CITY_OFF(st) (U32(st, CURRENT_PLAYER_ADDR) * CITY_REC_SIZE)

#define PX(st) S32(st, G_ADVISOR_X)
#define PY(st) S32(st, G_ADVISOR_Y)

static void group_at(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, int32_t font)
{
    layout_group(st, g, i, x, y, font, 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

static void centred_at(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, int32_t w, int32_t font)
{
    layout_group_centred(st, g, i, x, y, w, font, 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

static void lines_at(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, int32_t w, int32_t font)
{
    layout_group_lines(st, g, i, x, y, w, font, 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

void advisor_salary_draw(game_state *st)
{
    /* @bin 0x00524E20 windows 0x2C / 0x4D: the ranks 4..14 of group 0x34 with their salaries
     * (table i32 0x005D0320, suffix 0x005E03EC " Db"), the hovered one in font [0x00F56FA4]; the
     * advice: 0x4D when [city +0xEA5D44] > 0, else 0x47 when [city +0xEA450C] > [city +0xEA5CBC],
     * else 0x4C; the cancel button (lit when the hovered item is 1) */
    panel_draw(st, PX(st) + 0x80, PY(st) + 0x20, 0x18, 0x19);
    image_draw(st, S32(st, G_RESOURCE_ICONS) + 0x15, PX(st) + 0x90, PY(st) + 0x30);
    centred_at(st, 0x34, 0xF, PX(st) + 0x90, PY(st) + 0x30, 0x170, S32(st, G_FONT_TITLE));
    layout_inner_panel(st, PX(st) + 0x90, PY(st) + 0x50, 0x16, 0xF, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    for (int32_t rank = 4, row = 0; rank - 4 < 0xB; rank++, row += 0x14) {
        int32_t font = S32(st, G_HOVER) == rank - 2 ? S32(st, G_FONT_RED) : S32(st, G_FONT_9C);
        S32(st, G_TEXT_X) = 0;
        group_at(st, 0x34, rank, PX(st) + 0xB0, row + PY(st) + 0x5A, font);
        layout_number_at(st, S32(st, G_SALARY_BY_RANK + 4u * (uint32_t) (rank - 4)), 0x40, G_STR_SPACE_DB,
                         S32(st, G_TEXT_X) + PX(st) + 0xB0, row + PY(st) + 0x5A, font, 0, S32(st, G_LAYOUT_DX),
                         S32(st, G_LAYOUT_DY));
    }
    uint32_t c = CITY_OFF(st);
    int32_t idx;
    if (S32(st, CITY_BASE + CITY_EXTRA_MONTHS + c) > 0) {
        idx = 0x4D;
    } else if (S32(st, CITY_BASE + CITY_SALARY_RANK + c) > S32(st, CITY_BASE + CITY_PLAYER_RANK + c)) {
        idx = 0x47;
    } else {
        idx = 0x4C;
    }
    lines_at(st, 0x34, idx, PX(st) + 0x98, PY(st) + 0x150, 0x150, S32(st, G_FONT_LARGE));
    layout_frame(st, 0, PX(st) + 0xF0, PY(st) + 0x18B, 0xA0, 0x14, S32(st, G_HOVER) == 1, S32(st, G_LAYOUT_DX),
                 S32(st, G_LAYOUT_DY));
    centred_at(st, 0xD, 4, PX(st) + 0xB0, PY(st) + 0x190, 0x120, S32(st, G_FONT_LARGE));
}

static void amount_at(game_state *st, int32_t v, int32_t xo)
{
    number_draw_centred(st, v, 0x40, G_NUMBER_SUFFIX, PX(st) + xo, PY(st) + 0xDD, 0x40, S32(st, G_FONT_9C), 0);
}

void advisor_donation_draw(game_state *st)
{
    /* @bin 0x005250F0 window 0x2D: the amounts 0, 500, 2000, 5000 and "all" (0x34, 0x13), the
     * family savings [city +0xEA5CCC], the buttons' labels */
    panel_draw(st, PX(st) + 0x40, PY(st) + 0xA0, 0x20, 0xA);
    image_draw(st, S32(st, G_RESOURCE_ICONS) + 0x15, PX(st) + 0x60, PY(st) + 0xB0);
    centred_at(st, 0x34, 0x10, PX(st) + 0x50, PY(st) + 0xB0, 0x1F0, S32(st, G_FONT_TITLE));
    layout_inner_panel(st, PX(st) + 0x70, PY(st) + 0xD0, 0x1A, 4, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    amount_at(st, 0, 0x7C);
    amount_at(st, 0x1F4, 0xCC);
    amount_at(st, 0x7D0, 0x11C);
    amount_at(st, 0x1388, 0x16C);
    centred_at(st, 0x34, 0x13, PX(st) + 0x1BC, PY(st) + 0xDD, 0x40, S32(st, G_FONT_9C));
    group_at(st, 0x34, 0x11, PX(st) + 0x80, PY(st) + 0xF8, S32(st, G_FONT_9C));
    layout_number_at(st, S32(st, CITY_BASE + CITY_DONATION + CITY_OFF(st)), 0x40, G_NUMBER_SUFFIX, PX(st) + 0x13C, PY(st) + 0xF8,
                     S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    centred_at(st, 0xD, 4, PX(st) + 0x150, PY(st) + 0x120, 0xA0, S32(st, G_FONT_LARGE));
    centred_at(st, 0x34, 0x12, PX(st) + 0x90, PY(st) + 0x120, 0xA0, S32(st, G_FONT_LARGE));
}

void advisor_gift_draw(game_state *st)
{
    /* @bin 0x005255A0 window 0x2E: the title, the months since the last gift [city +0xEA5DCC], the
     * cancel label */
    panel_draw(st, PX(st) + 0x60, PY(st) + 0x90, 0x1E, 0xF);
    image_draw(st, S32(st, G_RESOURCE_ICONS) + 0x15, PX(st) + 0x80, PY(st) + 0xA0);
    centred_at(st, 0x34, 0x45, PX(st) + 0x80, PY(st) + 0xA0, 0x1B0, S32(st, G_FONT_TITLE));
    layout_inner_panel(st, PX(st) + 0x70, PY(st) + 0xD0, 0x1C, 5, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    S32(st, G_TEXT_X) = 0;
    group_at(st, 0x34, 0x32, PX(st) + 0x90, PY(st) + 0x130, S32(st, G_FONT_LARGE));
    text_count_draw(st, 8, 4, S32(st, CITY_BASE + CITY_U455C + CITY_OFF(st)), S32(st, G_TEXT_X) + PX(st) + 0x90,
                    PY(st) + 0x130, S32(st, G_FONT_LARGE), 0);
    centred_at(st, 0xD, 4, PX(st) + 0x190, PY(st) + 0x155, 0xA0, S32(st, G_FONT_LARGE));
}

void advisor_open_route_draw(game_state *st)
{
    /* @bin 0x00525B90 window 0x26 (over the empire map, at the layout's origin): the title, the text
     * by the empire city [0x00784F18] (its byte +0x55 of 0x0077DE03 records of 0x6A, i.e. 0x0077DE58:
     * lines 1 and 3, else line 1 lower), line 2 */
    int32_t dx = S32(st, G_LAYOUT_DX);
    int32_t dy = S32(st, G_LAYOUT_DY);
    panel_draw(st, dx + 0x50, dy + 0x40, 0x1E, 0xE);
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    layout_group_centred(st, 0x8E, 0, dx + 0x50, dy + 0x50, 0x1E0, S32(st, G_FONT_TITLE), 0, dx, dy);
    uint32_t city = U32(st, G_EMPIRE_SELECTED);
    int32_t font = S32(st, G_FONT_LARGE);
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    if (U8(st, G_EMPIRE_CITIES + 0x58u + city * 0x6Au) != 0) {
        layout_group_lines(st, 0x8E, 1, dx + 0x70, dy + 0x78, 0x1A0, font, 0, dx, dy);
        dx = S32(st, G_LAYOUT_DX);
        dy = S32(st, G_LAYOUT_DY);
        layout_group_lines(st, 0x8E, 3, dx + 0x70, dy + 0xB8, 0x1A0, S32(st, G_FONT_LARGE), 0, dx, dy);
    } else {
        layout_group_lines(st, 0x8E, 1, dx + 0x70, dy + 0x98, 0x1A0, font, 0, dx, dy);
    }
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    layout_group(st, 0x8E, 2, dx + 0x80, dy + 0x100, S32(st, G_FONT_LARGE), 0, dx, dy);
}
