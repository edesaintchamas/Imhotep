/* Imhotep - the figures' route searches (package figa): breadth-first searches over the tile
 * distance grid 0x00EED760, one per kind of walker.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "figure/actions_a.h"

#include "sim/call.h"
#include "state/records.h"

#define DISTANCES 0x00EED760u /* i32 per tile: 0 not reached, else 1 + the steps from the start */
#define TERRAIN 0x00EE0C50u   /* i8 per tile: the walking grid (0 .. 2 roads and land, < 0 blocked) */
#define QUEUE 0x00F203A0u     /* i32 ring of 0xCB10 tile offsets */
#define QUEUE_LAST 0xCB0F
#define QUEUE_HEAD 0x00EAE00Cu
#define QUEUE_TAIL 0x00EAE008u
#define SEARCH_START 0x00EAE004u
#define SEARCHES 0x00EADFD4u /* searches made */

/* may the search step from tile `from` onto tile `to` (not reached yet; v its walking value) */
typedef int (*route_open_fn)(game_state *st, int32_t to, int8_t v);

static int32_t dist(game_state *st, int32_t tile)
{
    return S32(st, DISTANCES + 4u * (uint32_t) tile);
}

/* @bin 0x00519840 .. the common start: the grid cleared (0x0040F8F0), the start tile at 1 */
static void route_begin(game_state *st, int32_t x, int32_t y)
{
    GAME_CALL(st, 0x0040F8F0u, (int32_t) DISTANCES, 0x32C40, 0);
    S32(st, SEARCH_START) = (int32_t) (U32(st, 0x00BCA2D8u) + 228u * (uint32_t) y + (uint32_t) x);
    S32(st, SEARCHES) += 1;
    S32(st, DISTANCES + 4u * (uint32_t) S32(st, SEARCH_START)) = 1;
    S32(st, QUEUE) = S32(st, SEARCH_START);
    S32(st, QUEUE_HEAD) = 0;
    S32(st, QUEUE_TAIL) = 1;
}

static void route_visit(game_state *st, int32_t to, int32_t d, route_open_fn open)
{
    if (dist(st, to) != 0 || !open(st, to, S8(st, TERRAIN + (uint32_t) to))) {
        return;
    }
    S32(st, DISTANCES + 4u * (uint32_t) to) = d;
    int32_t tail = S32(st, QUEUE_TAIL);
    S32(st, QUEUE + 4u * (uint32_t) tail) = to;
    tail++;
    S32(st, QUEUE_TAIL) = tail;
    if (tail > QUEUE_LAST) {
        S32(st, QUEUE_TAIL) = 0;
    }
}

/* expand the queue's head (north, east, south, west); returns 0 when the queue is empty */
static void route_expand(game_state *st, int32_t tile, route_open_fn open)
{
    int32_t d = dist(st, tile) + 1;
    route_visit(st, tile - 228, d, open);
    route_visit(st, tile + 1, d, open);
    route_visit(st, tile + 228, d, open);
    route_visit(st, tile - 1, d, open);
}

static int route_next(game_state *st)
{
    int32_t head = S32(st, QUEUE_HEAD) + 1;
    S32(st, QUEUE_HEAD) = head;
    if (head > QUEUE_LAST) {
        head = 0;
        S32(st, QUEUE_HEAD) = 0;
    }
    return head != S32(st, QUEUE_TAIL);
}

/* search until the target tile is taken from the queue; 1 when it was reached */
static int32_t route_to_tile(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty, route_open_fn open)
{
    route_begin(st, x, y);
    int32_t target = (int32_t) (U32(st, 0x00BCA2D8u) + 228u * (uint32_t) ty + (uint32_t) tx);
    do {
        int32_t tile = S32(st, QUEUE + 4u * (uint32_t) S32(st, QUEUE_HEAD));
        if (tile == target) {
            break;
        }
        route_expand(st, tile, open);
    } while (route_next(st));
    return dist(st, target) != 0;
}

/* search until one of the n grid offsets at `list` is taken from the queue: its index (1 ..) */
static int32_t route_to_list(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n, route_open_fn open)
{
    route_begin(st, x, y);
    do {
        int32_t tile = S32(st, QUEUE + 4u * (uint32_t) S32(st, QUEUE_HEAD));
        for (int32_t i = n - 1; i >= 0; i--) {
            if (S32(st, list + 4u * (uint32_t) i) == tile) {
                return i + 1;
            }
        }
        route_expand(st, tile, open);
    } while (route_next(st));
    return 0;
}

