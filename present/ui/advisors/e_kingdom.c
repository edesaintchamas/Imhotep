/* Imhotep - the advisors: page 4, the kingdom (political) advisor: rating, requests, and the gift
 * amounts helper.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "e.h"

#include "city/day_city.h"
#include "city/events_military.h"
#include "city/month_events.h"
#include "city/text_lookup.h"
#include "draw/draw.h"
#include "draw/text.h"
#include "present_layout.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"

#define CITY_OFF(st) (U32(st, G_PLAYER) * CITY_REC_SIZE) /* @bin the current player [0x00EAAC28] x 0x49D8 */

void advisor_gift_amounts(game_state *st)
{
    /* @bin 0x004E7DB0 the three gift sizes from the family savings [city +0xEA5CC0]: /8 + 20, /4 + 50,
     * /2 + 100 (signed, toward zero), into +0xEA5DE4, +0xEA5DE8, +0xEA5DEC */
    uint32_t c = CITY_OFF(st);
    S32(st, CITY_BASE + CITY_GIFT_COST + c) = S32(st, CITY_BASE + CITY_PERSONAL_SAVINGS + c) / 8 + 0x14;
    S32(st, CITY_BASE + CITY_GIFT_COST + 4u + c) = S32(st, CITY_BASE + CITY_PERSONAL_SAVINGS + c) / 4 + 0x32;
    S32(st, CITY_BASE + CITY_GIFT_COST + 8u + c) = S32(st, CITY_BASE + CITY_PERSONAL_SAVINGS + c) / 2 + 0x64;
}

uint32_t advisor_king_name(game_state *st)
{
    /* @bin 0x00447840 "%s %s" (0x005D9A10) of the player name 0x00E38E74 and string [0x00784F0C] of
     * group 0x98, into 0x00785424 (the text cursor 0x00E20768 restored); the player name alone
     * when [0x00784F0C] is 0 (@bin 0x00447885) */
    int32_t v = S32(st, G_U784F0C);
    if (v == 0) {
        return G_PLAYER_NAME;
    }
    uint32_t saved = U32(st, G_TEXT_CURSOR);
    text_seek(st, 0x98, v);
    GAME_CALL(st, FN_CRT_SPRINTF, G_KING_NAME_BUFFER, G_FMT_S_S, G_PLAYER_NAME, S32(st, G_TEXT_CURSOR));
    U32(st, G_TEXT_CURSOR) = saved;
    return G_KING_NAME_BUFFER;
}

int32_t advisor_event_months(game_state *st, uint32_t ev)
{
    /* @bin 0x0044C100 thiscall: the event's i16 +0x2E (months left) */
    return P16(st, ev + 0x2E);
}

/* @bin 0x00524331 (and 0x00524425, 0x00524674: byte tables 0x00524800 ...) kinds 0x18, 0x19, 0x1A,
 * 0x1E, 0x23 (the goods counted in hundreds) */
static int kind_in_units(int32_t t)
{
    return t == 0x18 || t == 0x19 || t == 0x1A || t == 0x1E || t == 0x23;
}

