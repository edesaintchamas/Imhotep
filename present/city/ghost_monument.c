/* Imhotep - the construction ghosts of monuments (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Called by the ghost 0x0043B630 (present/city/ghost.c). The monuments are objects whose virtual
 * function +0x10 (thiscall (terrain, part)) says whether a tile suits a part; the ghost draws each
 * part's footprint tile by tile, green where it suits. Parts: records of 0x34 bytes (+0 dx, +4 dy,
 * +8 size, +0x14 + 4 e x shift, +0x24 + 4 e y shift by the view orientation e). */
#include "ghost.h"

#include "building/rest_redraw.h"
#include "command/place.h"
#include "command/place_misc.h"
#include "state/ported.h"

/* @bin 0x004E5490 (thiscall: parts, count) / 0x00545FC0 (the same with the part this +0x50 that
 * must also have a suitable neighbour: terrain at -228, +228, -1, +1, all through the virtual
 * function) */
static void monument_parts(game_state *st, uint32_t self, uint32_t parts, int32_t count, int big)
{
    uint32_t table = big ? 0x00603258u : 0x005E44D0u; /* the footprint tile order */
    int32_t e = S32(st, G_ORIENT) / 2;
    int32_t xstep = S32(st, G_TILE_W) + 1;
    int32_t ystep = S32(st, G_TILE_H);
    int32_t sx = 0, sy = 0;
    view_tile_to_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    int32_t rel = S32(st, G_HOVER) - S32(st, G_MAP_START);
    int32_t tx = rel % 0xE4, ty = rel / 0xE4;
    for (int32_t k = 0; k < count; k++) {
        uint32_t rec = parts + 0x34u * (uint32_t) k;
        uint8_t near_ok = 0;
        if (big && k == S32(st, self + 0x50)) {
            /* @bin 0x0054607F the four neighbours of the part's first tile */
            int32_t off = (S32(st, rec + 4) + ty) * 0xE4 + S32(st, rec) + S32(st, G_MAP_START) + tx;
            static const int32_t around[4] = {-0xE4, 0xE4, -1, 1};
            for (int a = 0; a < 4; a++) {
                uint32_t fn = U32(st, U32(st, self) + 0x10);
                if ((GAME_CALLT(st, self, fn, (int32_t) g_terrain(st, off + around[a]), k) & 0xFF) != 0) {
                    near_ok = 1;
                }
            }
        }
        int32_t ry = S32(st, rec + 4) + ty;
        int32_t rx = S32(st, rec) + tx;
        S32(st, 0x00BC9768u) = S32(st, G_MAP_START) + ry * 0xE4 + rx;
        int32_t n = S32(st, rec + 8);
        for (int32_t i = 0; i < n; i++) {
            for (int32_t j = 0; j < n; j++) {
                int32_t x = S32(st, rec + 0x14 + 4u * (uint32_t) e) + sx;
                int32_t y = S32(st, rec + 0x24 + 4u * (uint32_t) e) + S32(st, G_TILE_H) + sy;
                if ((GAME_CALL(st, 0x0046F9A0u, S32(st, G_CAMERA_TX) + j + rx, i + ry + S32(st, G_CAMERA_TY), 3) & 0xFF) !=
                    0) {
                    continue;
                }
                int32_t off = S32(st, 0x005CC8C0u + 8u * (uint32_t) (6 * i + j)) + S32(st, 0x00BC9768u);
                uint32_t fn = U32(st, U32(st, self) + 0x10);
                int ok = (GAME_CALLT(st, self, fn, (int32_t) g_terrain(st, off), k) & 0xFF) != 0;
                int32_t colour;
                if (!big) {
                    colour = ok ? 0x1FE3 : 0xF863;
                } else if (!ok) {
                    colour = 0xF863;
                } else {
                    colour = (k != S32(st, self + 0x50) || near_ok) ? 0x1FE3 : 0xF863;
                }
                int32_t idx = 0;
                int32_t o = S32(st, G_ORIENT);
                if (o == 0) {
                    idx = S32(st, table + 4u * (uint32_t) (i + 6 * j));
                } else if (o == 2) {
                    idx = S32(st, table + 4u * (uint32_t) (n - j - 1 + 6 * i));
                    x += (n - 1) * xstep;
                    y -= (n - 1) * ystep;
                } else if (o == 4) {
                    idx = S32(st, table + 4u * (uint32_t) (7 * n - 6 * j - i - 7));
                    y -= 2 * (n - 1) * ystep;
                } else if (o == 6) {
                    idx = S32(st, table + 4u * (uint32_t) (j + 6 * (n - i - 1)));
                    x -= (n - 1) * xstep;
                    y -= (n - 1) * ystep;
                }
                g_foot(st, S32(st, IMG_FOOTPRINT), S32(st, G_FOOT_OFFS + 8u * (uint32_t) idx) + x,
                       S32(st, G_FOOT_OFFS + 4 + 8u * (uint32_t) idx) - S32(st, G_TILE_H) + y, colour);
            }
        }
    }
}