/* @bin 0x00519840, 0x00519E30 walking values 0 .. 2 */
static int open_walk(game_state *st, int32_t to, int8_t v)
{
    (void) st;
    (void) to;
    return v >= 0 && v <= 2;
}

int32_t route_free_tile(game_state *st, int32_t tile)
{
    /* @bin 0x0051AFF0 0 when one of the tile's figures (list 0x00B350DC, +0x08) is in use with
     * +0x0D 1 and action 5 */
    int16_t o = S16(st, 0x00B350DCu + 2u * (uint32_t) tile);
    if (o <= 0) {
        return 1;
    }
    for (;;) {
        uint32_t f = FIGURE_ADDR(o);
        if (U8(st, f + 0x0D) == 1 && U8(st, f + 0x38) == 5) {
            return 0;
        }
        int16_t next = S16(st, f + 0x08);
        if (next == o) {
            return 1;
        }
        o = next;
        if (o == 0) {
            return 1;
        }
    }
}

/* @bin 0x00518880 a free tile (0x0051AFF0) of walking value 0 .. 4 but 3, or -3, or -1 with terrain
 * flag 0x1000 */
static int open_walk_free(game_state *st, int32_t to, int8_t v)
{
    if (!route_free_tile(st, to)) {
        return 0;
    }
    if (!(v >= 0 || v == -3 || (v == -1 && (U32(st, 0x00AF598Cu + 4u * (uint32_t) to) & 0x1000u)))) {
        return 0;
    }
    return v <= 4 && v != 3;
}

int32_t route_walk_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    return route_to_tile(st, x, y, tx, ty, open_walk); /* @bin 0x00519840 */
}

int32_t route_walk_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n)
{
    return route_to_list(st, x, y, list, n, open_walk); /* @bin 0x00519E30 */
}

int32_t route_walk_free_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    return route_to_tile(st, x, y, tx, ty, open_walk_free); /* @bin 0x00518880 */
}

/* @bin 0x00517F20, 0x00518B30 as 0x00518880, value 3 allowed */
static int open_walk_free3(game_state *st, int32_t to, int8_t v)
{
    if (!route_free_tile(st, to)) {
        return 0;
    }
    if (!(v >= 0 || v == -3 || (v == -1 && (U32(st, 0x00AF598Cu + 4u * (uint32_t) to) & 0x1000u)))) {
        return 0;
    }
    return v <= 4;
}

int32_t route_any_free_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    return route_to_tile(st, x, y, tx, ty, open_walk_free3); /* @bin 0x00517F20 */
}

int32_t route_any_free_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n)
{
    return route_to_list(st, x, y, list, n, open_walk_free3); /* @bin 0x00518B30 */
}

/* one neighbour of 0x005190A0: returns the index + 1 of the marked tile found, else 0 */
static int32_t marked_visit(game_state *st, int32_t to, int32_t d, int west)
{
    if (dist(st, to) > 0 || !open_walk_free(st, to, S8(st, TERRAIN + (uint32_t) to))) {
        return 0;
    }
    int32_t r = -1 - dist(st, to);
    if (!west) {
        S32(st, DISTANCES + 4u * (uint32_t) to) = d;
    }
    if (r >= 0) {
        return r + 1;
    }
    if (west) {
        S32(st, DISTANCES + 4u * (uint32_t) to) = d;
    }
    int32_t tail = S32(st, QUEUE_TAIL);
    S32(st, QUEUE + 4u * (uint32_t) tail) = to;
    tail++;
    S32(st, QUEUE_TAIL) = tail;
    if (tail > QUEUE_LAST) {
        S32(st, QUEUE_TAIL) = 0;
    }
    return 0;
}

