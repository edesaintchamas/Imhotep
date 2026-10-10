/* Imhotep - window 0x15's input: the scenario editor's empire map, its trade routes (roadmap 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The block 0x004D9CB5..0x004DBE56 of the input dispatch 0x004D8310, and the route methods it calls.
 * A route (50 of them, 0x144 bytes from 0x00776FB0; the current one 0x005D2A30): +0 u8 the spacing
 * of its drawn points, +4 the points (heap, entries of 6: i16 x, y, u8 segment), +8 the waypoints
 * (50 entries of 6: i16 x, y), +0x134 i32 the point count, +0x138 i32 the points' capacity, +0x13C
 * / +0x13E i16 the empire objects at its ends (-1 none), +0x141 u8 the waypoint count, +0x142 u8.
 * Map coordinates: the mouse plus the map's scroll 0x00775910 / 0x0077590C minus its screen
 * origin 0x005D2A18 / 0x005D2A1C. Modes 0x00775928: 0x0E a new route (each release adds a
 * waypoint), 0x0F editing (drag a waypoint, press on the path inserts one, right click deletes
 * one), 1, 2, 4 the object tools.
 *
 * The route's points are computed with the x87 (fsqrt, divisions stored as floats, products and sums
 * at the 53-bit precision the CRT sets, @bin 0x005640EC, __ftol truncation): reproduced on integers
 * with core/city/day_x87.h (x87_sqrt_int: core/city/monument_rating_x87.h; float stores:
 * core/sim/x87_float.h), as the project's rule asks (no floating point in core/ or present/). */
#include "ui.h"

#include "city/day_x87.h"
#include "city/monument_rating_x87.h"
#include "present_layout.h"
#include "sim/call.h"
#include "sim/x87_float.h"
#include "state/ported.h"

#define ROUTE_SIZE 0x144u

static uint32_t route_current(game_state *st)
{
    return G_ROUTES + (uint32_t) S32(st, G_ROUTE_SELECTED) * ROUTE_SIZE;
}

static int16_t wp_x(game_state *st, uint32_t r, int32_t k) { return S16(st, r + 8u + 6u * (uint32_t) k); }
static int16_t wp_y(game_state *st, uint32_t r, int32_t k) { return S16(st, r + 0x0Au + 6u * (uint32_t) k); }

/* ---- the route methods (thiscall on the route) ---- */

int32_t route_last_waypoint(game_state *st, uint32_t r) { return (int32_t) U8(st, r + 0x141) - 1; } /* @bin 0x004DD8D0 */

int32_t route_clear(game_state *st, uint32_t r)
{
    /* @bin 0x004DD8E0 no waypoints, no points */
    U8(st, r + 0x141) = 0;
    S32(st, r + 0x134) = 0;
    return 1;
}

void route_set_flag(game_state *st, uint32_t r, int32_t v) { U8(st, r + 0x142) = (uint8_t) v; } /* @bin 0x004DD900 */
int32_t route_start(game_state *st, uint32_t r) { return S16(st, r + 0x13C); }                /* @bin 0x004DD910 */
int32_t route_end(game_state *st, uint32_t r) { return S16(st, r + 0x13E); }                  /* @bin 0x004DD920 */

static int in_box(int32_t x, int32_t y, int32_t x0, int32_t x1, int32_t y0, int32_t y1)
{
    return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}

int32_t route_hit(game_state *st, uint32_t r, int32_t x, int32_t y, uint8_t *on_waypoint)
{
    /* @bin 0x004DDBD0 the waypoint within 8 of (x, y) (*on_waypoint := 1), else the segment of the
     * point within 8 (*on_waypoint := 0), else -1 */
    int32_t x0 = x - 8, x1 = x + 8, y0 = y - 8, y1 = y + 8;
    int32_t n = U8(st, r + 0x141);
    for (int32_t k = 0; k < n; k++) {
        if (in_box(wp_x(st, r, k), wp_y(st, r, k), x0, x1, y0, y1)) {
            *on_waypoint = 1;
            return k;
        }
    }
    *on_waypoint = 0;
    int32_t cnt = S32(st, r + 0x134);
    uint32_t p = P32(st, r + 4);
    for (int32_t j = 0; j < cnt; j++, p += 6) {
        if (in_box(P16(st, p), P16(st, p + 2), x0, x1, y0, y1)) {
            return P8(st, p + 4);
        }
    }
    return -1;
}