void ghost_4e5490(game_state *st, uint32_t self, uint32_t parts, int32_t count)
{
    monument_parts(st, self, parts, count, 0);
}

void ghost_545fc0(game_state *st, uint32_t self, uint32_t parts, int32_t count)
{
    monument_parts(st, self, parts, count, 1);
}

/* the footprint of n x n tiles at (x, y) (screen, before the tile height), shifted for the view as
 * the monuments do (dis 0x00541CA9): +(n-1) tile widths at 2, -(n-1) at 6, the height back */
static void shift_for_view(game_state *st, int32_t n, int32_t *x, int32_t *y)
{
    if (n <= 1) {
        return;
    }
    int32_t o = S32(st, G_ORIENT);
    int32_t w = S32(st, G_TILE_W) + 1, h = S32(st, G_TILE_H);
    if (o == 2) {
        *x += (n - 1) * w;
        *y -= (n - 1) * h;
    } else if (o == 4) {
        *y -= 2 * (n - 1) * h;
    } else if (o == 6) {
        *x -= (n - 1) * w;
        *y -= (n - 1) * h;
    }
}

static void plain_footprint(game_state *st, int32_t count, int32_t x, int32_t y, int32_t colour)
{
    for (int32_t i = 0; i < count; i++) {
        uint32_t d = G_FOOT_OFFS + 8u * (uint32_t) i;
        g_foot(st, S32(st, IMG_FOOTPRINT), S32(st, d) + x, S32(st, d + 4) - S32(st, G_TILE_H) + y, colour);
    }
}

void ghost_541c00(game_state *st)
{
    /* @bin 0x00541C00 monument 0xD5: one colour (place_monument_d5_fits 0x00541AE0) for its 100
     * parts (0x00601DE0, 0x34 bytes: +0 size, +0xC + 4 e x, +0x1C + 4 e y) */
    int32_t colour = place_monument_d5_fits(st, S32(st, G_HOVER)) ? 0x1FE3 : 0xF863;
    int32_t e = S32(st, G_ORIENT) / 2;
    int32_t sx = 0, sy = 0;
    view_tile_to_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    for (uint32_t rec = 0x00601DE0u; rec < 0x00603230u; rec += 0x34) {
        int32_t x = S32(st, rec + 0xC + 4u * (uint32_t) e) + sx;
        int32_t y = S32(st, G_TILE_H) + sy + S32(st, rec + 0x1C + 4u * (uint32_t) e);
        int32_t n = S32(st, rec);
        int32_t count = S32(st, G_FOOT_COUNT + 4u * (uint32_t) n);
        shift_for_view(st, n, &x, &y);
        plain_footprint(st, count, x, y, colour);
    }
}

void ghost_4debe0(game_state *st)
{
    /* @bin 0x004DEBE0 monument 0xDE: one colour (place_monument_de_fits 0x004DEF00) for its 0x74
     * parts laid out by 0x004DED50 (+0x14 + 4 e x, +0x24 + 4 e y, +8 size) */
    int32_t colour = place_monument_de_fits(st, S32(st, G_HOVER)) ? 0x1FE3 : 0xF863;
    int32_t e = S32(st, G_ORIENT) / 2;
    int32_t sx = 0, sy = 0;
    view_tile_to_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    for (int32_t k = 0; k < 0x74; k++) {
        int32_t lay[13];
        monument_part_layout_4ded50(st, lay, k, S32(st, G_ORIENT), S32(st, G_ORIENT));
        int32_t y = lay[9 + e] + S32(st, G_TILE_H) + sy;
        int32_t x = lay[5 + e] + sx;
        int32_t n = lay[2];
        int32_t count = S32(st, G_FOOT_COUNT + 4u * (uint32_t) n);
        shift_for_view(st, n, &x, &y);
        plain_footprint(st, count, x, y, colour);
    }
}


static int ramp_near(game_state *st, int32_t x, int32_t y)
{
    return (GAME_CALL(st, 0x0046F9A0u, S32(st, G_CAMERA_TX) + x, S32(st, G_CAMERA_TY) + y, 3) & 0xFF) != 0;
}

