/* Imhotep - the advisors: page 2 (military: the legions) and page 3 (fleet: the warships).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "a.h"
#include "advisors.h"

#include "building/building_spawn.h"
#include "city/day_buildings.h"
#include "city/day_city.h"
#include "command/military.h"
#include "draw/draw.h"
#include "draw/text.h"
#include "figure/formation.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"
#include "state/records.h"

#define TX 0x00E20778u /* i32 the text cursor x after a draw */
#define FONT_ROW 0x00F56F94u
#define FONT_NAME 0x00F56F9Cu
#define STATUS_IMAGE 0x00F5690Cu
#define ORDER_IMAGES 0x00F56834u

static uint32_t city(game_state *st, uint32_t addr)
{
    return addr + U32(st, CURRENT_PLAYER_ADDR) * CITY_REC_SIZE;
}

static int32_t dx(game_state *st) { return S32(st, ADV_DX); }
static int32_t dy(game_state *st) { return S32(st, ADV_DY); }
static int32_t px(game_state *st) { return S32(st, ADV_PANEL_X); }
static int32_t py(game_state *st) { return S32(st, ADV_PANEL_Y); }

static void group(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, uint32_t font)
{
    layout_group(st, g, i, x, y, S32(st, font), 0, dx(st), dy(st));
}

static void frame30(game_state *st, int32_t x, int32_t y)
{
    layout_frame(st, 0, x, y, 0x1E, 0x1E, 0, dx(st), dy(st));
}

int32_t legions_count(game_state *st)
{
    /* @bin 0x004B78F0 formations 1..49 in use (+0 == 1) with +0x42 set */
    int32_t n = 0;
    for (int32_t id = 1; id < 50; id++) {
        uint32_t f = FORMATION_ADDR(id);
        if (U8(st, f) == 1 && U8(st, f + 0x42) != 0) {
            n++;
        }
    }
    return n;
}

int32_t ship_at_wharf(game_state *st, int32_t id)
{
    /* @bin 0x00522D80 1 when the ship stands on the road tile next to its building (+0x5A) that
     * 0x004806C0 finds; the tile globals 0x00D3B4E4 / 0x00D3B4E0 are restored after */
    uint32_t f = FIGURE_ADDR(id);
    int32_t saved_y = S32(st, 0x00D3B4E0u);
    int32_t saved_x = S32(st, 0x00D3B4E4u);
    uint32_t b = BUILDING_ADDR(S16(st, f + 0x5A));
    building_find_road_8(st, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3));
    int32_t r = 1;
    if (S16(st, f + 0x24) != S32(st, 0x00D3B4E4u) || S16(st, f + 0x26) != S32(st, 0x00D3B4E0u)) {
        r = 0;
    }
    S32(st, 0x00D3B4E4u) = saved_x;
    S32(st, 0x00D3B4E0u) = saved_y;
    return r;
}

int32_t ship_health_text(game_state *st, int32_t id)
{
    /* @bin 0x0050FC10 group 0xB8's string for the ship's health share (+0x128 of +0x12C, in %) */
    uint32_t f = FIGURE_ADDR(id);
    int32_t p = calc_ratio_percent(S32(st, f + 0x128), S32(st, f + 0x12C));
    if (p <= 0x0F) {
        return 8;
    }
    if (p <= 0x1E) {
        return 7;
    }
    if (p <= 0x32) {
        return 6;
    }
    if (p <= 0x46) {
        return 5;
    }
    return (p <= 0x5A) + 3;
}

int32_t ship_state_text(game_state *st, int32_t id)
{
    /* @bin 0x0050FC70 group 0xB8's string for the ship's state +0x154: 0x1C, 0x1D (1), 0x1E */
    uint8_t s = U8(st, FIGURE_ADDR(id) + 0x154);
    if (s == 0) {
        return 0x1C;
    }
    return (s != 1) + 0x1D;
}

/* @bin 0x00521B50 / 0x00522E10 the shared part of pages 2 and 3: the panel, the war events and
 * the requests (0x00521A60), the status lines and the list's inner panel */