/* ---- the inlined sequences ---- */

static int route_rebuild(game_state *st, uint32_t r)
{
    /* @bin 0x004D9FFF (and its copies) the route's points: per segment max(|dx|, |dy|) + 1 room (the
     * buffer regrown with 50 to spare), then points every `spacing` along each segment; 0 when they
     * do not fit (the count is then left at the room), else the count is stored */
    int32_t n = U8(st, r + 0x141);
    int32_t size = 0;
    for (int32_t k = 1; k < n; k++) {
        int32_t a = wp_x(st, r, k - 1), b = wp_x(st, r, k);
        int32_t ax = a - b > b - a ? a - b : b - a;
        a = wp_y(st, r, k - 1);
        b = wp_y(st, r, k);
        int32_t ay = a - b > b - a ? a - b : b - a;
        size += (ax > ay ? ax : ay) + 1;
    }
    int32_t cap = S32(st, r + 0x138);
    if (size > cap && cap < size + 0x32) {
        if (P32(st, r + 4) != 0) {
            state_dealloc(st, P32(st, r + 4));
            S32(st, r + 4) = 0;
        }
        S32(st, r + 4) = (int32_t) state_alloc(st, (uint32_t) (size + 0x32) * 6u);
        S32(st, r + 0x138) = size + 0x32;
    }
    S32(st, r + 0x134) = size;
    int32_t idx = 0;
    for (int32_t k = 1; k < U8(st, r + 0x141); k++) {
        if (idx > S32(st, r + 0x134)) {
            return 0;
        }
        int32_t x0 = wp_x(st, r, k - 1), y0 = wp_y(st, r, k - 1);
        int32_t dx = wp_x(st, r, k) - x0, dy = wp_y(st, r, k) - y0;
        int32_t count = 0;
        /* @bin 0x004DA0F8 fild dy, dx; dx * dx + dy * dy (exact); fsqrt (53 bits), fstp to a float */
        uint64_t sq = (uint64_t) ((int64_t) dx * dx) + (uint64_t) ((int64_t) dy * dy);
        uint32_t len = x87_to_float_bits(x87_sqrt_int(sq));
        x87_val flen = x87_from_float(len);
        if ((len & 0x7FFFFFFFu) != 0 && U8(st, r) != 0) {
            /* @bin 0x004DA13E dy / len and dx / len (53 bits), stored as floats */
            uint32_t fy = x87_to_float_bits(x87_divv(x87_from_int(dy), flen));
            uint32_t fx = x87_to_float_bits(x87_divv(x87_from_int(dx), flen));
            if (x87_cmp(x87_from_float(0), flen) <= 0) {
                /* @bin 0x004DA173 x0 kept as fild; y0 stored as a float; for t = 0, spacing, ... while t
                 * <= len: (t * fx + x0, t * fy + y0) at 53 bits, __ftol, the low 16 bits */
                x87_val x0v = x87_from_int(x0);
                x87_val y0f = x87_from_float(x87_to_float_bits(x87_from_int(y0)));
                uint32_t p = (uint32_t) idx * 6u;
                int32_t t = 0;
                do {
                    x87_val tv = x87_from_int(t);
                    x87_val vx = x87_add(x87_mul(tv, x87_from_float(fx)), x0v);
                    P16(st, P32(st, r + 4) + p) = (uint16_t) x87_ftol(vx);
                    x87_val vy = x87_add(x87_mul(tv, x87_from_float(fy)), y0f);
                    P16(st, P32(st, r + 4) + p + 2) = (uint16_t) x87_ftol(vy);
                    P8(st, P32(st, r + 4) + p + 4) = (uint8_t) (k - 1);
                    t += U8(st, r);
                    count++;
                    p += 6;
                } while (x87_cmp(x87_from_int(t), flen) <= 0);
            }
        }
        idx += count;
    }
    S32(st, r + 0x134) = idx;
    return 1;
}