int32_t route_walk_free_to_marked(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n)
{
    /* @bin 0x005190A0 the n grid offsets at `list` are marked -1, -2, .. in the grid; the search
     * (0x00518880's tiles) stops at the first marked tile it steps on: its index (1 ..); the west
     * neighbour keeps its mark when found */
    GAME_CALL(st, 0x0040F8F0u, (int32_t) DISTANCES, 0x32C40, 0);
    if (n <= 0) {
        return 0;
    }
    int32_t mark = -1;
    for (int32_t i = 0; i < n; i++) {
        S32(st, DISTANCES + 4u * U32(st, list + 4u * (uint32_t) i)) = mark--;
    }
    S32(st, SEARCH_START) = (int32_t) (U32(st, 0x00BCA2D8u) + 228u * (uint32_t) y + (uint32_t) x);
    S32(st, SEARCHES) += 1;
    S32(st, DISTANCES + 4u * (uint32_t) S32(st, SEARCH_START)) = 1;
    S32(st, QUEUE) = S32(st, SEARCH_START);
    S32(st, QUEUE_HEAD) = 0;
    S32(st, QUEUE_TAIL) = 1;
    do {
        int32_t tile = S32(st, QUEUE + 4u * (uint32_t) S32(st, QUEUE_HEAD));
        int32_t d = dist(st, tile) + 1;
        int32_t r;
        if ((r = marked_visit(st, tile - 228, d, 0)) != 0 || (r = marked_visit(st, tile + 1, d, 0)) != 0 ||
            (r = marked_visit(st, tile + 228, d, 0)) != 0 || (r = marked_visit(st, tile - 1, d, 1)) != 0) {
            return r;
        }
    } while (route_next(st));
    return 0;
}

int32_t route_store(game_state *st, int32_t mode, int32_t slot, int32_t x0, int32_t y0, int32_t tx, int32_t ty)
{
    /* @bin 0x0051B400 the route from the search grid: from the target (tx, ty) down the distances
     * to the start (x0, y0), 8 directions (mode 8) or 4; on ties the direction of the start
     * (0x0051CBA0), never straight back. The steps (reversed, at most 499) go to the route table
     * 0x00C8AD38 (500 per slot) through 0x00F533C8; returns their number (0: no route) */
    static const int32_t step_off[8] = {-228, -227, 1, 229, 228, 227, -1, -229};
    static const int32_t step_x[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int32_t step_y[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
    int32_t cur = (int32_t) (U32(st, 0x00BCA2D8u) + 228u * (uint32_t) ty + (uint32_t) tx);
    S32(st, SEARCH_START) = cur;
    int32_t best = dist(st, cur);
    if (best <= 0) {
        return 0;
    }
    int32_t step = (mode != 8) + 1;
    int32_t back = -1, n = 0;
    if (best > 1) {
        do {
            best = dist(st, cur);
            int32_t prefer = (uint8_t) GAME_CALL(st, 0x0051CBA0u, tx, ty, x0, y0);
            int32_t dir = -1;
            for (int32_t k = 0; k < 8; k += step) {
                if (k == back) {
                    continue;
                }
                int32_t v = dist(st, cur + S32(st, 0x005F80FCu + 4u * (uint32_t) k));
                if (v == 0) {
                    continue;
                }
                if (v < best || (v == best && (k == prefer || dir == -1))) {
                    best = v;
                    dir = k;
                }
            }
            if (dir == -1) {
                return 0;
            }
            cur += step_off[dir];
            tx += step_x[dir];
            ty += step_y[dir];
            back = dir + 4;
            if (back >= 8) {
                back -= 8;
            }
            U8(st, 0x00F533C8u + (uint32_t) n) = (uint8_t) back;
            n++;
            if (n >= 500) {
                return 0;
            }
        } while (best > 1);
    }
    for (int32_t i = 0; i < n; i++) {
        U8(st, 0x00C8AD38u + 500u * (uint32_t) slot + (uint32_t) i) = U8(st, 0x00F533C7u + (uint32_t) (n - i));
    }
    return n;
}

void road_wear_up(game_state *st, int32_t kind, int32_t tile)
{
    /* @bin 0x0047E250 the 3-bit field `kind` of the tile's dword in 0x00978D9C goes up to 7; the
     * original clears it with (7 << s) ^ 0xFFFF, which keeps the high half's other bits set */
    uint32_t a = 0x00978D9Cu + 4u * (uint32_t) tile;
    uint32_t s = (uint32_t) (kind * 3) & 31u;
    uint32_t w = U32(st, a);
    uint32_t v = (w >> s) & 7u;
    if (v >= 7) {
        return;
    }
    U32(st, a) = (((7u << s) ^ 0xFFFFu) & w) | ((v + 1) << s);
}

/* @bin 0x00518400, 0x00518DD0 free tiles of walking value 0, 2, 4 or 6, -3, or -1 with terrain
 * 0x1000 (not 1, 3, 5) */
static int open_walk_even(game_state *st, int32_t to, int8_t v)
{
    if (!route_free_tile(st, to)) {
        return 0;
    }
    if (!(v >= 0 || v == -3 || (v == -1 && (U32(st, 0x00AF598Cu + 4u * (uint32_t) to) & 0x1000u)))) {
        return 0;
    }
    return v != 3 && v != 1 && (v <= 4 || v == 6);
}

/* @bin 0x005186D0 every tile */
static int open_any(game_state *st, int32_t to, int8_t v)
{
    (void) st;
    (void) to;
    (void) v;
    return 1;
}

/* @bin 0x00519390 free tiles of walking value 0 and up but 3 */
static int open_walk_not3(game_state *st, int32_t to, int8_t v)
{
    return route_free_tile(st, to) && v >= 0 && v != 3;
}

/* @bin 0x005195E0 free tiles of walking value 0 .. 2 or 5 */
static int open_walk_or5(game_state *st, int32_t to, int8_t v)
{
    return route_free_tile(st, to) && v >= 0 && (v <= 2 || v == 5);
}

/* @bin 0x00519BE0 walking value 0 and up, or -1 on terrain 0x10004 without a building */
static int open_walk_water(game_state *st, int32_t to, int8_t v)
{
    return v >= 0 || (v == -1 && (U32(st, 0x00AF598Cu + 4u * (uint32_t) to) & 0x10004u) &&
                      S16(st, 0x00B4E6FCu + 2u * (uint32_t) to) == 0);
}

/* @bin 0x0051A030, 0x0051A220 the grid 0x00EBAB20 at 0 .. 2 */
static int open_boat(game_state *st, int32_t to, int8_t v)
{
    (void) v;
    int8_t w = S8(st, 0x00EBAB20u + (uint32_t) to);
    return w >= 0 && w <= 2;
}

int32_t route_even_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    return route_to_tile(st, x, y, tx, ty, open_walk_even); /* @bin 0x00518400 */
}

int32_t route_even_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n)
{
    return route_to_list(st, x, y, list, n, open_walk_even); /* @bin 0x00518DD0 */
}