static void ramp_foot(game_state *st, int32_t x, int32_t y, int32_t colour)
{
    int32_t sx = x, sy = y;
    view_tile_to_screen(st, x, y, &sx, &sy);
    int32_t n = S32(st, 0x005CFCDCu);
    for (int32_t i = 0; i < n; i++) {
        uint32_t d = G_FOOT_OFFS + 8u * (uint32_t) i;
        g_foot(st, S32(st, IMG_FOOTPRINT), S32(st, d) + sx, S32(st, d + 4) + sy, colour);
    }
}

/* @bin 0x004ED360 mode 1 (x, y, 1, colour): the pyramid's ramp drawn as 2x2 footprints from (x, y)
 * eastwards by 2 while the tiles are free (terrain 0xEEFFD76E at +1, +228, +229), at most 20
 * steps, ending on a road; the globals 0x00EAAE84.. as the other modes (core/city/events_actions.c
 * ramp_walk, which leaves this drawing mode to the original). Not registered: core has the
 * address. */
static void ghost_ramp(game_state *st, int32_t x, int32_t y, int32_t colour)
{
    const uint32_t m14 = 0xEEFFD76Eu, m8 = 0xFBFFFF7Bu;
    U8(st, 0x00EAAE84u) = 0;
    S32(st, 0x00EAAEA0u) = 1;
    S32(st, 0x00EAAE94u) = 2;
    S32(st, 0x00EAAE9Cu) = 0;
    S32(st, 0x00EAAE98u) = 0;
    int32_t t = S32(st, G_MAP_START) + y * 0xE4 + x;
    S32(st, 0x00EAAE90u) = 0;
    S32(st, 0x00EAAEA4u) = t;
    S32(st, 0x00EAAE8Cu) = 0;
    if (ramp_near(st, x, y) || ramp_near(st, x + 1, y + 1)) {
        return;
    }
    if ((g_terrain(st, t + 1) & m14) || (g_terrain(st, t + 0xE4) & m14) || (g_terrain(st, t + 0xE5) & m14)) {
        return;
    }
    ramp_foot(st, x, y, colour);
    t += S32(st, 0x00EAAEA0u) * 2;
    int32_t x2 = x + 2;
    if (ramp_near(st, x2, y)) {
        return;
    }
    for (;;) {
        if (ramp_near(st, x2 + 1, y + 1)) {
            return;
        }
        if ((g_terrain(st, t + 1) & m14) || (g_terrain(st, t + 0xE4) & m14) || (g_terrain(st, t + 0xE5) & m14)) {
            /* @bin 0x004ED92F an obstacle: the walk ends on it when it is a road */
            if (S32(st, 0x00EAAE98u) == 0) {
                return;
            }
            uint32_t a = g_terrain(st, t + 1);
            if ((m8 & a) || (g_terrain(st, t + 0xE4) & m14)) {
                return;
            }
            uint32_t b = g_terrain(st, t + 0xE5);
            if ((m8 & b) || !(a & 4) || !(b & 4)) {
                return;
            }
            if (ramp_near(st, x2, y) || ramp_near(st, x2 + 1, y + 1)) {
                return;
            }
            ramp_foot(st, x2, y, colour);
            S32(st, 0x00EAAE9Cu) = t;
            return;
        }
        ramp_foot(st, x2, y, colour);
        S32(st, 0x00EAAE98u) = S32(st, 0x00EAAE98u) + 1;
        if (S32(st, 0x00EAAE98u) > 0x14) {
            return;
        }
        if (S32(st, G_CAMERA_TY) + y >= S32(st, 0x00BCA2F4u)) {
            return;
        }
        t += S32(st, 0x00EAAEA0u) * 2;
        x2 += 2;
        if (ramp_near(st, x2, y)) {
            return;
        }
    }
}