static void route_find_ends(game_state *st, uint32_t r)
{
    /* @bin 0x004DA21D the empire objects (kind 1, in use) whose picture (sizes 0x010D6A0C) holds the
     * first and the last waypoint: +0x13C, +0x13E (-1 none; the last object found wins) */
    S16(st, r + 0x13E) = -1;
    S16(st, r + 0x13C) = -1;
    uint8_t n = U8(st, r + 0x141);
    if (n == 0) {
        return;
    }
    for (int32_t j = 0; j < 200; j++) {
        uint32_t o = G_EMPIRE_OBJECTS + (uint32_t) j * 0x62u;
        if (U8(st, o + 1) == 0 || U8(st, o) != 1) {
            continue;
        }
        int32_t img = U16(st, o + 0x0C);
        int32_t lim = S32(st, G_IMAGE_COUNT);
        uint32_t iw = img < lim ? (uint32_t) img : 0u;
        uint32_t ih = img < lim ? (uint32_t) img : 0u;
        int32_t ox = S16(st, o + 4), oy = S16(st, o + 6);
        int32_t x1 = S16(st, G_U0D6A0C + iw * 64u) + ox;
        int32_t y1 = S16(st, G_IMGS_H + ih * 64u) + oy;
        if (in_box(wp_x(st, r, 0), wp_y(st, r, 0), ox, x1, oy, y1)) {
            S16(st, r + 0x13C) = (int16_t) j;
        }
        if (in_box(S16(st, r + 2u + 6u * n), S16(st, r + 4u + 6u * n), ox, x1, oy, y1)) {
            S16(st, r + 0x13E) = (int16_t) j;
        }
    }
}

static int in_map(game_state *st)
{
    int32_t mx = S32(st, G_MOUSE_X), left = S32(st, G_MAP_X);
    if (mx < left || mx >= S32(st, G_UE38D9C) - 2 * left) {
        return 0;
    }
    int32_t my = S32(st, G_MOUSE_Y), top = S32(st, G_MAP_Y);
    return my >= top && my < S32(st, G_DRAW_H) - S32(st, G_MAP_BOTTOM) - top;
}

static int route_insert(game_state *st, uint32_t r, int32_t n, int32_t x, int32_t y)
{
    /* a waypoint inserted at n (the later ones moved up); 0 when the route has 50 */
    uint8_t cnt = U8(st, r + 0x141);
    if (cnt >= 0x32) {
        return 0;
    }
    for (int32_t i = (int32_t) cnt - 1; i >= n; i--) {
        uint32_t s = r + 8u + 6u * (uint32_t) i;
        S32(st, s + 6) = S32(st, s);
        S16(st, s + 10) = S16(st, s + 4);
    }
    S16(st, r + 8u + 6u * (uint32_t) n) = (int16_t) x;
    S16(st, r + 0x0Au + 6u * (uint32_t) n) = (int16_t) y;
    U8(st, r + 0x141) = (uint8_t) (cnt + 1);
    return 1;
}

static void set_wp(game_state *st, uint32_t r, int32_t k, int32_t x, int32_t y)
{
    S16(st, r + 8u + 6u * (uint32_t) k) = (int16_t) x;
    S16(st, r + 0x0Au + 6u * (uint32_t) k) = (int16_t) y;
}

static void rebuild_and_close(game_state *st, uint32_t r)
{
    /* the new-route copies: on success the route's own update 0x00407F90 */
    if (route_rebuild(st, r)) {
        GAME_CALLT0(st, r, FN_ROUTE_FIND_CITIES);
    }
}

static int find_in_route(game_state *st, uint32_t r, int32_t x0, int32_t x1, int32_t y0, int32_t y1)
{
    /* @bin 0x004DA700 a waypoint (its index) or a point (its segment) of route r in the box, else -1 */
    for (int32_t k = 0; k < U8(st, r + 0x141); k++) {
        if (in_box(wp_x(st, r, k), wp_y(st, r, k), x0, x1, y0, y1)) {
            return k;
        }
    }
    int32_t cnt = S32(st, r + 0x134);
    if (cnt > 0) {
        uint32_t p = P32(st, r + 4);
        for (int32_t j = 0; j < cnt; j++, p += 6) {
            if (in_box(P16(st, p), P16(st, p + 2), x0, x1, y0, y1)) {
                return P8(st, p + 4);
            }
        }
    }
    return -1;
}

static int32_t rect_input(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, uint32_t slot)
{
    int32_t hit = rect_hit(st, x, y, table, count);
    S32(st, slot) = rect_highlight(st, x, y, table, count, hit);
    return GAME_CALL(st, FN_BUTTONS_RECT_CLICK, x, y, (int32_t) table, count);
}

static int32_t rects(game_state *st, uint32_t table, int32_t count)
{
    return GAME_CALL(st, FN_BUTTONS_RECT_CLICK, 0xDF, S32(st, G_SCREEN_H) - 0x72, (int32_t) table, count);
}