int32_t route_any_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    return route_to_tile(st, x, y, tx, ty, open_any); /* @bin 0x005186D0 */
}

int32_t route_not3_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    return route_to_tile(st, x, y, tx, ty, open_walk_not3); /* @bin 0x00519390 */
}

int32_t route_or5_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    return route_to_tile(st, x, y, tx, ty, open_walk_or5); /* @bin 0x005195E0 */
}

int32_t route_water_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    return route_to_tile(st, x, y, tx, ty, open_walk_water); /* @bin 0x00519BE0 */
}

int32_t route_boat_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    return route_to_tile(st, x, y, tx, ty, open_boat); /* @bin 0x0051A030 */
}

int32_t route_boat_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n)
{
    return route_to_list(st, x, y, list, n, open_boat); /* @bin 0x0051A220 */
}

/* @bin 0x0051D420, 0x0051D6D0 as 0x00517F20 but values 0 .. 4 or 6 */
static int open_walk_free6(game_state *st, int32_t to, int8_t v)
{
    if (!route_free_tile(st, to)) {
        return 0;
    }
    if (!(v >= 0 || v == -3 || (v == -1 && (U32(st, 0x00AF598Cu + 4u * (uint32_t) to) & 0x1000u)))) {
        return 0;
    }
    return v <= 4 || v == 6;
}

/* @bin 0x0051B220 the grid 0x00EC7630 at 0 and up */
static int open_ec7630(game_state *st, int32_t to, int8_t v)
{
    (void) v;
    return S8(st, 0x00EC7630u + (uint32_t) to) >= 0;
}

int32_t route_free6_to(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    return route_to_tile(st, x, y, tx, ty, open_walk_free6); /* @bin 0x0051D420 */
}

int32_t route_free6_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n)
{
    return route_to_list(st, x, y, list, n, open_walk_free6); /* @bin 0x0051D6D0 */
}

int32_t route_ec7630_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n)
{
    return route_to_list(st, x, y, list, n, open_ec7630); /* @bin 0x0051B220 */
}
