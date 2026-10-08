/* Imhotep - the advisors: page 5, the ratings (culture, prosperity, monument, kingdom).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "advisors.h"
#include "b.h"

#include "draw/draw.h"
#include "draw/text.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"

#define TEXT_CURSOR 0x00E20778u /* i32 text x after a draw */
#define CITY_STRIDE 0x49D8u     /* the city record (0x00EAAC28 the current one) */
#define RATING_SEL 0x007056C8u  /* i32 the rating selected (its frame lit, 1-4) */

static int32_t group_base(game_state *st, int32_t group)
{
    return GAME_CALLT(st, ADV_IMAGES, 0x004381A0u, group);
}

static uint32_t city(game_state *st, uint32_t field)
{
    return field + (uint32_t) (S32(st, 0x00EAAC28u) * (int32_t) CITY_STRIDE);
}

void ratings_frame(game_state *st, int32_t k)
{
    /* @bin 0x004F7720 the frame of rating k (lit when it is the selected one 0x007056C8) */
    int32_t dx = S32(st, ADV_DX), dy = S32(st, ADV_DY);
    int32_t lit = S32(st, RATING_SEL) == k;
    layout_frame(st, 0, S32(st, ADV_PANEL_X) + k * 0x78 - 0x28, S32(st, ADV_PANEL_Y) + 0x11E, 0x78, 0x42, lit, dx, dy);
}

void ratings_column(game_state *st, int32_t x, int32_t y, int32_t value, int32_t top)
{
    /* @bin 0x004F77E0 a column: the base image [0x00F56A04] (raised by its height +0x0E when the id
     * is below the image count 0x0126517C, else by image 0's), value + value / 2 one-pixel pieces
     * (image +1) above it, then the top (image +2) when `top` */
    int32_t base = S32(st, 0x00F56A04u);
    int32_t i = base < S32(st, 0x0126517Cu) ? base : 0;
    int32_t yy = y - S16(st, 0x010D6A0Eu + (uint32_t) i * 0x40u);
    image_draw(st, base, x, yy);
    int32_t piece = S32(st, 0x00F56A04u) + 1;
    int32_t n = value / 2 + value; /* @bin 0x004F782B cdq / sar: toward zero */
    for (; n > 0; n--) {
        yy--;
        image_draw(st, piece, x + 0x0F, yy);
    }
    if (top != 0) {
        int32_t id = S32(st, 0x00F56A04u) + 2;
        i = id < S32(st, 0x0126517Cu) ? id : 0;
        image_draw(st, id, x - 2, yy - S16(st, 0x010D6A0Eu + (uint32_t) i * 0x40u));
    }
}

static int32_t clamp100(int32_t v)
{
    /* @bin 0x004F6FB7 signed: > 100 -> 100, < 0 -> 0 */
    return v > 0x64 ? 0x64 : (v < 0 ? 0 : v);
}