static void edit_mode(game_state *st, int32_t mx, int32_t my)
{
    /* @bin 0x004D9E28 mode 0x0F */
    if (rects(st, G_EMPIRE_EDITOR_RECTS_D, 4) != 0) {
        return;
    }
    uint32_t r = route_current(st);
    int32_t end0 = S16(st, r + 0x13E), start0 = S16(st, r + 0x13C);
    int at_end = 0; /* 1: 0x004DB01E (0x005D2A2C cleared), else 0x004DB028 */
    if (U8(st, G_LEFT_PRESSED) != 0) {
        int32_t x0 = mx - 8, x1 = mx + 8, y0 = my - 8, y1 = my + 8;
        int on_wp = 0;
        int32_t hit = -1;
        int32_t n = U8(st, r + 0x141);
        for (int32_t k = 0; k < n; k++) {
            if (in_box(wp_x(st, r, k), wp_y(st, r, k), x0, x1, y0, y1)) {
                on_wp = 1;
                hit = k;
                break;
            }
        }
        if (!on_wp && S32(st, r + 0x134) > 0) {
            uint32_t p = P32(st, r + 4);
            for (int32_t j = 0; j < S32(st, r + 0x134); j++, p += 6) {
                if (in_box(P16(st, p), P16(st, p + 2), x0, x1, y0, y1)) {
                    hit = P8(st, p + 4);
                    break;
                }
            }
        }
        S32(st, G_ROUTE_DRAG) = hit;
        if (hit < 0) {
            /* @bin 0x004DA6F7 a press off the route selects the route under the mouse */
            for (int32_t q = 0; q < 50; q++) {
                int32_t h = find_in_route(st, G_ROUTES + (uint32_t) q * ROUTE_SIZE, x0, x1, y0, y1);
                S32(st, G_ROUTE_DRAG) = h;
                if (h >= 0) {
                    S32(st, G_ROUTE_DRAG) = -1;
                    S32(st, G_ROUTE_SELECTED) = q;
                    U8(st, G_REDRAW) = 1;
                    break;
                }
            }
        } else if (on_wp) {
            /* @bin 0x004DA340 the waypoint follows the mouse */
            at_end = 1;
            if (in_map(st) && hit < U8(st, r + 0x141)) {
                set_wp(st, r, hit, mx, my);
                if (route_rebuild(st, r)) {
                    route_find_ends(st, r);
                }
            }
        } else {
            /* @bin 0x004D9F6D a press on the path inserts a waypoint after the segment */
            at_end = 1;
            int32_t ins = hit + 1 < 0 ? 0 : hit + 1;
            if (!route_insert(st, r, ins, mx, my) || !route_rebuild(st, r)) {
                S32(st, G_ROUTE_DRAG) = -1;
            } else {
                route_find_ends(st, r);
                S32(st, G_ROUTE_DRAG) = ins;
            }
        }
    } else if (U8(st, G_LEFT_RELEASED) != 0) {
        /* @bin 0x004DA7E2 the dragged waypoint is dropped */
        int32_t h = S32(st, G_ROUTE_DRAG);
        if (h >= 0) {
            if (in_map(st) && h < U8(st, r + 0x141)) {
                set_wp(st, r, h, mx, my);
                if (route_rebuild(st, r)) {
                    route_find_ends(st, r);
                }
            }
            S32(st, G_U5D2A2C) = 0;
        }
        S32(st, G_ROUTE_DRAG) = -1;
    } else if (U8(st, G_LEFT_HELD) != 0) {
        /* @bin 0x004DABC7 dragging */
        int32_t h = S32(st, G_ROUTE_DRAG);
        if (h >= 0) {
            at_end = 1;
            if (in_map(st) && h < U8(st, r + 0x141)) {
                set_wp(st, r, h, mx, my);
                if (route_rebuild(st, r)) {
                    route_find_ends(st, r);
                }
            }
        }
    } else if (U8(st, G_RIGHT_RELEASED) != 0) {
        /* @bin 0x004DAF9D a right click on a waypoint deletes it (0x004DD930); the last one clears
         * the route */
        uint8_t on_wp = 0;
        int32_t h = route_hit(st, r, mx, my, &on_wp);
        if (h >= 0 && on_wp != 0) {
            GAME_CALLT(st, route_current(st), 0x004DD930u, h);
            if (route_last_waypoint(st, route_current(st)) <= 0) {
                route_set_flag(st, route_current(st), 0);
                route_clear(st, route_current(st));
                at_end = 1;
            }
        }
    }
    if (at_end) {
        S32(st, G_U5D2A2C) = 0;
    }
    /* @bin 0x004DB028 a change of the route's ends redraws */
    if (start0 != route_start(st, route_current(st)) || end0 != route_end(st, route_current(st))) {
        U8(st, G_REDRAW) = 1;
    }
}

