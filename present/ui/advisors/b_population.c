/* Imhotep - the advisors: page 7, the population, with its three graphs.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "advisors.h"
#include "b.h"

#include "draw/draw.h"
#include "draw/text.h"
#include "present_layout.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"

#define RED 0xF800              /* @bin 0x00527626 the small graphs' colour, a constant */

static uint32_t city(game_state *st, uint32_t field)
{
    return field + (uint32_t) (S32(st, G_PLAYER) * (int32_t) CITY_REC_SIZE);
}

static int32_t cs32(game_state *st, uint32_t field)
{
    return S32(st, city(st, field));
}

int32_t population_history_at(game_state *st, int32_t months, int32_t i)
{
    /* @bin 0x004E9760 the population i months into the last `months` (ring of 2400 months at
     * city +0x18AC, newest index +0x3E2C, count +0x3E30) */
    int32_t start;
    if (cs32(st, CITY_BASE + CITY_U25C0) > months) {
        start = cs32(st, CITY_BASE + CITY_U25BC) - months;
        if (start < 0) {
            start += 0x960;
        }
    } else {
        start = 0;
    }
    int32_t k = i + start;
    if (k >= 0x960) {
        k -= 0x960;
    }
    return S32(st, city(st, CITY_BASE + CITY_POPULATION_HISTORY) + (uint32_t) k * 4u);
}

int32_t population_history_dates(game_state *st, int32_t months)
{
    /* @bin 0x004E97C0 the first and last month / year of the graph (0x00EA180C, 0x00EA1808 and
     * 0x00EA1804, 0x00EA1800): from the start year 0x007848D8 when the history is short, else back
     * from the current month 0x0078D7C0 - 1 / year 0x0078D7BC. Returns what eax holds */
    if (cs32(st, CITY_BASE + CITY_U25C0) <= months) {
        int32_t year0 = S16(st, G_START_YEAR);
        S32(st, G_GRAPH_FIRST_MONTH) = 0;
        S32(st, G_GRAPH_FIRST_YEAR) = year0;
        S32(st, G_GRAPH_LAST_MONTH) = months % 12;
        S32(st, G_GRAPH_LAST_YEAR) = months / 12 + year0;
        return months < 0; /* @bin 0x004E980D the sign bit of the quotient's high word */
    }
    int32_t year = S32(st, G_YEAR);
    int32_t q = months / 12;
    S32(st, G_GRAPH_LAST_YEAR) = year;
    int32_t month = S32(st, G_MONTH) - 1;
    S32(st, G_GRAPH_LAST_MONTH) = month;
    if (month < 0) {
        month += 12;
        year--;
        S32(st, G_GRAPH_LAST_MONTH) = month;
        S32(st, G_GRAPH_LAST_YEAR) = year;
    }
    int32_t ret = q;
    int32_t first = month - months % 12;
    S32(st, G_GRAPH_FIRST_MONTH) = first;
    if (first < 0) {
        ret = first + 12;
        S32(st, G_GRAPH_FIRST_MONTH) = ret;
    }
    S32(st, G_GRAPH_FIRST_YEAR) = year - q;
    return ret;
}

void population_graph_scale(game_state *st, int32_t max)
{
    /* @bin 0x00527AD0 the scale: the first of 100, 200, 400 ... 51200 at or above max */
    static const int32_t tops[9] = {0x64, 0xC8, 0x190, 0x320, 0x640, 0xC80, 0x1900, 0x3200, 0x6400};
    for (int i = 0; i < 9; i++) {
        if (max <= tops[i]) {
            S32(st, G_SCALE_MAX) = tops[i];
            S32(st, G_SCALE_SHIFT) = i - 1;
            return;
        }
    }
    S32(st, G_SCALE_MAX) = 0xC800;
    S32(st, G_SCALE_SHIFT) = 8;
}

/* the scale's three numbers left of a big graph */
static void scale_numbers(game_state *st, int32_t x, int32_t y)
{
    int32_t lx = x - 0x42;
    number_draw_centred(st, S32(st, G_SCALE_MAX), 0x40, G_NUMBER_SUFFIX, lx, y - 3, 0x3C, S32(st, G_FONT_SMALL), 0);
    number_draw_centred(st, S32(st, G_SCALE_MAX) / 2, 0x40, G_NUMBER_SUFFIX, lx, y + 0x60, 0x3C, S32(st, G_FONT_SMALL), 0);
    number_draw_centred(st, 0, 0x40, G_NUMBER_SUFFIX, lx, y + 0xC4, 0x3C, S32(st, G_FONT_SMALL), 0);
}

