/* Imhotep - the advisors: page 1 (labour) and the labour priority dialog (window 0x2A).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "a.h"
#include "advisors.h"

#include "city/month_health.h"
#include "draw/draw.h"
#include "draw/text.h"
#include "present_layout.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"
#include "state/records.h"

/* a field of the current city's record, by its address in city 0 (imul [0x00EAAC28], 0x49D8) */
static uint32_t city(game_state *st, uint32_t addr)
{
    return addr + U32(st, CURRENT_PLAYER_ADDR) * CITY_REC_SIZE;
}

static int32_t dx(game_state *st) { return S32(st, G_LAYOUT_DX); }
static int32_t dy(game_state *st) { return S32(st, G_LAYOUT_DY); }
static int32_t px(game_state *st) { return S32(st, G_ADVISOR_X); }
static int32_t py(game_state *st) { return S32(st, G_ADVISOR_Y); }

static void group(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, uint32_t font)
{
    layout_group(st, g, i, x, y, S32(st, font), 0, dx(st), dy(st));
}

static void number(game_state *st, int32_t v, uint32_t suffix, int32_t x, int32_t y, int32_t font)
{
    layout_number_at(st, v, 0x40, suffix, x, y, font, 0, dx(st), dy(st));
}

void advisor_labour_draw(game_state *st)
{
    /* @bin 0x00521080 page 1: the panel at the layout's origin (40 x 26 pieces); while the window
     * slides in (5 - [0x00F53F48] in 1..4) only a black frame shrinking toward it */
    int32_t extra = city_0x4c38f0(st); /* @bin 0x00521082 added to the unemployment figure */
    int32_t x0 = S32(st, G_LAYOUT_DX);
    int32_t y0 = S32(st, G_LAYOUT_DY);
    int32_t k = 5 - S32(st, G_ADVISOR_SLIDE);
    S32(st, G_ADVISOR_X) = x0;
    S32(st, G_ADVISOR_Y) = y0;
    S32(st, G_PANEL_W) = 0x28;
    S32(st, G_PANEL_H) = 0x1A;
    if (k >= 5) {
        return;
    }
    if (k > 0) {
        rect_draw(st, k * 16 + x0, y0 + k * 8, (0x28 - 2 * k) * 16, (0x1A - 2 * k) * 16, 0);
        return;
    }
    panel_draw(st, x0, y0, 0x28, 0x1A);
    image_draw(st, S32(st, G_ADVISOR_HEADER_IMAGES), px(st) + 10, py(st) + 10);
    group(st, 0x32, 0, px(st) + 0x3C, py(st) + 0x0C, G_FONT_TITLE);
    group(st, 0x32, 0x15, px(st) + 0x3C, py(st) + 0x38, G_FONT_LARGE);
    group(st, 0x32, 0x16, px(st) + 0xAA, py(st) + 0x38, G_FONT_LARGE);
    group(st, 0x32, 0x17, px(st) + 0x190, py(st) + 0x38, G_FONT_LARGE);
    group(st, 0x32, 0x18, px(st) + 0x1F4, py(st) + 0x38, G_FONT_LARGE);
    /* @bin 0x00521235 "<workers> employed, <jobs> ..., <unemployed>%" (city +0x2BF4, +0x2BF8, +0x2BFC) */
    S32(st, G_TEXT_X) = 0;
    number(st, S32(st, city(st, CITY_BASE + CITY_EMPLOYED)), G_NUMBER_SUFFIX, px(st) + 0x20, py(st) + 0x140,
           S32(st, G_FONT_LARGE));
    group(st, 0x32, 0x0C, S32(st, G_TEXT_X) + px(st) + 0x20, py(st) + 0x140, G_FONT_LARGE);
    number(st, S32(st, city(st, CITY_BASE + CITY_UNEMPLOYED)), G_NUMBER_SUFFIX, S32(st, G_TEXT_X) + px(st) + 0x32, py(st) + 0x140,
           S32(st, G_FONT_LARGE));
    group(st, 0x32, 0x0D, S32(st, G_TEXT_X) + px(st) + 0x32, py(st) + 0x140, G_FONT_LARGE);
    number(st, S32(st, city(st, CITY_BASE + CITY_UNEMPLOYMENT)), G_STR_PERCENT_PAREN, S32(st, G_TEXT_X) + px(st) + 0x32, py(st) + 0x140,
           S32(st, G_FONT_LARGE));
    layout_inner_panel(st, px(st) + 0x40, py(st) + 0x15E, 0x20, 2, dx(st), dy(st));
    S32(st, G_TEXT_X) = 0;
    group(st, 0x32, 0x0E, px(st) + 0x46, py(st) + 0x167, G_FONT_9C);
    S32(st, G_TEXT_X) = 0;
    number(st, S32(st, city(st, CITY_BASE + CITY_WAGE_RATE)), G_NUMBER_SUFFIX, px(st) + 0xE6, py(st) + 0x167, S32(st, G_FONT_9C));
    group(st, 0x32, 0x0F, S32(st, G_TEXT_X) + px(st) + 0xE6, py(st) + 0x167, G_FONT_9C);
    group(st, 0x32, 0x12, S32(st, G_TEXT_X) + px(st) + 0xE6, py(st) + 0x167, G_FONT_9C);
    /* @bin 0x0052152C city +0x2C0C plus 0x004C38F0's result */
    number(st, (int32_t) (U32(st, city(st, CITY_BASE + CITY_WAGE_KINGDOM)) + (uint32_t) extra), G_STR_PAREN,
           S32(st, G_TEXT_X) + px(st) + 0xE6, py(st) + 0x167, S32(st, G_FONT_9C));
    S32(st, G_TEXT_X) = 0;
    group(st, 0x32, 0x13, px(st) + 0x40, py(st) + 0x186, G_FONT_LARGE);
    number(st, S32(st, city(st, CITY_BASE + CITY_WAGES_ESTIMATE)), G_STR_SPACE_DB, S32(st, G_TEXT_X) + px(st) + 0x40, py(st) + 0x186,
           S32(st, G_FONT_LARGE));
}