static void header(game_state *st)
{
    panel_draw(st, dx(st), dy(st), 0x28, 0x1A);
    image_draw(st, S32(st, 0x00F5681Cu) + 1, px(st) + 10, py(st) + 10);
}

typedef struct {
    uint8_t other, kind2;      /* 0x00521A60's outputs */
    uint8_t ours_67, other_67; /* an active event of type 0x25 with +0x30 == 6 or 7 */
    uint8_t ours_5, other_5;   /* ... with +0x30 == 5 */
} war_flags;

static void scan_events(game_state *st, war_flags *w)
{
    /* @bin 0x00521DDF the active events (0x0044BAF0 on the list 0x00785D70) of type 0x25: whether
     * their city +0x1C (0..49) is of kind 2 (0x007770F0 + city * 0x144) */
    w->other_67 = w->ours_67 = w->other_5 = w->ours_5 = 0;
    uint32_t e = events_next_active(st, EVENTS, 0);
    while (e != 0) {
        if (S16(st, e + 4) == 0x25) {
            int32_t c = S16(st, e + 0x1C);
            int ours = 0;
            if (c >= 0 && c < 0x32) {
                ours = U8(st, 0x007770F0u + (uint32_t) c * 0x144u) == 2;
            }
            int16_t k = S16(st, e + 0x30);
            if (k == 5) {
                if (ours) {
                    w->ours_5 = 1;
                } else {
                    w->other_5 = 1;
                }
            }
            if (k == 6 || k == 7) {
                if (ours) {
                    w->ours_67 = 1;
                } else {
                    w->other_67 = 1;
                }
            }
        }
        e = events_next_active(st, EVENTS, e);
    }
}

static void status_lines(game_state *st, const war_flags *w, int32_t y1)
{
    /* @bin 0x00521F89 the first status line (group 0x33): 0xB when city +0x4644, 0xA when +0x4634,
     * 9 when 0x00446610, else 8 */
    int32_t i;
    if (S32(st, city(st, 0x00EA5EB4u)) != 0) {
        i = 0x0B;
    } else if (S32(st, city(st, 0x00EA5EA4u)) != 0) {
        i = 0x0A;
    } else if (GAME_CALL0(st, 0x00446610u) != 0) {
        i = 9;
    } else {
        i = 8;
    }
    group(st, 0x33, i, px(st) + 0x46, py(st) + y1, ADV_FONT_NORMAL);
    image_draw(st, S32(st, STATUS_IMAGE), px(st) + 0x32, py(st) + 0x185);
    /* @bin 0x005220A2 the second: the first that holds of 0xF, 0x1C, 0xE, 0x1B, 0xD, 0x1A, else 0xC */
    if (w->other_67) {
        i = 0x0F;
    } else if (w->ours_67) {
        i = 0x1C;
    } else if (w->other_5) {
        i = 0x0E;
    } else if (w->ours_5) {
        i = 0x1B;
    } else if (w->other) {
        i = 0x0D;
    } else if (w->kind2) {
        i = 0x1A;
    } else {
        i = 0x0C;
    }
    group(st, 0x33, i, px(st) + 0x46, py(st) + 0x184, ADV_FONT_NORMAL);
    layout_inner_panel(st, px(st) + 0x20, py(st) + 0x46, 0x24, 0x11, dx(st), dy(st));
}

/* @bin 0x00522759 the first order buttons of a row (formation +0x82 / ship +0x132 < 0) */
static void order_icons(game_state *st, int32_t row)
{
    frame30(st, px(st) + 0x1B8, row + py(st) + 0x53);
    image_draw(st, S32(st, ORDER_IMAGES), px(st) + 0x1BB, row + py(st) + 0x56);
    frame30(st, px(st) + 0x1F4, row + py(st) + 0x53);
}