static void new_route_mode(game_state *st, int32_t mx, int32_t my, int32_t start0, int32_t end0)
{
    /* @bin 0x004DB079 mode 0x0E: a release moves the last waypoint to the mouse (the first one sets
     * the route's flag +0x142) and adds a new one there; between releases the last one follows the
     * mouse; right click ends (mode 0x0F) */
    if (U8(st, G_LEFT_RELEASED) != 0) {
        uint32_t r = route_current(st);
        int32_t c = (int32_t) U8(st, r + 0x141) - 1;
        if (c < 0) {
            if (!in_map(st)) {
                return;
            }
            U8(st, r + 0x142) = 1;
            if (route_insert(st, r, U8(st, r + 0x141), mx, my)) {
                rebuild_and_close(st, r);
            }
        } else if (in_map(st) && c < U8(st, r + 0x141)) {
            set_wp(st, r, c, mx, my);
            rebuild_and_close(st, r);
        }
        r = route_current(st);
        if (route_insert(st, r, U8(st, r + 0x141), mx, my)) {
            rebuild_and_close(st, r);
        }
        S32(st, G_U5D2A2C) = 0;
    } else if (U8(st, G_RIGHT_RELEASED) != 0) {
        S32(st, G_EDITOR_MODE) = 0x0F;
    } else {
        uint32_t r = route_current(st);
        int32_t c = (int32_t) U8(st, r + 0x141) - 1;
        if (in_map(st) && c >= 0 && c < U8(st, r + 0x141)) {
            set_wp(st, r, c, mx, my);
            rebuild_and_close(st, r);
        }
        S32(st, G_U5D2A2C) = 0;
    }
    uint32_t r = route_current(st);
    if (start0 != S16(st, r + 0x13C) || end0 != S16(st, r + 0x13E)) {
        U8(st, G_REDRAW) = 1;
    }
}

static void object_pick(game_state *st, int32_t keep)
{
    /* the object under the mouse (0x00407620 / 0x00407870) becomes the current one 0x0077591C */
    GAME_CALL(st, 0x00407620u, keep ? S32(st, G_EDITOR_OBJECT) : S32(st, G_U775918), keep);
    int32_t o = GAME_CALL0(st, 0x00407870u);
    if (o == 0) {
        return;
    }
    if (!keep) {
        S32(st, G_EDITOR_MODE) = 2;
    }
    S32(st, G_EDITOR_OBJECT) = o - 1;
    S32(st, G_OBJECT_IMAGE) = U16(st, G_EMPIRE_OBJECT_IMAGE + (uint32_t) (o - 1) * 0x62u);
}