static void group_at(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, int32_t font)
{
    layout_group(st, g, i, x, y, font, 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

static void number_at(game_state *st, int32_t v, int32_t x, int32_t y, int32_t font)
{
    layout_number_at(st, v, 0x40, G_NUMBER_SUFFIX, x, y, font, 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

void advisor_kingdom_draw(game_state *st)
{
    /* @bin 0x00523EA0 page 4: the panel (40 x 27 pieces at the layout's origin), or while the
     * opening animation [0x00F53F48] runs (5 - it in 1..4) a shrinking outline only */
    advisor_gift_amounts(st);
    int32_t dx = S32(st, G_LAYOUT_DX);
    int32_t dy = S32(st, G_LAYOUT_DY);
    int32_t k = 5 - S32(st, G_ADVISOR_SLIDE);
    S32(st, G_ADVISOR_X) = dx;
    S32(st, G_ADVISOR_Y) = dy;
    S32(st, G_PANEL_W) = 0x28;
    S32(st, G_PANEL_H) = 0x1B;
    if (k >= 5) {
        return;
    }
    if (k > 0) {
        rect_draw(st, k * 16 + dx, dy + k * 8, (0x28 - 2 * k) * 16, (0x1B - 2 * k) * 16, 0);
        return;
    }
    panel_draw(st, dx, dy, 0x28, 0x1B);
    image_draw(st, S32(st, G_ADVISOR_HEADER_IMAGES) + 2, S32(st, G_ADVISOR_X) + 10, S32(st, G_ADVISOR_Y) + 10);
    S32(st, G_TEXT_X) = 0;
    group_at(st, 0x34, 0x51, S32(st, G_ADVISOR_X) + 0x3C, S32(st, G_ADVISOR_Y) + 0xC, S32(st, G_FONT_TITLE));
    {
        /* @bin 0x00523FAD the arguments are read before the name call */
        int32_t font = S32(st, G_FONT_TITLE);
        int32_t y = S32(st, G_ADVISOR_Y) + 0xC;
        int32_t x = S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x3C;
        uint32_t name = advisor_king_name(st);
        text_draw(st, name, x, y, font, 0);
    }
    S32(st, G_TEXT_X) = 0;
    group_at(st, 0x34, 0, S32(st, G_ADVISOR_X) + 0x3C, S32(st, G_ADVISOR_Y) + 0x2C, S32(st, G_FONT_LARGE));
    number_at(st, S32(st, CITY_BASE + CITY_RATING_KINGDOM + CITY_OFF(st)), S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x3C,
              S32(st, G_ADVISOR_Y) + 0x2C, S32(st, G_FONT_LARGE));
    /* @bin 0x00524093 the rating [city +0xEA5B48] / 5 (signed) picks the line 0x16 + it */
    layout_group_lines(st, 0x34, S32(st, CITY_BASE + CITY_RATING_KINGDOM + CITY_OFF(st)) / 5 + 0x16, S32(st, G_ADVISOR_X) + 0x3C,
                       S32(st, G_ADVISOR_Y) + 0x3C, 0x220, S32(st, G_FONT_LARGE), 0, S32(st, G_LAYOUT_DX),
                       S32(st, G_LAYOUT_DY));
    layout_inner_panel(st, S32(st, G_ADVISOR_X) + 0x20, S32(st, G_ADVISOR_Y) + 0x5A, 0x24, 0xE, S32(st, G_LAYOUT_DX),
                       S32(st, G_LAYOUT_DY));

    /* @bin 0x005240DA the running requests (events 0x00785D70), one 0x2A-px row each, while the row
     * offset is below 0xD2 */
    int32_t count = 0;
    uint32_t ev = events_next_active(st, G_EVENTS, 0);
    if (ev != 0) {
        int32_t row = 0;
        for (; ev != 0; ev = events_next_active(st, G_EVENTS, ev)) {
            if (!event_is_running(st, ev)) {
                continue;
            }
            if (row >= 0xD2) {
                break;
            }
            if (P16(st, ev + 4) == 0x25) {
                /* @bin 0x0052412E a request for troops: the empire city +0x1C, the strength +0xC */
                int32_t city = empire_city_index(st, (uint32_t) (int32_t) P16(st, ev + 0x1C));
                int32_t strength = P16(st, ev + 0xC);
                int32_t months = event_troops_months_left(st, ev);
                layout_frame(st, 0, S32(st, G_ADVISOR_X) + 0x26, row + S32(st, G_ADVISOR_Y) + 0x60, 0x230, 0x28, 0,
                             S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
                int32_t c = P16(st, ev + 0x1C);
                /* @bin 0x0052418A the city's +0 byte (0x007770F0, records of 0x144) == 2 */
                int at_war = c >= 0 && c < 0x32 && U8(st, G_ARMIES + (uint32_t) c * 0x144u) == 2;
                image_draw(st, S32(st, G_REQUEST_IMAGES) + (at_war ? 1 : 0), S32(st, G_ADVISOR_X) + 0x32,
                           row + S32(st, G_ADVISOR_Y) + 0x6A);
                S32(st, G_TEXT_X) = 0;
                group_at(st, 0x34, at_war ? 0x50 : 0x48, S32(st, G_ADVISOR_X) + 0x50,
                         row + S32(st, G_ADVISOR_Y) + 0x66, S32(st, G_FONT_9C));
                /* @bin 0x0052421C the city name: group 0x15 when [0x00E39088] is set, else 0xC3; index
                 * the byte +0x6A * city + 0x0077DE03 */
                int32_t g = U8(st, G_EMPIRE_LANGUAGE) != 0 ? 0x15 : 0xC3;
                group_at(st, g, U8(st, G_EMPIRE_CITIES + 3u + (uint32_t) city * 0x6Au),
                         S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x50, row + S32(st, G_ADVISOR_Y) + 0x66,
                         S32(st, G_FONT_9C));
                S32(st, G_TEXT_X) = 0;
                /* @bin 0x0052428A the strength's word: < 0x2E 0x49, < 0x59 0x4A, else 0x4B */
                int32_t w = strength < 0x2E ? 0x49 : (strength >= 0x59 ? 0x4B : 0x4A);
                group_at(st, 0x34, w, S32(st, G_ADVISOR_X) + 0x50, row + S32(st, G_ADVISOR_Y) + 0x78,
                         S32(st, G_FONT_9C));
                text_count_draw(st, 8, 4, months, S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x50,
                                row + S32(st, G_ADVISOR_Y) + 0x78, S32(st, G_FONT_9C), 0);
            } else {
                /* @bin 0x0052431A a request for goods or money: kind +4, amount +0xC (x 100 except for
                 * the kinds in units, 0x1C, 0xA and 0x24 = money) */
                int32_t t = P16(st, ev + 4);
                int32_t amount = P16(st, ev + 0xC);
                if (!kind_in_units(t) && t != 0x1C && t != 0xA && t != 0x24) {
                    amount *= 100;
                }
                layout_frame(st, 0, S32(st, G_ADVISOR_X) + 0x26, row + S32(st, G_ADVISOR_Y) + 0x60, 0x230, 0x28, 0,
                             S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
                /* @bin 0x00524384 the icon [0x00F571FC] + kind, money + 0x15 */
                image_draw(st, S32(st, G_RESOURCE_ICONS) + (t == 0x24 ? 0x15 : t), S32(st, G_ADVISOR_X) + 0x28,
                           row + S32(st, G_ADVISOR_Y) + 0x64);
                S32(st, G_TEXT_X) = 0;
                number_at(st, amount, S32(st, G_ADVISOR_X) + 0x42, row + S32(st, G_ADVISOR_Y) + 0x66, S32(st, G_FONT_9C));
                /* @bin 0x00524412 the kind's name in group 0x17: the plural (kind + 0x36) for the kinds in
                 * units, and for 0x1C, 0xA above 1 */
                int32_t idx = t;
                if (kind_in_units(t) || ((t == 0x1C || t == 0xA) && amount > 1)) {
                    idx = t + 0x36;
                }
                group_at(st, 0x17, idx, S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x42,
                         row + S32(st, G_ADVISOR_Y) + 0x66, S32(st, G_FONT_9C));
                {
                    /* @bin 0x005244AC the arguments of 0x004CCC30 pushed before 0x0044C100 */
                    int32_t f = S32(st, G_FONT_9C);
                    int32_t y = row + S32(st, G_ADVISOR_Y) + 0x66;
                    int32_t x = S32(st, G_ADVISOR_X) + 0x136;
                    text_count_draw(st, 8, 4, advisor_event_months(st, ev), x, y, f, 0);
                }
                group_at(st, 0xC, 2, S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x136,
                         row + S32(st, G_ADVISOR_Y) + 0x66, S32(st, G_FONT_9C));
                S32(st, G_TEXT_X) = 0;
                int32_t have;
                int32_t word;
                if (t == 0x24) {
                    /* @bin 0x00524531 money: the treasury [city +0xEA187C] */
                    number_at(st, S32(st, CITY_BASE + CITY_TREASURY + CITY_OFF(st)), S32(st, G_ADVISOR_X) + 0x28,
                              row + S32(st, G_ADVISOR_Y) + 0x78, S32(st, G_FONT_9C));
                    word = 0x2C;
                    group_at(st, 0x34, word, S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x28,
                             row + S32(st, G_ADVISOR_Y) + 0x78, S32(st, G_FONT_9C));
                    have = S32(st, CITY_BASE + CITY_TREASURY + CITY_OFF(st));
                } else {
                    /* @bin 0x00524650 goods: the stock [city +0xEA6160 + 4 kind], / 100 (signed) for the
                     * kinds in units, 0x1C and 0xA */
                    have = S32(st, CITY_BASE + CITY_STORAGE_STOCK + (U32(st, G_PLAYER) * (CITY_REC_SIZE / 4u) + (uint32_t) t) * 4u);
                    if (kind_in_units(t) || t == 0x1C || t == 0xA) {
                        have /= 100;
                    }
                    number_at(st, have, S32(st, G_ADVISOR_X) + 0x28, row + S32(st, G_ADVISOR_Y) + 0x78,
                              S32(st, G_FONT_9C));
                    group_at(st, 0x34, 0x2B, S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x28,
                             row + S32(st, G_ADVISOR_Y) + 0x78, S32(st, G_FONT_9C));
                }
                /* @bin 0x005245C3 enough (signed >=): 0x2F, else 0x30 */
                group_at(st, 0x34, have >= amount ? 0x2F : 0x30, S32(st, G_TEXT_X) + S32(st, G_ADVISOR_X) + 0x50,
                         row + S32(st, G_ADVISOR_Y) + 0x78, S32(st, G_FONT_9C));
            }
            count++;
            row += 0x2A;
        }
        if (count != 0) {
            return;
        }
    }
    /* @bin 0x005247B2 no requests */
    layout_group_lines(st, 0x34, 0x15, S32(st, G_ADVISOR_X) + 0x40, S32(st, G_ADVISOR_Y) + 0xA0, 0x200,
                       S32(st, G_FONT_9C), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}