void advisor_labour_rows_draw(game_state *st)
{
    /* @bin 0x005215D0 the 9 labour categories (records of 0x14 bytes at city +0x2B2C): a frame per
     * row, lit for the chosen one (window 1: [0x007056C8] - 1, else [0x00605988]); the priority
     * (+0x10, with image [0x00F5670C] when set), the name (group 0x32, 1..9), the needed (+0) and
     * the employed (+4, in font [0x00F56FA4] when not equal to the needed); drawn only once the
     * window has slid in ([0x00F53F48] > 5) */
    if (S32(st, G_ADVISOR_SLIDE) <= 5) {
        return;
    }
    layout_inner_panel(st, px(st) + 0x20, py(st) + 0x46, 0x24, 0x0F, dx(st), dy(st));
    int32_t i = 0;
    uint32_t off = 0;
    for (int32_t r = 0; r < 0xE1; r += 0x19, off += CITY_LABOUR_STRIDE) {
        int32_t lit;
        if (U8(st, G_WINDOW) == 1) {
            lit = i == S32(st, G_HOVER) - 1;
        } else {
            lit = S32(st, G_LABOUR_SELECTED) == i;
        }
        layout_frame(st, 0, px(st) + 0x28, r + py(st) + 0x4D, 0x230, 0x16, lit, dx(st), dy(st));
        if (S32(st, city(st, CITY_BASE + CITY_LABOUR_PRIORITY) + off) != 0) {
            image_draw(st, S32(st, G_PRIORITY_IMAGE), px(st) + 0x46, r + py(st) + 0x50);
            number(st, S32(st, city(st, CITY_BASE + CITY_LABOUR_PRIORITY) + off), G_NUMBER_SUFFIX, px(st) + 0x5A, r + py(st) + 0x52,
                   S32(st, G_FONT_9C));
        }
        i++;
        S32(st, G_TEXT_X) = 0;
        group(st, 0x32, i, px(st) + 0xAA, r + py(st) + 0x52, G_FONT_9C);
        number(st, S32(st, city(st, CITY_BASE + CITY_LABOUR_NEEDED) + off), G_NUMBER_SUFFIX, px(st) + 0x19A, r + py(st) + 0x52,
               S32(st, G_FONT_9C));
        int32_t font = S32(st, G_FONT_9C);
        int32_t employed = S32(st, city(st, CITY_BASE + CITY_LABOUR_AVAILABLE) + off);
        if (employed != S32(st, city(st, CITY_BASE + CITY_LABOUR_NEEDED) + off)) {
            font = S32(st, G_FONT_RED);
        }
        number(st, employed, G_NUMBER_SUFFIX, px(st) + 0x1FE, r + py(st) + 0x52, font);
    }
}

void advisor_labour_priority_draw(game_state *st)
{
    /* @bin 0x00521800 window 0x2A: the priority chooser over page 1: a 20 x 9 panel, the title, 9
     * boxes (1..9, greyed from the count [0x00605984] on: signed jl), "no priority", the hint */
    panel_draw(st, px(st) + 0xA0, py(st) + 0xB0, 0x14, 9);
    layout_group_centred(st, 0x32, 0x19, px(st) + 0xA0, py(st) + 0xB9, 0x140, S32(st, G_FONT_TITLE), 0, dx(st),
                         dy(st));
    int32_t i = 0;
    for (int32_t o = 0; o < 0x120; o += 0x20, i++) {
        rect_draw(st, o + px(st) + 0xB2, py(st) + 0xDD, 0x1B, 0x1B, 0);
        layout_group_centred(st, 0x32, i + 0x1B, o + px(st) + 0xB2, py(st) + 0xE0, 0x1B, S32(st, G_FONT_TITLE), 0,
                             dx(st), dy(st));
        if (i >= S32(st, G_PRIORITY_CHOICES)) {
            rect_grey(st, o + px(st) + 0xB3, py(st) + 0xDE, 0x19, 0x19, 1);
        }
    }
    rect_draw(st, px(st) + 0xB4, py(st) + 0x100, 0x118, 0x19, 0);
    layout_group_centred(st, 0x32, 0x1A, px(st) + 0xB4, py(st) + 0x107, 0x118, S32(st, G_FONT_LARGE), 0, dx(st),
                         dy(st));
    layout_group_centred(st, 0x0D, 3, px(st) + 0xA0, py(st) + 0x128, 0x140, S32(st, G_FONT_LARGE), 0, dx(st),
                         dy(st));
}