void empire_editor_input(game_state *st, int32_t mouse_x, int32_t mouse_y)
{
    /* @bin 0x004D9CB5 window 0x15 (mouse_x / mouse_y: the cursor as 0x004D8310 read it on entry) */
    int32_t mx = S32(st, G_SCROLL_X) - S32(st, G_MAP_X) + mouse_x;
    int32_t my = S32(st, G_SCROLL_Y) - S32(st, G_MAP_Y) + mouse_y;
    uint32_t r0 = route_current(st);
    int32_t start0 = S16(st, r0 + 0x13C), end0 = S16(st, r0 + 0x13E);
    GAME_CALL0(st, 0x00442500u);
    GAME_CALL0(st, FN_EMPIRE_OBJECTS_SIZES);
    GAME_CALL(st, FN_EMPIRE_SCROLL_KEY, GAME_CALL0(st, FN_SCROLL_DIRECTION));
    if (rect_input(st, 0x0F, S32(st, G_SCREEN_H) - 0x74, G_EMPIRE_EDITOR_RECTS, 8, G_HOVER) != 0) {
        return;
    }
    if (S32(st, G_HOVER) == 0) {
        if (rect_input(st, 0xDF, S32(st, G_SCREEN_H) - 0x72, G_EMPIRE_EDITOR_RECTS_B, 1, G_HOVER) != 0) {
            return;
        }
        if (S32(st, G_HOVER) == 1) {
            S32(st, G_HOVER) = 0x0A;
        }
    }
    int32_t mode = S32(st, G_EDITOR_MODE);
    if (mode == 0x0E) {
        new_route_mode(st, mx, my, start0, end0);
    } else if (mode == 0x0F) {
        edit_mode(st, mx, my);
    } else if (mode == 1) {
        /* @bin 0x004DB108 */
        if (S32(st, G_PICTURE_KIND) == 0 &&
            GAME_CALL(st, FN_BUTTONS_EDITOR_CLICK, 0xDF, S32(st, G_SCREEN_H) - 0x72, G_EMPIRE_EDITOR_BUTTONS, 5) != 0) {
            return;
        }
        if (rects(st, G_EMPIRE_EDITOR_RECTS_C, 3) != 0) {
            return;
        }
        object_pick(st, 0);
    } else if (mode == 2) {
        /* @bin 0x004DB1A7 */
        int32_t wait = S32(st, G_UE38DDC);
        if (wait > 0) {
            S32(st, G_UE38DDC) = wait - 1;
            return;
        }
        if (U8(st, G_EMPIRE_OBJECTS + U32(st, G_EDITOR_OBJECT) * 0x62u) == 0 &&
            GAME_CALL(st, FN_BUTTONS_EDITOR_CLICK, 0xDF, S32(st, G_SCREEN_H) - 0x72, G_EMPIRE_EDITOR_BUTTONS, 5) != 0) {
            return;
        }
        int32_t o = S32(st, G_EDITOR_OBJECT);
        uint32_t e = G_EMPIRE_OBJECTS + (uint32_t) o * 0x62u;
        uint8_t kind = U8(st, e);
        if (kind == 1) {
            if (rects(st, G_EMPIRE_EDITOR_RECTS_E, 6) != 0) {
                return;
            }
            if (U8(st, e + 0x18) == 0) {
                if (rects(st, G_TRADE_KIND0_SLOTS, 0x0E) != 0) {
                    return;
                }
            } else if (rects(st, G_TRADE_SOLD_SLOTS, 0x10) != 0) {
                return;
            }
            GAME_CALL(st, 0x00442A40u, o);
            GAME_CALL0(st, 0x00442EF0u);
            GAME_CALL0(st, 0x00442F80u);
        } else if (kind == 3) {
            if (rects(st, G_EMPIRE_EDITOR_RECTS_G, 2) != 0) {
                return;
            }
        } else if (kind == 2) {
            if (rects(st, G_EMPIRE_EDITOR_RECTS_F, 1) != 0) {
                return;
            }
        }
        object_pick(st, 1);
    } else if (mode == 4) {
        rects(st, G_EMPIRE_EDITOR_RECTS_B, 1);
    }
}

/* ---- port table ---- */

static uint32_t this_of(const int32_t *a) { return (uint32_t) a[PORT_THIS_ARG]; }
static int32_t run_last(game_state *st, const int32_t *a) { return route_last_waypoint(st, this_of(a)); }
static int32_t run_clear(game_state *st, const int32_t *a) { return route_clear(st, this_of(a)); }
static int32_t run_flag(game_state *st, const int32_t *a) { route_set_flag(st, this_of(a), a[0]); return 0; }
static int32_t run_start(game_state *st, const int32_t *a) { return route_start(st, this_of(a)); }
static int32_t run_end(game_state *st, const int32_t *a) { return route_end(st, this_of(a)); }

const ported_function ui_empire_editor_ports[] = {
    {0x004DD8D0u, "route_last_waypoint", 0, 32, 0, run_last, 0},
    {0x004DD8E0u, "route_clear", 0, 8, 0, run_clear, 0},
    {0x004DD900u, "route_set_flag", 1, 0, 0, run_flag, 0},
    {0x004DD910u, "route_start", 0, 32, 0, run_start, 0},
    {0x004DD920u, "route_end", 0, 32, 0, run_end, 0},
    {0x004DDBD0u, "route_hit", 3, 32, 0, NULL, 0}, /* writes its caller's stack: ported inside 0x004D8310 */
};
const int ui_empire_editor_ports_count = sizeof(ui_empire_editor_ports) / sizeof(ui_empire_editor_ports[0]);