static int32_t slide(game_state *st)
{
    /* @bin 0x00521B50 the panel's place and size; while the window slides in (5 - [0x00F53F48] in
     * 1..4) only a black frame shrinking toward it; nothing before. 1: draw the page */
    int32_t x0 = S32(st, ADV_DX);
    int32_t y0 = S32(st, ADV_DY);
    int32_t k = 5 - S32(st, ADV_SLIDE);
    S32(st, ADV_PANEL_X) = x0;
    S32(st, ADV_PANEL_Y) = y0;
    S32(st, ADV_PANEL_W) = 0x28;
    S32(st, ADV_PANEL_H) = 0x1A;
    if (k >= 5) {
        return 0;
    }
    if (k > 0) {
        rect_draw(st, k * 16 + x0, y0 + k * 8, (0x28 - 2 * k) * 16, (0x1A - 2 * k) * 16, 0);
        return 0;
    }
    return 1;
}

void advisor_military_draw(game_state *st)
{
    /* @bin 0x00521B50 page 2: the legions */
    if (!slide(st)) {
        return;
    }
    header(st);
    group(st, 0x33, 0, px(st) + 0x3C, py(st) + 0x0C, ADV_FONT_LARGE);
    group(st, 0x33, 1, px(st) + 0x1A9, py(st) + 0x2B, ADV_FONT_NORMAL);
    group(st, 0x33, 2, px(st) + 0x1A9, py(st) + 0x3A, ADV_FONT_NORMAL);
    group(st, 0x33, 3, px(st) + 0x1EA, py(st) + 0x2B, ADV_FONT_NORMAL);
    group(st, 0x33, 4, px(st) + 0x1EA, py(st) + 0x3A, ADV_FONT_NORMAL);
    group(st, 0x33, 0x11, px(st) + 0x15E, py(st) + 0x2B, ADV_FONT_NORMAL);
    group(st, 0x33, 0x12, px(st) + 0x14A, py(st) + 0x3A, ADV_FONT_NORMAL);
    group(st, 0x8A, 0x24, px(st) + 0xFA, py(st) + 0x3A, ADV_FONT_NORMAL);
    S32(st, 0x00F53DD4u) = legions_count(st);
    war_flags w;
    military_requests_open(st, &w.other, &w.kind2);
    scan_events(st, &w);
    if (S32(st, 0x00F53DD4u) > 0) {
        image_draw(st, S32(st, STATUS_IMAGE), px(st) + 0x32, py(st) + 0x15D);
        /* @bin 0x00521E9B "<n> legions ... <m> ..." (city +0x466F, +0x466C bytes) */
        S32(st, TX) = 0;
        text_count_draw(st, 8, 0x2E, U8(st, city(st, 0x00EA5EDFu)), px(st) + 0x46, py(st) + 0x15C,
                        S32(st, ADV_FONT_NORMAL), 0);
        group(st, 0x33, 7, S32(st, TX) + px(st) + 0x46, py(st) + 0x15C, ADV_FONT_NORMAL);
        text_count_draw(st, 8, 0x30, U8(st, city(st, 0x00EA5EDCu)), S32(st, TX) + px(st) + 0x46, py(st) + 0x15C,
                        S32(st, ADV_FONT_NORMAL), 0);
        image_draw(st, S32(st, STATUS_IMAGE), px(st) + 0x32, py(st) + 0x171);
        status_lines(st, &w, 0x170);
    } else {
        image_draw(st, S32(st, STATUS_IMAGE), px(st) + 0x32, py(st) + 0x167);
        status_lines(st, &w, 0x166);
    }
    if (S32(st, 0x00F53DD4u) <= 0) {
        layout_group_lines(st, 0x33, 0x10, px(st) + 0x40, py(st) + 200, 0x1F0, S32(st, FONT_ROW), 0, dx(st), dy(st));
    } else {
        int32_t row = 0;
        int32_t n = 0;
        do {
            n++;
            int32_t id = player_legion_by_row(st, n);
            int32_t away = formation_has_action_0x12(st, id) != 0;
            uint32_t f = FORMATION_ADDR(id);
            /* @bin 0x00522565 the legion's icon: [0x00F56434] (away) or [0x00F56814] + its kind (+2) */
            int32_t base = away ? S32(st, 0x00F56434u) : S32(st, 0x00F56814u);
            image_draw(st, (int32_t) U8(st, f + 2) + base, px(st) + 0x30, row + py(st) + 0x52);
            group(st, 0x8A, U8(st, f + 2), px(st) + 0x64, row + py(st) + 0x53, FONT_NAME);
            S32(st, TX) = 0;
            layout_number_at(st, U8(st, f + 0x28), 0x40, 0x0057C048u, px(st) + 0x64, row + py(st) + 0x64,
                             S32(st, FONT_ROW), 0, dx(st), dy(st));
            /* @bin 0x00522636 the soldiers' kind (+4): 0xD -> 0x21, 0xB -> 0x23, else 0x22 */
            int16_t kind = S16(st, f + 4);
            int32_t s = kind == 0x0D ? 0x21 : kind == 0x0B ? 0x23 : 0x22;
            group(st, 0x8A, s, S32(st, TX) + px(st) + 0x64, row + py(st) + 0x64, FONT_ROW);
            /* @bin 0x00522720 the morale (+0x2C, i16) / 5 + 0x25 (idiv: toward zero) */
            layout_group_centred(st, 0x8A, S16(st, f + 0x2C) / 5 + 0x25, px(st) + 200, row + py(st) + 0x5B, 0x96,
                                 S32(st, FONT_ROW), 0, dx(st), dy(st));
            if (S16(st, f + 0x82) < 0) {
                order_icons(st, row);
                image_draw(st, S32(st, ORDER_IMAGES) + (U8(st, f + 3) ? 2 : 1), px(st) + 0x1F7, row + py(st) + 0x56);
                frame30(st, px(st) + 0x230, row + py(st) + 0x53);
                /* @bin 0x0052287A away: the second request flag, else the first */
                uint8_t open = away ? w.kind2 : w.other;
                if (open == 0) {
                    U8(st, f + 0x67) = 0;
                    image_draw(st, S32(st, ORDER_IMAGES) + 5, px(st) + 0x233, row + py(st) + 0x56);
                } else if (U8(st, f + 0x67) != 0) {
                    image_draw(st, S32(st, ORDER_IMAGES) + 3, px(st) + 0x233, row + py(st) + 0x56);
                } else {
                    image_draw(st, S32(st, ORDER_IMAGES) + 4, px(st) + 0x233, row + py(st) + 0x56);
                }
            } else {
                group(st, 0x33, 0x1D, px(st) + 0x1F4, row + py(st) + 0x56, FONT_ROW);
            }
            frame30(st, px(st) + 0x168, row + py(st) + 0x53);
            /* @bin 0x005229A5 the health image: (+0x7A (i16) + 10) / 20 from [0x00F5642C] */
            image_draw(st, (S16(st, f + 0x7A) + 10) / 20 + S32(st, 0x00F5642Cu), px(st) + 0x16B, row + py(st) + 0x56);
            row += 0x2C;
        } while (n < S32(st, 0x00F53DD4u));
    }
    image_draw(st, S32(st, 0x00F5643Cu) + 1, px(st) + 0x214, py(st) + 0x15B);
}