/* one rating's box: value, goal (shown when `has_goal`), column; k 1..4 */
static void rating_box(game_state *st, int32_t k, uint32_t value_field, uint32_t has_goal, uint32_t goal,
                       int32_t number_font_group, int check_goals)
{
    int32_t px = S32(st, ADV_PANEL_X);
    int32_t py = S32(st, ADV_PANEL_Y);
    int32_t value = S32(st, city(st, value_field));
    int32_t x = px + (k + 1) * 0x78 - 0xA0; /* 0x50, 0xC8, 0x140, 0x1B8 */
    int32_t y = py + 0x126;
    if (k == 4) {
        ratings_frame(st, 4); /* @bin 0x004F7394 the fourth through its helper */
    } else {
        int32_t dx = S32(st, ADV_DX), dy = S32(st, ADV_DY);
        layout_frame(st, 0, x, py + 0x11E, 0x78, 0x42, S32(st, RATING_SEL) == k, dx, dy);
    }
    {
        int32_t font = k == 1 ? S32(st, ADV_FONT_NORMAL) : group_base(st, 0x27B);
        layout_group_centred(st, 0x35, k, x, y, 0x78, font, 0, S32(st, ADV_DX), S32(st, ADV_DY));
    }
    {
        int32_t font = number_font_group ? group_base(st, number_font_group) : S32(st, ADV_FONT_LARGE);
        number_draw_centred(st, value, 0x40, 0x005F76C0u, x, y + 0x0F, 0x6E, font, 0);
    }
    S32(st, TEXT_CURSOR) = 0;
    {
        /* @bin 0x004F6EF2 the goal: [goal] when the flag is set, else 0; rating 1 without a goal
         * uses the normal font, the others image group 0x27B */
        int32_t dx = S32(st, ADV_DX), dy = S32(st, ADV_DY);
        int32_t font, goal_value;
        if (U8(st, has_goal) == 0) {
            font = k == 1 ? S32(st, ADV_FONT_NORMAL) : group_base(st, 0x27B);
            goal_value = 0;
        } else {
            font = group_base(st, 0x27B);
            goal_value = S32(st, goal);
        }
        layout_number_at(st, goal_value, 0x40, 0x007027D4u, x + 5, y + 0x28, font, 0, dx, dy);
    }
    {
        int32_t dx = S32(st, ADV_DX), dy = S32(st, ADV_DY);
        int32_t font = group_base(st, 0x27B);
        layout_group(st, 0x35, 5, S32(st, TEXT_CURSOR) + x + 5, y + 0x28, font, 0, dx, dy);
    }
    /* @bin 0x004F6FA6 the top is drawn when there is no goal or it is met (signed >=) */
    int32_t top = U8(st, has_goal) == 0 || value >= S32(st, goal);
    if (check_goals && (GAME_CALL0(st, 0x004EA930u) & 0xFF) == 0) {
        top = 0; /* @bin 0x004F7327 the monument rating: no top while 0x004EA930 says 0 */
    }
    ratings_column(st, S32(st, ADV_PANEL_X) + (k + 1) * 0x78 - 0x82, S32(st, ADV_PANEL_Y) + 0x112, clamp100(value), top);
}