static int32_t scaled(int32_t v, int32_t shift)
{
    /* @bin 0x00527564 doubled for shift -1, else sar (cl: the low 5 bits) */
    return shift == -1 ? (int32_t) ((uint32_t) v << 1) : v >> (shift & 0x1F);
}

void population_graph_history(game_state *st, int32_t big, int32_t x, int32_t y)
{
    /* @bin 0x00527360 the population history: 20, 40, 100, 200 or 400 months as the history is long
     * (small: 20 or 100), scaled to its maximum */
    int32_t len = cs32(st, CITY_BASE + CITY_U25C0);
    int32_t n;
    if (len <= 0x14) {
        n = 0x14;
    } else if (len <= 0x28) {
        n = 0x28;
    } else if (len <= 0x64) {
        n = 0x64;
    } else {
        n = len > 0xC8 ? 0x190 : 0xC8;
    }
    if (big == 0) {
        n = n > 0x28 ? 0x64 : 0x14;
    }
    int32_t max = 0;
    for (int32_t i = 0; i < n; i++) {
        int32_t v = population_history_at(st, n, i);
        if (v > max) {
            max = v;
        }
    }
    population_graph_scale(st, max);
    if (big == 0) {
        int32_t shift = S32(st, G_SCALE_SHIFT) + 2;
        S32(st, G_SCALE_SHIFT) = shift;
        int32_t bx = x;
        for (int32_t i = 0; i < n; i++, bx += 5) {
            int32_t v = population_history_at(st, n, i) >> (S32(st, G_SCALE_SHIFT) & 0x1F);
            if (v > 0) {
                if (n == 0x14) {
                    rect_fill(st, bx, y - v + 0x32, 4, v + 1, RED);
                } else {
                    line_draw(st, x + i, y - v + 0x32, x + i, y + 0x32, RED);
                }
            }
        }
        return;
    }
    scale_numbers(st, x, y);
    population_history_dates(st, n);
    int32_t ly = y + 0xD2;
    S32(st, G_TEXT_X) = 0;
    layout_group(st, 0x19, S32(st, G_GRAPH_FIRST_MONTH), x - 0x14, ly, S32(st, G_FONT_SMALL), 0, S32(st, G_LAYOUT_DX),
                 S32(st, G_LAYOUT_DY));
    text_money_draw(st, 0, S32(st, G_GRAPH_FIRST_YEAR), S32(st, G_TEXT_X) + x - 0x14, ly, S32(st, G_FONT_SMALL), 0);
    S32(st, G_TEXT_X) = 0;
    layout_group(st, 0x19, S32(st, G_GRAPH_LAST_MONTH), x + 0x17C, ly, S32(st, G_FONT_SMALL), 0, S32(st, G_LAYOUT_DX),
                 S32(st, G_LAYOUT_DY));
    text_money_draw(st, 0, S32(st, G_GRAPH_LAST_YEAR), S32(st, G_TEXT_X) + x + 0x17C, ly, S32(st, G_FONT_SMALL), 0);
    S32(st, G_CLIP_BOTTOM) = y + 0xC8;
    for (int32_t i = 0; i < n; i++) {
        int32_t v = scaled(population_history_at(st, n, i), S32(st, G_SCALE_SHIFT));
        if (v <= 0) {
            continue;
        }
        int32_t base = S32(st, G_GRAPH_BAR_IMAGES);
        int32_t vy = y - v + 0xC8;
        /* @bin 0x00527577 a bar image per width (20 px, 10, 4, 2), else a line */
        if (n == 0x14) {
            image_draw(st, base, x + i * 0x14, vy);
        } else if (n == 0x28) {
            image_draw(st, base + 1, x + i * 0x0A, vy);
        } else if (n == 0x64) {
            image_draw(st, base + 2, x + i * 4, vy);
        } else if (n == 0xC8) {
            image_draw(st, base + 3, x + i * 2, vy);
        } else {
            line_draw(st, x + i, vy, x + i, y + 0xC7, RED);
        }
    }
    S32(st, G_CLIP_BOTTOM) = S32(st, G_SCREEN_H);
}