void advisor_fleet_draw(game_state *st)
{
    /* @bin 0x00522E10 page 3: the warships */
    if (!slide(st)) {
        return;
    }
    header(st);
    group(st, 0x33, 0x13, px(st) + 0x3C, py(st) + 0x0C, ADV_FONT_LARGE);
    group(st, 0x33, 1, px(st) + 0x1A7, py(st) + 0x2B, ADV_FONT_NORMAL);
    group(st, 0x33, 0x14, px(st) + 0x1A7, py(st) + 0x3A, ADV_FONT_NORMAL);
    group(st, 0x33, 3, px(st) + 0x1E0, py(st) + 0x2B, ADV_FONT_NORMAL);
    group(st, 0x33, 0x15, px(st) + 0x1E0, py(st) + 0x3A, ADV_FONT_NORMAL);
    group(st, 0x33, 0x16, px(st) + 0x13B, py(st) + 0x2B, ADV_FONT_NORMAL);
    group(st, 0x33, 0x17, px(st) + 0x12C, py(st) + 0x3A, ADV_FONT_NORMAL);
    group(st, 0x33, 0x18, px(st) + 0xAA, py(st) + 0x3A, ADV_FONT_NORMAL);
    /* @bin 0x0052306F the warships: city +0x4668 (byte) */
    S32(st, 0x00F53DD0u) = U8(st, city(st, 0x00EA5ED8u));
    war_flags w;
    military_requests_open(st, &w.other, &w.kind2);
    scan_events(st, &w);
    if (S32(st, 0x00F53DD0u) > 0) {
        image_draw(st, S32(st, STATUS_IMAGE), px(st) + 0x32, py(st) + 0x15D);
        S32(st, TX) = 0;
        text_count_draw(st, 8, 0x32, U8(st, city(st, 0x00EA5ED8u)), px(st) + 0x46, py(st) + 0x15C,
                        S32(st, ADV_FONT_NORMAL), 0);
        /* @bin 0x005231BC and the transports (city +0x4664) when there are */
        if (U8(st, city(st, 0x00EA5ED4u)) != 0) {
            group(st, 0x33, 0x20, S32(st, TX) + px(st) + 0x46, py(st) + 0x15C, ADV_FONT_NORMAL);
            text_count_draw(st, 8, 0x3A, U8(st, city(st, 0x00EA5ED4u)), S32(st, TX) + px(st) + 0x46,
                            py(st) + 0x15C, S32(st, ADV_FONT_NORMAL), 0);
        }
        image_draw(st, S32(st, STATUS_IMAGE), px(st) + 0x32, py(st) + 0x171);
        status_lines(st, &w, 0x170);
    } else {
        image_draw(st, S32(st, STATUS_IMAGE), px(st) + 0x32, py(st) + 0x167);
        status_lines(st, &w, 0x166);
    }
    if (S32(st, 0x00F53DD0u) <= 0) {
        layout_group_lines(st, 0x33, 0x19, px(st) + 0x40, py(st) + 200, 0x1F0, S32(st, FONT_ROW), 0, dx(st), dy(st));
    } else {
        int32_t row = 0;
        int32_t n = 0;
        do {
            n++;
            int32_t id = player_warship_by_row(st, n);
            uint32_t f = FIGURE_ADDR(id);
            image_draw(st, S32(st, 0x00F56694u), px(st) + 0x30, row + py(st) + 0x52);
            group(st, 0x33, 0x14, px(st) + 0x64, row + py(st) + 0x53, FONT_NAME);
            layout_group_centred(st, 0xB8, ship_state_text(st, id), px(st) + 0x8C, row + py(st) + 0x5B, 0x96,
                                 S32(st, FONT_ROW), 0, dx(st), dy(st));
            if (S16(st, f + 0x132) < 0) {
                order_icons(st, row);
                int32_t home = ship_at_wharf(st, id);
                image_draw(st, S32(st, ORDER_IMAGES) + (home ? 2 : 1), px(st) + 0x1F7, row + py(st) + 0x56);
                frame30(st, px(st) + 0x230, row + py(st) + 0x53);
                if (w.kind2 == 0) {
                    U8(st, f + 2) = 0;
                    image_draw(st, S32(st, ORDER_IMAGES) + 5, px(st) + 0x233, row + py(st) + 0x56);
                } else if (U8(st, f + 2) != 0) {
                    image_draw(st, S32(st, ORDER_IMAGES) + 3, px(st) + 0x233, row + py(st) + 0x56);
                } else {
                    image_draw(st, S32(st, ORDER_IMAGES) + 4, px(st) + 0x233, row + py(st) + 0x56);
                }
            } else {
                group(st, 0x33, 0x1D, px(st) + 0x1F4, row + py(st) + 0x56, FONT_ROW);
            }
            layout_group_centred(st, 0xB8, ship_health_text(st, id), px(st) + 0x111, row + py(st) + 0x5B, 0x96,
                                 S32(st, FONT_ROW), 0, dx(st), dy(st));
            row += 0x2C;
        } while (n < S32(st, 0x00F53DD0u));
    }
    image_draw(st, S32(st, 0x00F5643Cu), px(st) + 0x214, py(st) + 0x15B);
}