void ghost_4edbb0(game_state *st)
{
    /* @bin 0x004EDBB0 a pyramid 0xB7 (variant 0x005E6970): place_pyramid_fits 0x004ECEC0 gives the
     * colour; its blocks (2x2) from the tables 0x005F1D74 (when 0x005E4EF4 = 5) or 0x005E6974, red
     * on terrain 0xFEFFD76E or near the map's edge; the ramp (0x004ED360) for the kinds
     * 0x004ECEA0 accepts; then the tiles of 0x005700B4.. that hold terrain 0xFEFFD72E in red */
    int32_t red_all = g_treasury_low(st);
    int32_t reason = 0;
    int32_t colour = place_pyramid_fits(st, S32(st, G_HOVER), S32(st, 0x005E6970u), &reason) ? 0x1FE3 : 0xF863;
    int32_t e = S32(st, G_ORIENT) / 2;
    int32_t sx = 0, sy = 0;
    view_tile_to_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    for (int32_t k = 0;; k++) {
        int32_t v = S32(st, 0x005E6970u);
        int alt = S32(st, 0x005E4EF4u) == 5;
        int32_t limit = alt ? S32(st, 0x0056F7ECu + 4u * (uint32_t) v) : S32(st, 0x0056F7D8u + 4u * (uint32_t) v);
        if (k >= limit) {
            break;
        }
        int32_t tile, x0, y0;
        if (alt) {
            uint32_t rec = 0x005F1D74u + 16u * (uint32_t) (k + 36 * (4 * v + e));
            int32_t p = S32(st, rec + 8);
            tile = S32(st, 0x0056FD10u + 8u * (uint32_t) p) + S32(st, 0x0056FD14u + 8u * (uint32_t) p) * 0xE4 + S32(st, G_HOVER);
            x0 = S32(st, rec) * (S32(st, 0x0070E458u) + 2);
            y0 = S32(st, rec + 4);
        } else {
            uint32_t rec = 0x005E6974u + 16u * (uint32_t) (k + 144 * (4 * v + e));
            tile = S32(st, 0x0056FE30u + 4u * U32(st, rec + 8)) + S32(st, G_HOVER);
            x0 = S32(st, rec) * (S32(st, 0x0070E458u) + 2);
            y0 = S32(st, rec + 4);
        }
        y0 = y0 * S32(st, G_TILE_TOP) + sy;
        x0 += sx;
        int32_t count = S32(st, 0x005CFCDCu);
        if (count <= 0) {
            continue;
        }
        int32_t ty = tile / 0xE4, tx = tile % 0xE4;
        uint32_t row = G_FOOT_TILES + 0x90u * (uint32_t) e;
        for (int32_t i = 0; i < count; i++) {
            uint32_t d = G_FOOT_OFFS + 8u * (uint32_t) i;
            int32_t x = S32(st, d) + x0;
            int32_t y = S32(st, d + 4) + y0;
            uint32_t t = g_terrain(st, S32(st, row + 4u * (uint32_t) i) + tile) & 0xFEFFD76Eu;
            int near = (GAME_CALL(st, 0x0046F9A0u, tx, ty, 3) & 0xFF) != 0;
            g_foot(st, S32(st, IMG_FOOTPRINT), x, y, (near || t || red_all) ? RED : GREEN);
        }
    }
    if ((GAME_CALL(st, 0x004ECEA0u, S32(st, 0x005E6970u)) & 0xFF) != 0) {
        /* @bin 0x004EDE40 the ramp */
        int32_t off = S32(st, 0x005F5D08u + 4u * U32(st, 0x005E6970u)) + S32(st, G_HOVER) - S32(st, G_MAP_START);
        ghost_ramp(st, off % 0xE4, off / 0xE4, colour & 0xFFFF); /* 0x004ED360 mode 1 */
    }
    int32_t v = S32(st, 0x005E6970u);
    int32_t n, base;
    if (S32(st, 0x005E4EF4u) == 5) {
        n = S32(st, 0x005700A8u + 4u * (uint32_t) v);
        base = S32(st, 0x005700C8u + 4u * (uint32_t) v);
    } else {
        n = S32(st, 0x00570094u + 4u * (uint32_t) v);
        base = S32(st, 0x005700B4u + 4u * (uint32_t) v);
    }
    for (int32_t s = 0; s < n; s++) {
        int32_t off = S32(st, G_HOVER) + base + s;
        if (g_terrain(st, off) & 0xFEFFD72Eu) {
            int32_t x = 0, y = 0;
            view_tile_to_screen(st, off % 0xE4 - S32(st, G_CAMERA_TX), off / 0xE4 - S32(st, G_CAMERA_TY), &x, &y);
            g_foot(st, S32(st, IMG_FOOTPRINT), x, y, RED);
        }
    }
}

#define THIS(a) ((uint32_t) (a)[PORT_THIS_ARG])
static int32_t run_4e5490(game_state *st, const int32_t *a) { ghost_4e5490(st, THIS(a), (uint32_t) a[0], a[1]); return 0; }
static int32_t run_545fc0(game_state *st, const int32_t *a) { ghost_545fc0(st, THIS(a), (uint32_t) a[0], a[1]); return 0; }
static int32_t run_541c00(game_state *st, const int32_t *a) { (void) a; ghost_541c00(st); return 0; }
static int32_t run_4debe0(game_state *st, const int32_t *a) { (void) a; ghost_4debe0(st); return 0; }
static int32_t run_4edbb0(game_state *st, const int32_t *a) { (void) a; ghost_4edbb0(st); return 0; }
/* the monuments' ghost methods (thiscall, no argument): parts this + k, count */
#define THUNK(name, k, n) \
    static int32_t name(game_state *st, const int32_t *a) { ghost_4e5490(st, THIS(a), THIS(a) + (k), (n)); return 0; }