void advisor_ratings_draw(game_state *st)
{
    /* @bin 0x004F6AF0 advisor page 5: the panel 40 x 27 at the layout's origin (opening: a
     * shrinking rectangle while 5 - [0x00F53F48] is 1..4, nothing at 5 and more), the title, the
     * population and the score goal lines, the four rating boxes, the explanation below */
    int32_t dx = S32(st, ADV_DX), dy = S32(st, ADV_DY);
    int32_t step = 5 - S32(st, 0x00F53F48u);
    S32(st, ADV_PANEL_X) = dx;
    S32(st, ADV_PANEL_Y) = dy;
    S32(st, ADV_PANEL_W) = 0x28;
    S32(st, ADV_PANEL_H) = 0x1B;
    if (step >= 5) {
        return;
    }
    if (step > 0) {
        rect_draw(st, step * 16 + dx, dy + step * 8, (0x28 - 2 * step) * 16, (0x1B - 2 * step) * 16, 0);
        return;
    }
    panel_draw(st, dx, dy, 0x28, 0x1B);
    image_draw(st, S32(st, 0x00F5681Cu) + 3, S32(st, ADV_PANEL_X) + 10, S32(st, ADV_PANEL_Y) + 10);
    S32(st, TEXT_CURSOR) = 0;
    layout_group(st, 0x35, 0, S32(st, ADV_PANEL_X) + 0x3C, S32(st, ADV_PANEL_Y) + 0x0C, S32(st, ADV_FONT_LARGE), 0,
                 S32(st, ADV_DX), S32(st, ADV_DY));
    S32(st, TEXT_CURSOR) = 0;
    layout_group(st, 0x35, 6, S32(st, ADV_PANEL_X) + 0x118, S32(st, ADV_PANEL_Y) + 0x11, S32(st, ADV_FONT_NORMAL), 0,
                 S32(st, ADV_DX), S32(st, ADV_DY));
    {
        int32_t ddy = S32(st, ADV_DY), ddx = S32(st, ADV_DX), font = S32(st, ADV_FONT_NORMAL);
        int32_t y = S32(st, ADV_PANEL_Y) + 0x11;
        int32_t x = S32(st, TEXT_CURSOR) + S32(st, ADV_PANEL_X) + 0x118;
        layout_number_at(st, S32(st, city(st, 0x00EA1894u)), 0x40, 0x0057C048u, x, y, font, 0, ddx, ddy);
    }
    /* @bin 0x004F6C6B the population goal (0x00784C58, value 0x00784C5C) unless the word 0x00784B60 */
    if (S32(st, 0x00784C58u) != 0 && U16(st, 0x00784B60u) == 0) {
        int32_t ddy = S32(st, ADV_DY), ddx = S32(st, ADV_DX), font = S32(st, ADV_FONT_NORMAL);
        S32(st, TEXT_CURSOR) = 0;
        layout_number_at(st, S32(st, 0x00784C5Cu), 0x40, 0x0057C048u, S32(st, ADV_PANEL_X) + 0x118,
                         S32(st, ADV_PANEL_Y) + 0x22, font, 0, ddx, ddy);
        ddy = S32(st, ADV_DY);
        ddx = S32(st, ADV_DX);
        font = S32(st, ADV_FONT_NORMAL);
        int32_t y = S32(st, ADV_PANEL_Y) + 0x22;
        int32_t x = S32(st, TEXT_CURSOR) + S32(st, ADV_PANEL_X) + 0x118;
        layout_group(st, 0x35, 5, x, y, font, 0, ddx, ddy);
    }
    /* @bin 0x004F6D0A the year goal (0x00784C34, kind 0x00784C38: string 29, kind + 40) */
    if (S32(st, 0x00784C34u) != 0 && U16(st, 0x00784B60u) == 0) {
        int32_t ddy = S32(st, ADV_DY), ddx = S32(st, ADV_DX), font = S32(st, ADV_FONT_NORMAL);
        S32(st, TEXT_CURSOR) = 0;
        layout_group(st, 0x1D, S32(st, 0x00784C38u) + 0x28, S32(st, ADV_PANEL_X) + 0x1AE, S32(st, ADV_PANEL_Y) + 0x11,
                     font, 0, ddx, ddy);
        ddy = S32(st, ADV_DY);
        ddx = S32(st, ADV_DX);
        font = S32(st, ADV_FONT_NORMAL);
        int32_t y = S32(st, ADV_PANEL_Y) + 0x11;
        int32_t x = S32(st, TEXT_CURSOR) + S32(st, ADV_PANEL_X) + 0x1AE;
        layout_number_at(st, S32(st, 0x00EADB28u), 0x40, 0x0057C048u, x, y, font, 0, ddx, ddy);
        ddy = S32(st, ADV_DY);
        ddx = S32(st, ADV_DX);
        font = S32(st, ADV_FONT_NORMAL);
        S32(st, TEXT_CURSOR) = 0;
        layout_number_at(st, S32(st, 0x00784C34u), 0x40, 0x0057C048u, S32(st, ADV_PANEL_X) + 0x1AE,
                         S32(st, ADV_PANEL_Y) + 0x22, font, 0, ddx, ddy);
        ddy = S32(st, ADV_DY);
        ddx = S32(st, ADV_DX);
        font = S32(st, ADV_FONT_NORMAL);
        y = S32(st, ADV_PANEL_Y) + 0x22;
        x = S32(st, TEXT_CURSOR) + S32(st, ADV_PANEL_X) + 0x1AE;
        layout_group(st, 0x35, 5, x, y, font, 0, ddx, ddy);
    }
    image_draw(st, S32(st, 0x00F55BC4u), S32(st, ADV_PANEL_X) + 0x3C, S32(st, ADV_PANEL_Y) + 0x30);
    /* @bin 0x004F6E51 the four ratings: city +0x4E64.. (0x00EA5B3C..48), goals flags
     * 0x00784C3C..3F, goals 0x00784C24..30 */
    rating_box(st, 1, 0x00EA5B3Cu, 0x00784C3Cu, 0x00784C24u, 0, 0);
    rating_box(st, 2, 0x00EA5B40u, 0x00784C3Du, 0x00784C28u, 0x280, 0);
    rating_box(st, 3, 0x00EA5B44u, 0x00784C3Eu, 0x00784C2Cu, 0x280, 1);
    rating_box(st, 4, 0x00EA5B48u, 0x00784C3Fu, 0x00784C30u, 0x280, 0);

    layout_inner_panel(st, S32(st, ADV_PANEL_X) + 0x40, S32(st, ADV_PANEL_Y) + 0x164, 0x20, 4, S32(st, ADV_DX),
                       S32(st, ADV_DY));
    /* @bin 0x004F7519 the explanation of the selected rating (city +0x4E64 + 0x164, 0x00EA5CA0:
     * 0 none, 1-4 a rating): its title (string kind) and text of group 53 */
    int32_t x = S32(st, ADV_PANEL_X) + 0x48;
    int32_t py = S32(st, ADV_PANEL_Y);
    int32_t y = py + 0x176;
    int32_t text = -1, title = -1, extra = -1, centred = 0;
    switch (U32(st, city(st, 0x00EA5CA0u))) { /* @bin 0x004F7552 unsigned (ja) */
    case 0:
        y = py + 0x17C;
        text = 8;
        centred = 1;
        break;
    case 1:
        title = 1;
        /* @bin 0x004F758A signed: above 90 the rating's top text */
        text = S32(st, city(st, 0x00EA5B3Cu)) > 0x5A ? 0x41 : S32(st, city(st, 0x00EA5CA4u)) + 9;
        break;
    case 2:
        title = 2;
        if (S32(st, city(st, 0x00EA5B40u)) > 0x5A) {
            text = 0x42;
        } else {
            extra = S32(st, city(st, 0x00EA5B54u)) + 0x14;
            int32_t r = S32(st, city(st, 0x00EA5CA8u));
            if (r != 0) {
                text = r + 0x14;
            }
        }
        break;
    case 3:
        title = 3;
        text = S32(st, city(st, 0x00EA5B44u)) > 0x5A ? 0x43 : S32(st, city(st, 0x00EA5CACu)) + 0x37;
        break;
    case 4:
        title = 4;
        text = S32(st, city(st, 0x00EA5B48u)) > 0x5A ? 0x44 : S32(st, city(st, 0x00EA5CB0u)) + 0x20;
        break;
    default:
        break;
    }
    S32(st, TEXT_CURSOR) = 0;
    if (title >= 0) {
        int32_t ddy = S32(st, ADV_DY), ddx = S32(st, ADV_DX), font = S32(st, 0x00F56F9Cu);
        layout_group(st, 0x35, title, x, py + 0x167, font, 0, ddx, ddy);
    }
    if (extra >= 0) {
        int32_t ddy = S32(st, ADV_DY), ddx = S32(st, ADV_DX);
        int32_t font = group_base(st, 0x27D);
        int32_t yy = S32(st, ADV_PANEL_Y) + 0x167;
        int32_t xx = S32(st, TEXT_CURSOR) + S32(st, ADV_PANEL_X) + 0x48;
        layout_group(st, 0x35, extra, xx, yy, font, 0, ddx, ddy);
    }
    S32(st, TEXT_CURSOR) = 0;
    if (text >= 0) {
        int32_t ddy = S32(st, ADV_DY), ddx = S32(st, ADV_DX);
        int32_t font = group_base(st, 0x27D);
        if (centred) {
            layout_group_centred(st, 0x35, text, x, y, 0x1F0, font, 0, ddx, ddy);
        } else {
            layout_group_lines(st, 0x35, text, x, y, 0x1F0, font, 0, ddx, ddy);
        }
    }
}