void population_graph_census(game_state *st, int32_t big, int32_t x, int32_t y)
{
    /* @bin 0x00527710 the people by age: 100 i16 at city +0x3E34 */
    uint32_t ages = city(st, CITY_BASE + CITY_POPULATION_AGES);
    int32_t max = 0;
    for (uint32_t i = 0; i < 0x64; i++) {
        int32_t v = S16(st, ages + 2 * i);
        if (v > max) {
            max = v;
        }
    }
    population_graph_scale(st, max);
    if (big == 0) {
        S32(st, G_SCALE_SHIFT) = S32(st, G_SCALE_SHIFT) + 2;
        for (int32_t i = 0; i < 0x64; i++) {
            int32_t v = S16(st, city(st, CITY_BASE + CITY_POPULATION_AGES) + 2u * (uint32_t) i) >> (U8(st, G_SCALE_SHIFT) & 0x1F);
            if (v > 0) {
                line_draw(st, i + x, y - v + 0x32, i + x, y + 0x32, RED);
            }
        }
        return;
    }
    scale_numbers(st, x, y);
    for (int32_t a = 0, lx = x - 0x16; a <= 0x64; a += 10, lx += 0x28) {
        number_draw_centred(st, a, 0x40, G_NUMBER_SUFFIX, lx, y + 0xD2, 0x28, S32(st, G_FONT_SMALL), 0);
    }
    S32(st, G_CLIP_BOTTOM) = y + 0xC8;
    for (int32_t i = 0; i < 0x64; i++) {
        int32_t v = scaled(S16(st, city(st, CITY_BASE + CITY_POPULATION_AGES) + 2u * (uint32_t) i), S32(st, G_SCALE_SHIFT));
        if (v > 0) {
            image_draw(st, S32(st, G_GRAPH_BAR_IMAGES) + 2, x + i * 4, y - v + 0xC8);
        }
    }
    S32(st, G_CLIP_BOTTOM) = S32(st, G_SCREEN_H);
}

void population_graph_society(game_state *st, int32_t big, int32_t x, int32_t y)
{
    /* @bin 0x005278E0 the people by house level: 20 i32 at city +0x3EFC */
    uint32_t levels = city(st, CITY_BASE + CITY_POPULATION_BY_HOUSE);
    int32_t max = 0;
    for (uint32_t i = 0; i < 0x14; i++) {
        int32_t v = S32(st, levels + 4 * i);
        if (v > max) {
            max = v;
        }
    }
    population_graph_scale(st, max);
    if (big == 0) {
        S32(st, G_SCALE_SHIFT) = S32(st, G_SCALE_SHIFT) + 2;
        for (int32_t i = 0, bx = x; i < 0x14; i++, bx += 5) {
            int32_t v = S32(st, city(st, CITY_BASE + CITY_POPULATION_BY_HOUSE) + 4u * (uint32_t) i) >> (S32(st, G_SCALE_SHIFT) & 0x1F);
            if (v > 0) {
                rect_fill(st, bx, y - v + 0x32, 4, v + 1, RED);
            }
        }
        return;
    }
    scale_numbers(st, x, y);
    layout_group_centred(st, 0x37, 9, x - 0x50, y + 0xD2, 0xC8, S32(st, G_FONT_SMALL), 0, S32(st, G_LAYOUT_DX),
                         S32(st, G_LAYOUT_DY));
    layout_group_centred(st, 0x37, 10, x + 0x118, y + 0xD2, 0xC8, S32(st, G_FONT_SMALL), 0, S32(st, G_LAYOUT_DX),
                         S32(st, G_LAYOUT_DY));
    S32(st, G_CLIP_BOTTOM) = y + 0xC8;
    for (int32_t i = 0; i < 0x14; i++) {
        int32_t v = scaled(S32(st, city(st, CITY_BASE + CITY_POPULATION_BY_HOUSE) + 4u * (uint32_t) i), S32(st, G_SCALE_SHIFT));
        if (v > 0) {
            image_draw(st, S32(st, G_GRAPH_BAR_IMAGES), x + i * 0x14, y - v + 0xC8);
        }
    }
    S32(st, G_CLIP_BOTTOM) = S32(st, G_SCREEN_H);
}

typedef void (*graph_fn)(game_state *st, int32_t big, int32_t x, int32_t y);