#define THUNK_BIG(name, k, n) \
    static int32_t name(game_state *st, const int32_t *a) { ghost_545fc0(st, THIS(a), THIS(a) + (k), (n)); return 0; }
THUNK(run_401420, 0x4C, 0x10)
THUNK(run_40d870, 0x4C, 0x19)
THUNK(run_416f90, 0x4C, 9)
THUNK(run_4e6610, 0x4C, 2)
THUNK_BIG(run_5456e0, 0x1E5C, 0xA7)
THUNK_BIG(run_54f410, 0x5C88, 0x201)
THUNK_BIG(run_555e20, 0x4EE0, 0x1B5)
THUNK_BIG(run_558b80, 0x1CBC, 0x9E)


/* the monuments' virtual function +0x10 (thiscall (terrain, part), al: the tile suits the part);
 * terrain bits 0xFEFFD76F */
static int32_t run_401440(game_state *st, const int32_t *a)
{
    /* @bin 0x00401440 part with +0x58 (+ 0x34 a part) = 9: plain land; else 0x20000002 exactly */
    uint32_t t = (uint32_t) a[0] & 0xFEFFD76Fu;
    if (S32(st, THIS(a) + 0x58 + 0x34u * (uint32_t) a[1]) == 9) {
        return t == 0;
    }
    return t == 0x20000002u;
}

static int32_t run_4e5310(game_state *st, const int32_t *a)
{
    (void) st;
    return ((uint32_t) a[0] & 0xFEFFD76Fu) == 0; /* @bin 0x004E5310 plain land */
}

static int32_t run_4e6620(game_state *st, const int32_t *a)
{
    /* @bin 0x004E6620 part 1: plain land; the others: 2 exactly */
    (void) st;
    uint32_t t = (uint32_t) a[0] & 0xFEFFD76Fu;
    return a[1] == 1 ? t == 0 : t == 2;
}

static int32_t run_547de0(game_state *st, const int32_t *a)
{
    /* @bin 0x00547DE0 the part this +0x50: plain land; the others: 0x20000002 exactly */
    uint32_t t = (uint32_t) a[0] & 0xFEFFD76Fu;
    if (a[1] == S32(st, THIS(a) + 0x50)) {
        return t == 0;
    }
    return t == 0x20000002u;
}

const ported_function cityview_ghost_monument_ports[] = {
    {0x004E5490u, "ghost_monument_parts", 2, 0, 0, run_4e5490, 0},
    {0x00545FC0u, "ghost_monument_parts_big", 2, 0, 0, run_545fc0, 0},
    {0x00541C00u, "ghost_monument_d5", 0, 0, 0, run_541c00, 0},
    {0x004DEBE0u, "ghost_monument_de", 0, 0, 0, run_4debe0, 0},
    {0x004EDBB0u, "ghost_pyramid", 0, 0, 0, run_4edbb0, 0},
    {0x00401420u, "ghost_monument_e6", 0, 0, 0, run_401420, 0},
    {0x0040D870u, "ghost_monument_e1", 0, 0, 0, run_40d870, 0},
    {0x00416F90u, "ghost_monument_e3", 0, 0, 0, run_416f90, 0},
    {0x004E6610u, "ghost_monument_e4", 0, 0, 0, run_4e6610, 0},
    {0x005456E0u, "ghost_monument_e5", 0, 0, 0, run_5456e0, 0},
    {0x0054F410u, "ghost_monument_ec", 0, 0, 0, run_54f410, 0},
    {0x00555E20u, "ghost_monument_eb", 0, 0, 0, run_555e20, 0},
    {0x00558B80u, "ghost_monument_ea", 0, 0, 0, run_558b80, 0},
    {0x00401440u, "monument_part_tile_ok_e6", 2, 8, 0, run_401440, 0},
    {0x004E5310u, "monument_part_tile_ok_plain", 2, 8, 0, run_4e5310, 0},
    {0x004E6620u, "monument_part_tile_ok_e4", 2, 8, 0, run_4e6620, 0},
    {0x00547DE0u, "monument_part_tile_ok_big", 2, 8, 0, run_547de0, 0},
};
const int cityview_ghost_monument_ports_count =
    sizeof(cityview_ghost_monument_ports) / sizeof(cityview_ghost_monument_ports[0]);