/* the population line's food string (group 55): by food per 100 people */
static void food_line(game_state *st)
{
    int32_t ratio;
    if (cs32(st, CITY_BASE + CITY_POPULATION) == 0) {
        ratio = 0;
    } else {
        int32_t f = GAME_CALL0(st, FN_PLACE_FOOD_RATIO_447530);
        ratio = (int32_t) ((uint32_t) f * 100u) / cs32(st, CITY_BASE + CITY_POPULATION); /* @bin 0x0052659F idiv */
    }
    int32_t food = GAME_CALL0(st, FN_PLACE_FOOD_RATIO_447530);
    int32_t font = S32(st, G_FONT_ROW);
    int32_t font0 = font;
    int32_t s;
    /* @bin 0x005265B8 signed thresholds 150, 110, 95, 85, 70 */
    if (ratio > 0x96) {
        s = 0x33;
    } else if (ratio > 0x6E) {
        s = 0x30;
    } else if (ratio > 0x5F) {
        s = 0x2D;
    } else if (ratio > 0x55) {
        s = 0x2A;
    } else if (ratio > 0x46) {
        font = S32(st, G_FONT_9C);
        s = 0x27;
    } else {
        font = S32(st, G_FONT_RED);
        s = 0x24;
    }
    int32_t trend = cs32(st, CITY_BASE + CITY_MIGRATION_PERCENT);
    if (trend < 0) {
        s += 1;
    } else if (trend > 0) {
        s += 2;
    }
    int32_t px = S32(st, G_ADVISOR_X), py = S32(st, G_ADVISOR_Y);
    S32(st, G_TEXT_X) = 0;
    layout_group(st, 0x37, 0x22, px + 0x4B, py + 0x134, font0, 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    layout_number_at(st, food, 0x40, G_NUMBER_SUFFIX, S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x4B,
                     S32(st, G_ADVISOR_Y) + 0x134, S32(st, G_FONT_ROW), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    layout_group(st, 0x37, 0x23, S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x4B, S32(st, G_ADVISOR_Y) + 0x134,
                 S32(st, G_FONT_ROW), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    layout_group(st, 0x37, s, S32(st, G_ADVISOR_X) + 0x4B, S32(st, G_ADVISOR_Y) + 0x144, font, 0, S32(st, G_LAYOUT_DX),
                 S32(st, G_LAYOUT_DY));
}

/* a line of group 55 at the text cursor (after the previous one) on row dy */
static void group_after(game_state *st, int32_t index, int32_t row)
{
    layout_group(st, 0x37, index, S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x4B, S32(st, G_ADVISOR_Y) + row,
                 S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

static void group_at(game_state *st, int32_t group, int32_t index, int32_t row)
{
    layout_group(st, group, index, S32(st, G_ADVISOR_X) + 0x4B, S32(st, G_ADVISOR_Y) + row, S32(st, G_FONT_9C), 0,
                 S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

static void number_at(game_state *st, int32_t value, int32_t x, int32_t row)
{
    layout_number_at(st, value, 0x40, G_NUMBER_SUFFIX, x, S32(st, G_ADVISOR_Y) + row, S32(st, G_FONT_9C), 0,
                     S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

/* @bin 0x00526CC5 the migration reason (city +0x5C80, 0-5: strings 21-24, 32, 33), each test
 * re-read; the first at the line's start when `fixed` */
static void migration_reason(game_state *st, int fixed)
{
    static const int32_t strings[6] = {0x15, 0x16, 0x17, 0x18, 0x20, 0x21};
    for (int32_t k = 0; k < 6; k++) {
        if (cs32(st, CITY_BASE + CITY_U4410) == k) {
            if (k == 0 && fixed) {
                group_at(st, 0x37, strings[k], 0x18C);
            } else {
                group_after(st, strings[k], 0x18C);
            }
        }
    }
}

void advisor_population_draw(game_state *st)
{
    /* @bin 0x00525CB0 advisor page 7: the panel 40 x 27 (opening as page 5's), the title by the
     * graphs shown 0x00605A40, a big graph and two small ones in their frames, the food,
     * immigration and housing lines */
    int32_t dx = S32(st, G_LAYOUT_DX), dy = S32(st, G_LAYOUT_DY);
    int32_t step = 5 - S32(st, G_ADVISOR_SLIDE);
    S32(st, G_ADVISOR_X) = dx;
    S32(st, G_ADVISOR_Y) = dy;
    S32(st, G_PANEL_W) = 0x28;
    S32(st, G_PANEL_H) = 0x1B;
    if (step >= 5) {
        return;
    }
    if (step > 0) {
        rect_draw(st, step * 16 + dx, dy + step * 8, (0x28 - 2 * step) * 16, (0x1B - 2 * step) * 16, 0);
        return;
    }
    panel_draw(st, dx, dy, 0x28, 0x1B);
    image_draw(st, S32(st, G_ADVISOR_HEADER_IMAGES) + 5, S32(st, G_ADVISOR_X) + 10, S32(st, G_ADVISOR_Y) + 10);
    int32_t g = S32(st, G_GRAPH);
    /* @bin 0x00525D60 the title: 2 for graphs 4.., 1 for 2-3, 0 for 0-1, none below 0 (signed) */
    if (g >= 0) {
        int32_t title = g >= 4 ? 2 : (g >= 2 ? 1 : 0);
        layout_group(st, 0x37, title, S32(st, G_ADVISOR_X) + 0x3C, S32(st, G_ADVISOR_Y) + 0x0C,
                     S32(st, G_FONT_TITLE), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    }
    image_draw(st, S32(st, G_GOD_IMAGES) + 0x0E, S32(st, G_ADVISOR_X) + 0x38, S32(st, G_ADVISOR_Y) + 0x3C);
    layout_frame(st, 0, S32(st, G_ADVISOR_X) + 0x1F5, S32(st, G_ADVISOR_Y) + 0x3C, 0x6A, 0x39, 0, S32(st, G_LAYOUT_DX),
                 S32(st, G_LAYOUT_DY));
    layout_frame(st, 0, S32(st, G_ADVISOR_X) + 0x1F5, S32(st, G_ADVISOR_Y) + 0xA0, 0x6A, 0x39, 0, S32(st, G_LAYOUT_DX),
                 S32(st, G_LAYOUT_DY));
    /* @bin 0x00525E84 graphs 0-5: the big graph's caption, the two small ones', then the big graph
     * and the small ones (history 0x00527360, census 0x00527710, society 0x005278E0) */
    static const struct {
        int32_t caption, small1, small2;
        graph_fn big, top, bottom;
    } graphs[6] = {
        {6, 4, 5, population_graph_history, population_graph_census, population_graph_society},
        {6, 5, 4, population_graph_history, population_graph_society, population_graph_census},
        {7, 3, 5, population_graph_census, population_graph_history, population_graph_society},
        {7, 5, 3, population_graph_census, population_graph_society, population_graph_history},
        {8, 3, 4, population_graph_society, population_graph_history, population_graph_census},
        {8, 4, 3, population_graph_society, population_graph_census, population_graph_history},
    };
    if (g >= 0 && g <= 5) {
        layout_group_centred(st, 0x37, graphs[g].caption, S32(st, G_ADVISOR_X) + 0x3C, S32(st, G_ADVISOR_Y) + 0x118,
                             0x190, S32(st, G_FONT_LARGE), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        layout_group_centred(st, 0x37, graphs[g].small1, S32(st, G_ADVISOR_X) + 0x1F8, S32(st, G_ADVISOR_Y) + 0x78,
                             0x64, S32(st, G_FONT_LARGE), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        layout_group_centred(st, 0x37, graphs[g].small2, S32(st, G_ADVISOR_X) + 0x1F8, S32(st, G_ADVISOR_Y) + 0xDC,
                             0x64, S32(st, G_FONT_LARGE), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        graphs[g].big(st, 1, S32(st, G_ADVISOR_X) + 0x40, S32(st, G_ADVISOR_Y) + 0x40);
        graphs[g].top(st, 0, S32(st, G_ADVISOR_X) + 0x1F9, S32(st, G_ADVISOR_Y) + 0x3F);
        graphs[g].bottom(st, 0, S32(st, G_ADVISOR_X) + 0x1F9, S32(st, G_ADVISOR_Y) + 0xA3);
    }
    layout_inner_panel(st, S32(st, G_ADVISOR_X) + 0x30, S32(st, G_ADVISOR_Y) + 0x131, 0x22, 7, S32(st, G_LAYOUT_DX),
                       S32(st, G_LAYOUT_DY));
    static const int32_t bullets[5] = {0x136, 0x158, 0x16A, 0x17C, 0x18E};
    for (int i = 0; i < 5; i++) {
        image_draw(st, S32(st, G_STATUS_IMAGE), S32(st, G_ADVISOR_X) + 0x38, S32(st, G_ADVISOR_Y) + bullets[i]);
    }
    food_line(st);

    /* @bin 0x00526705 the immigration line: 11 when 0x00784BBC, else the count of
     * immigrants waiting (+0x438C, string 8/6) and why (+0x4388 the emigrants) */
    if (S32(st, G_U784BBC) != 0) {
        group_at(st, 0x37, 0x0B, 0x156);
    } else {
        S32(st, G_TEXT_X) = 0;
        text_count_draw(st, 8, 6, cs32(st, CITY_BASE + CITY_U2B1C), S32(st, G_ADVISOR_X) + 0x4B, S32(st, G_ADVISOR_Y) + 0x156,
                        S32(st, G_FONT_9C), 0);
        if (cs32(st, CITY_BASE + CITY_U2B18) > 0) {
            group_after(st, 0x0C, 0x156);
            text_count_draw(st, 8, 4, cs32(st, CITY_BASE + CITY_U2B18), S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x4B,
                            S32(st, G_ADVISOR_Y) + 0x156, S32(st, G_FONT_9C), 0);
        } else {
            int32_t a = cs32(st, CITY_BASE + CITY_FOOD_TOTAL);
            if (a > cs32(st, CITY_BASE + CITY_U45AC) / 2) { /* @bin 0x005267B4 toward zero, signed */
                group_after(st, 0x0D, 0x156);
            } else if (a <= 0) {
                group_after(st, 0x0E, 0x156);
            } else {
                group_after(st, 0x0F, 0x156);
            }
        }
    }
    /* @bin 0x00526903 the housing line: capacity +0x18A8 */
    {
        int32_t room = cs32(st, CITY_BASE + CITY_HOUSE_ROOM_LEFT);
        S32(st, G_TEXT_X) = 0;
        group_at(st, 0x37, 0x10, 0x168);
        number_at(st, room, S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x4B, 0x168);
        group_after(st, 0x11, 0x168);
    }
    /* @bin 0x005269D2 the migration line by mode +0x3FB0 */
    int32_t mode = cs32(st, CITY_BASE + CITY_U2740);
    if (mode == 0) {
        S32(st, G_TEXT_X) = 0;
        if (cs32(st, CITY_BASE + CITY_U4644) + cs32(st, CITY_BASE + CITY_U4634) > 3) {
            group_at(st, 0x37, 0x1A, 0x17A);
            S32(st, G_TEXT_X) = 0;
            group_at(st, 0x3D, 0x2B, 0x18C);
            return;
        }
        if (cs32(st, CITY_BASE + CITY_IMMIGRANTS_MONTH) >= 5) {
            group_at(st, 0x37, 0x19, 0x17A);
            S32(st, G_TEXT_X) = 0;
            number_at(st, cs32(st, CITY_BASE + CITY_IMMIGRANTS_MONTH), S32(st, G_ADVISOR_X) + 0x4B, 0x18C);
            group_after(st, 0x12, 0x18C);
            return;
        }
        if (cs32(st, CITY_BASE + CITY_U2728) != 0 || cs32(st, CITY_BASE + CITY_HOUSE_ROOM_LEFT) <= 0) {
            group_at(st, 0x37, 0x19, 0x17A);
            S32(st, G_TEXT_X) = 0;
            group_at(st, 0x37, 0x14, 0x18C);
            return;
        }
        if (cs32(st, CITY_BASE + CITY_MIGRATION_PERCENT) >= 0x50) {
            group_at(st, 0x37, 0x19, 0x17A);
            S32(st, G_TEXT_X) = 0;
            number_at(st, cs32(st, CITY_BASE + CITY_IMMIGRANTS_MONTH), S32(st, G_ADVISOR_X) + 0x4B, 0x18C);
            group_after(st, cs32(st, CITY_BASE + CITY_IMMIGRANTS_MONTH) == 1 ? 0x13 : 0x12, 0x18C);
            return;
        }
        group_at(st, 0x37, 0x1A, 0x17A);
        S32(st, G_TEXT_X) = 0;
        migration_reason(st, 1);
    } else if (mode == 1) {
        group_at(st, 0x37, 0x1A, 0x17A);
        migration_reason(st, 0);
    } else {
        layout_group_lines(st, 0x37, mode == 2 ? 0x1E : 0x1F, S32(st, G_ADVISOR_X) + 0x4B, S32(st, G_ADVISOR_Y) + 0x17A,
                           0x1F0, S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    }
}
