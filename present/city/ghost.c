/* Imhotep - the construction ghost 0x0043B630 (package cityview, docs/presentation.md 3.1).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The building chosen (0x006059E8) drawn at the hovered tile (0x00BCA300..) over the city view: the
 * building's image tinted green (0x0070E120 = 0x1FE3) where it can be placed, else its footprint
 * tiles red (0xF863) or green one by one. Roads, canals and bridges have their own draws (entered
 * by a jump from 0x0043B630: 0x0043C590, 0x0043C780, 0x0043C980), the other special types are
 * the functions of ghost_*.c. The state writes: 0x006DE514, the R key's building orientation
 * 0x00BC9714, 0x00BC9768 (the tile for 0x0047B920 / 0x0047E870), 0x00BC9748, the bridge's
 * draw records 0x0070E13C / 0x0070E140.., the flag 0x00BCA32C (0x0043DD70), and what the rule
 * functions it calls write (bridge_measure 0x0047F200, canal_flags 0x0047E870, ...). */
#include "ghost.h"

#include "frame/frame.h"
#include "state/ported.h"

#define MONUMENT_FLAG 0x006DE514u /* u8 1: the type is a monument of 0xE5, 0xEA, 0xEB, 0xEC */
#define CANAL_TILE 0x00BC9768u    /* i32 the tile 0x0047B920 / 0x0047E870 look at */
#define BRIDGE_DIR 0x00BC96F4u
#define BRIDGE_LEN 0x00BC96F8u
#define BRIDGE_END 0x00BC96FCu
#define BRIDGE_COUNT 0x0070E13Cu  /* i32 the draws recorded */
#define BRIDGE_RECS 0x0070E140u   /* 16 bytes each: image, x, y of the draw (0x00D3B580 / 7C / 78) */
#define LAST_IMAGE 0x00D3B580u    /* i32 the last draw's image, x, y (0x0053E590) */
#define LAST_X 0x00D3B57Cu
#define LAST_Y 0x00D3B578u

/* ---- small rule helpers ---- */

int32_t ghost_47ed00(game_state *st, int32_t off, uint32_t mask)
{
    /* @bin 0x0047ED00 (offset, mask) the terrain at +1 or -1 has a bit of mask */
    if (U32(st, G_TERRAIN + 4u * (uint32_t) (off + 1)) & mask) {
        return 1;
    }
    return (U32(st, G_TERRAIN + 4u * (uint32_t) (off - 1)) & mask) != 0;
}

int32_t ghost_47ed30(game_state *st, int32_t off, uint32_t mask)
{
    /* @bin 0x0047ED30 (offset, mask) the terrain at -228 or +228 has a bit of mask */
    if (U32(st, G_TERRAIN + 4u * (uint32_t) (off - 0xE4)) & mask) {
        return 1;
    }
    return (U32(st, G_TERRAIN + 4u * (uint32_t) (off + 0xE4)) & mask) != 0;
}

int32_t ghost_47ed60(game_state *st, int32_t off)
{
    /* @bin 0x0047ED60 (offset) water (bit 4) at -228, +1, +228: then whether -1 is too */
    if (!(U8(st, G_TERRAIN + 4u * (uint32_t) (off - 0xE4)) & 4)) {
        return 0;
    }
    if (!(U8(st, G_TERRAIN + 4u * (uint32_t) (off + 1)) & 4)) {
        return 0;
    }
    if (!(U8(st, G_TERRAIN + 4u * (uint32_t) (off + 0xE4)) & 4)) {
        return 0;
    }
    return (U8(st, G_TERRAIN + 4u * (uint32_t) (off - 1)) >> 2) & 1;
}

int32_t ghost_477d40(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00477D40 (x, y) 1 when the tile is water (4) with bit 0x4000000 and one of its 8
     * neighbours (-228, -227, +1, +229, +228, +227, -1, -229) lacks 0x4000000 */
    int32_t off = S32(st, G_MAP_START) + y * 0xE4 + x;
    uint32_t t = g_terrain(st, off);
    if (!(t & 4) || !(t & 0x4000000u)) {
        return 0;
    }
    static const int32_t around[8] = {-0xE4, -0xE3, 1, 0xE5, 0xE4, 0xE3, -1, -0xE5};
    for (int i = 0; i < 8; i++) {
        if (!(g_terrain(st, off + around[i]) & 0x4000000u)) {
            return 1;
        }
    }
    return 0;
}

int32_t ghost_51c460(game_state *st, int32_t off)
{
    /* @bin 0x0051C460 (offset) the bridge-ramp image of a tile: its image - 0x00F56B84 (mod 0x30)
     * through the byte table 0x0051C4E0 and the jump table 0x0051C4CC */
    static const uint8_t kind[48] = {0, 1, 4, 4, 4, 4, 1, 0, 1, 0, 4, 4, 4, 4, 4, 0, 1, 0, 1, 0, 1, 2, 3, 4,
                                     4, 4, 4, 3, 2, 3, 2, 4, 4, 4, 4, 4, 2, 3, 2, 3, 2, 3, 0, 1, 0, 1, 0, 1};
    int32_t v = S32(st, G_GRID_IMAGE + 4u * (uint32_t) off) - S32(st, 0x00F56B84u);
    if (v >= 0x30) {
        v -= 0x30;
    }
    if ((uint32_t) v > 0x2F) {
        return 1;
    }
    switch (kind[v]) {
    case 0:
        return GAME_CALL(st, 0x0047B8E0u, off) != 0 ? 0x2A : 0xF;
    case 1:
        return GAME_CALL(st, 0x0047B8E0u, off) != 0 ? 0x2B : 0x10;
    case 2:
        return 0x24;
    case 3:
        return 0x10;
    default:
        return 1;
    }
}

int32_t ghost_4c0ed0(int32_t type)
{
    /* @bin 0x004C0ED0 the temple-complex part of a type 0x3C..0x45 (jump table 0x004C0F04) */
    if ((uint32_t) (type - 0x3C) > 9) {
        return -1;
    }
    return (type - 0x3C) % 5;
}

/* ---- small ghosts ---- */

void ghost_43dd70(game_state *st, int32_t unused, int32_t floodable)
{
    /* @bin 0x0043DD70 (unused, floodable) a 2x2 building on the shore (water lifts 7 / 0x5A, 0x4C,
     * 0xB5): shore_fit 0x0047CC70; the water-lift image (0x00487D30) with the flag 0x00BCA32C */
    (void) unused;
    int32_t sx, sy;
    g_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    sy += S32(st, G_TILE_H);
    uint32_t m = G_MODELS + 20u * U32(st, G_TYPE);
    int32_t image = g_group(st, S32(st, m + 8)) + S32(st, m + 0xC);
    int32_t fit = GAME_CALL(st, 0x0047CC70u, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), 1, floodable);
    if (g_treasury_low(st)) {
        fit = 999;
    } else if (fit == 0) {
        int32_t base = S32(st, 0x00F56CCCu);
        if (image >= base && image <= base + 0xB) {
            /* @bin 0x0043DE65 */
            U8(st, 0x00BCA32Cu) = (uint8_t) GAME_CALL(st, 0x004893C0u, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), 2, 2,
                                                      0x10000);
            image = GAME_CALL(st, 0x00487D30u, S32(st, 0x00BC96DCu), fit);
        } else {
            image += S32(st, 0x00BC96DCu);
        }
        int32_t y = sy - S32(st, G_TILE_H);
        U16(st, G_TINT) = GREEN;
        g_draw(st, image, sx, y);
        g_top(st, image, sx, y);
        U16(st, G_TINT) = 0;
        return;
    }
    for (uint32_t p = G_FOOT_OFFS + 4; p < 0x005CFD14u; p += 8) {
        int32_t x = S32(st, p - 4) + sx;
        int32_t y = S32(st, p) - S32(st, G_TILE_H) + sy;
        g_foot(st, S32(st, IMG_FOOTPRINT), x, y, (fit == 999 || fit != 0) ? RED : GREEN);
    }
}

void ghost_43df10(game_state *st, int32_t group)
{
    /* @bin 0x0043DF10 (image group) a 3x3 building on the shore (0x4A, 0x4B, 0xB6, 0xC0):
     * shore_fit_47d060, the image by the side 0x00BC96DC */
    int32_t sx, sy;
    g_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    sy += S32(st, G_TILE_H);
    int32_t fit = GAME_CALL(st, 0x0047D060u, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), 1);
    if (g_treasury_low(st)) {
        fit = 999;
    } else if (fit == 0) {
        int32_t side = S32(st, 0x00BC96DCu);
        int32_t image = g_group(st, group);
        if (side == 0) {
        } else if (side == 1) {
            image += 1;
        } else if (side == 2) {
            image += 2;
        } else {
            image += 3;
        }
        int32_t y = sy - S32(st, G_TILE_H);
        U16(st, G_TINT) = GREEN;
        g_draw(st, image, sx, y);
        g_top(st, image, sx, y);
        U16(st, G_TINT) = 0;
        return;
    }
    for (uint32_t p = G_FOOT_OFFS + 4; p < 0x005CFD3Cu; p += 8) {
        int32_t x = S32(st, p - 4) + sx;
        int32_t y = S32(st, p) - S32(st, G_TILE_H) + sy;
        g_foot(st, S32(st, IMG_FOOTPRINT), x, y, (fit == 999 || fit != 0) ? RED : GREEN);
    }
}

void ghost_43e090(game_state *st)
{
    /* @bin 0x0043E090 type 0x81 (editor): 0x0047D350 (x, y, 0); the image + 0x00BC96D4 */
    int32_t sx, sy;
    g_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    sy += S32(st, G_TILE_H);
    uint32_t m = G_MODELS + 20u * U32(st, G_TYPE);
    int32_t image = g_group(st, S32(st, m + 8)) + S32(st, m + 0xC);
    if (GAME_CALL(st, 0x0047D350u, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), 0) == 0) {
        int32_t y = sy - S32(st, G_TILE_H);
        image += S32(st, 0x00BC96D4u);
        U16(st, G_TINT) = GREEN;
        g_draw(st, image, sx, y);
        g_top(st, image, sx, y);
        U16(st, G_TINT) = 0;
        return;
    }
    for (uint32_t p = G_FOOT_OFFS; p < 0x005CFD10u; p += 8) {
        g_foot(st, S32(st, IMG_FOOTPRINT), S32(st, p) + sx, S32(st, p + 4) - S32(st, G_TILE_H) + sy, RED);
    }
}

/* ---- roads, canals, bridges (entered by a jump from 0x0043B630) ---- */

static void ghost_road(game_state *st)
{
    /* @bin 0x0043C590 a road: the road image the tile would get (bridge ramps 0x0051C390 /
     * 0x0051C460, flood plain 0x0051C510 / 0x0051C5F0, the junction by the neighbours) */
    int32_t hov = S32(st, G_HOVER);
    int32_t image = S32(st, 0x00F56C94u);
    int bad = 0;
    int32_t sx, sy;
    g_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    uint32_t t = g_terrain(st, hov);
    sy += S32(st, G_TILE_H);
    if ((t & 0x10000) || g_flooded(st, hov)) {
        image = S32(st, 0x00F56C34u);
        if (GAME_CALL(st, 0x0051C510u, hov) == 0) {
            bad = 1;
        } else {
            image += GAME_CALL(st, 0x0051C5F0u, hov, S32(st, G_GRID_IMAGE + 4u * (uint32_t) hov) - image);
        }
    }
    t &= 0xEEFED77Fu;
    if (t & 0x100) {
        image = S32(st, 0x00F56B84u);
        if (GAME_CALL(st, 0x0051C390u, hov) == 0) {
            bad = 1;
        } else {
            image += ghost_51c460(st, hov);
        }
    } else if (t != 0) {
        bad = 1;
    } else {
        image = (U8(st, G_GRID_EDGE + (uint32_t) hov) & 0x80) ? S32(st, 0x00F56C44u) : S32(st, 0x00F56C94u);
        if (ghost_47ed30(st, S32(st, G_HOVER), 0x40) == 0 && ghost_47ed00(st, S32(st, G_HOVER), 0x40) != 0) {
            image++;
        }
    }
    if (g_treasury_low(st)) {
        bad = 1;
    }
    if ((GAME_CALL(st, 0x0046F9A0u, S32(st, G_CAMERA_TX) + S32(st, G_HOVER_X), S32(st, G_CAMERA_TY) + S32(st, G_HOVER_Y),
                   3) & 0xFF) != 0 ||
        bad) {
        g_foot(st, S32(st, IMG_FOOTPRINT), sx, sy - S32(st, G_TILE_H), RED);
        return;
    }
    int32_t y = sy - S32(st, G_TILE_H);
    U16(st, G_TINT) = GREEN;
    g_draw(st, image, sx, y);
    g_top(st, image, sx, y);
    U16(st, G_TINT) = 0;
}

static void ghost_canal(game_state *st)
{
    /* @bin 0x0043C780 a canal: the canal image (0x0047E870 the neighbours, 0x00484BE0 the
     * pattern table 0x005CC678) */
    int32_t sx, sy;
    int bad = 0;
    g_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    sy += S32(st, G_TILE_H);
    if (S32(st, G_DRAGGING) == 0) {
        int32_t off = S32(st, G_MAP_START) + S32(st, G_HOVER_Y) * 0xE4 + S32(st, G_HOVER_X);
        S32(st, CANAL_TILE) = off;
        uint32_t t = g_terrain(st, off);
        if (t & 0x40) {
            bad = GAME_CALL(st, 0x0051C630u, off) == 0;
        } else if ((t & 0xEEFED77Fu) != 0 || g_flooded(st, off)) {
            bad = 1;
        }
    } else if (S32(st, 0x00BC9724u) == 0) {
        bad = 1;
    }
    if (g_treasury_low(st)) {
        bad = 1;
    }
    if ((GAME_CALL(st, 0x0046F9A0u, S32(st, G_CAMERA_TX) + S32(st, G_HOVER_X), S32(st, G_CAMERA_TY) + S32(st, G_HOVER_Y),
                   3) & 0xFF) != 0) {
        bad = 1;
    }
    int32_t y = sy - S32(st, G_TILE_H);
    if (bad) {
        g_foot(st, S32(st, IMG_FOOTPRINT), sx, y, RED);
        return;
    }
    U16(st, G_TINT) = GREEN;
    S32(st, CANAL_TILE) = S32(st, G_HOVER);
    GAME_CALL0(st, 0x0047E870u);
    GAME_CALL(st, 0x00484BE0u, 0x005CC678, 0x10, -1, 0);
    int32_t off = S32(st, CANAL_TILE);
    int32_t k;
    if (U8(st, G_TERRAIN + 4u * (uint32_t) off) & 0x40) {
        if (S32(st, 0x00BC9744u) == 0) {
            /* @bin 0x0043C8F5 */
            S32(st, 0x00BC9748u) = (int32_t) ((((uint32_t) (uint8_t) ~U8(st, 0x00AF55FCu + 4u * (uint32_t) off)) >> 6) & 1);
        }
        if (U8(st, G_GRID_EDGE + (uint32_t) off) & 0x80) {
            k = S32(st, G_GRID_IMAGE + 4u * (uint32_t) off) - S32(st, 0x00F56B84u);
        } else {
            GAME_CALL0(st, 0x0047B920u);
            k = S32(st, 0x00BC9748u) + 0x30;
        }
    } else {
        k = S32(st, 0x00BC9748u) + 0x30;
    }
    g_draw(st, S32(st, 0x00F56B84u) + k, sx, y);
    U16(st, G_TINT) = 0;
}

static void bridge_record(game_state *st)
{
    /* @bin 0x0043CB79 the draw just made: image, x, y (0x00D3B580 / 0x00D3B57C / 0x00D3B578) */
    int32_t n = S32(st, BRIDGE_COUNT);
    uint32_t r = BRIDGE_RECS + 16u * (uint32_t) n;
    S32(st, r) = S32(st, LAST_IMAGE);
    S32(st, r + 4) = S32(st, LAST_X);
    S32(st, r + 8) = S32(st, LAST_Y);
    S32(st, BRIDGE_COUNT) = n + 1;
}

static void bridge_step(game_state *st, int32_t dir, int32_t *x, int32_t *y)
{
    if (dir == 0) {
        *x += S32(st, G_TILE_W);
        *y -= S32(st, G_TILE_H);
    } else if (dir == 2) {
        *x += S32(st, G_TILE_W);
        *y += S32(st, G_TILE_H);
    } else if (dir == 4) {
        *x -= S32(st, G_TILE_W);
        *y += S32(st, G_TILE_H);
    } else if (dir == 6) {
        *x -= S32(st, G_TILE_W);
        *y -= S32(st, G_TILE_H);
    }
}

static void ghost_bridge(game_state *st)
{
    /* @bin 0x0043C980 a bridge (0x52 low, 0x53 high): bridge_measure 0x0047F200, then the spans */
    int32_t sx, sy;
    g_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    int32_t type = S32(st, G_TYPE);
    sy += S32(st, G_TILE_H);
    GAME_CALL(st, 0x0047F200u, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), type == 0x52 ? 0 : 1);
    int32_t dir = S32(st, BRIDGE_DIR) - S32(st, G_ORIENT);
    if (dir < 0) {
        dir += 8;
    }
    type = S32(st, G_TYPE);
    int32_t len = S32(st, BRIDGE_LEN);
    int bad = (type == 0x53 && len < 5) ? 1 : S32(st, BRIDGE_END) == 0;
    int32_t x = sx, y;
    if (g_treasury_low(st) || bad) {
        /* @bin 0x0043CA48 the footprints: the first green when the bridge has a length */
        y = sy - S32(st, G_TILE_H);
        g_foot(st, S32(st, IMG_FOOTPRINT), x, y, len < 1 ? RED : GREEN);
        if (S32(st, BRIDGE_LEN) <= 1) {
            return;
        }
        int32_t w = S32(st, G_TILE_W), h = S32(st, G_TILE_H);
        for (int32_t n = S32(st, BRIDGE_LEN) - 1; n != 0; n--) {
            if (dir == 0) {
                x += w;
                y -= h;
            } else if (dir == 2) {
                x += w;
                y += h;
            } else if (dir == 4) {
                x -= w;
                y += h;
            } else if (dir == 6) {
                x -= w;
                y -= h;
            }
        }
        g_foot(st, S32(st, IMG_FOOTPRINT), x, y, RED);
        return;
    }
    y = sy - S32(st, G_TILE_TOP) - S32(st, G_TILE_H);
    U16(st, G_TINT) = GREEN;
    S32(st, BRIDGE_COUNT) = 0;
    if (type == 0x52) {
        /* @bin 0x0043CB23 a low bridge */
        int32_t k = dir == 0 ? 3 : dir == 2 ? 0 : dir == 4 ? 5 : 2;
        g_foot(st, S32(st, 0x00F56D34u) + k, x, y, GREEN);
        g_draw(st, S32(st, 0x00F56D34u) + k, x, y);
        bridge_record(st);
        for (int32_t i = 1; i < S32(st, BRIDGE_LEN); i++) {
            bridge_step(st, dir, &x, &y);
            if (i == S32(st, BRIDGE_LEN) - 1) {
                k = dir == 0 ? 5 : dir == 2 ? 2 : dir == 4 ? 3 : 0;
            } else {
                k = (dir == 0 || dir == 4) ? 4 : 1;
            }
            g_draw(st, S32(st, 0x00F56D34u) + k, x, y);
            bridge_record(st);
        }
    } else {
        /* @bin 0x0043CCD8 a high bridge: the middle span by the length */
        int32_t mid;
        if (len == 9 || len == 10) {
            mid = 4;
        } else if (len == 11 || len == 12) {
            mid = 5;
        } else if (len == 13 || len == 14) {
            mid = 6;
        } else if (len == 15) {
            mid = 7;
        } else {
            mid = len != 0x10 ? 8 : 7;
        }
        if (dir == 0) {
            g_draw(st, S32(st, 0x00F56D34u) + 0xB, x - 3, y - 0x32);
        } else if (dir == 2) {
            g_draw(st, S32(st, 0x00F56D34u) + 6, x - 1, y - 0xC);
        } else if (dir == 4) {
            g_draw(st, S32(st, 0x00F56D34u) + 9, x - 0x1E, y - 0xC);
        } else if (dir == 6) {
            g_draw(st, S32(st, 0x00F56D34u) + 8, x - 0x17, y - 0x35);
        }
        bridge_record(st);
        for (int32_t i = 1; i < S32(st, BRIDGE_LEN); i++) {
            bridge_step(st, dir, &x, &y);
            if (i == S32(st, BRIDGE_LEN) - 1 || i == 1) {
                continue;
            }
            int32_t k = -1, dx = 0, dy = 0;
            if (i == S32(st, BRIDGE_LEN) - 2) {
                if (dir == 0) {
                    k = 9, dx = 1, dy = -0x18;
                } else if (dir == 2) {
                    k = 8, dx = 7, dy = -0x27;
                } else if (dir == 4) {
                    k = 0xB, dx = -0x22, dy = -0x23;
                } else if (dir == 6) {
                    k = 6, dx = -0x1D, dy = -0x16;
                }
            } else if (i == mid) {
                if (dir == 0 || dir == 4) {
                    k = 0xD, dy = -0x26;
                } else if (dir == 2 || dir == 6) {
                    k = 0xC, dx = 7, dy = -0x26;
                }
            } else {
                if (dir == 0 || dir == 4) {
                    k = 0xA, dy = -0x25;
                } else if (dir == 2 || dir == 6) {
                    k = 7, dx = 7, dy = -0x26;
                }
            }
            if (k >= 0) {
                g_draw(st, S32(st, 0x00F56D34u) + k, x + dx, y + dy);
            }
            bridge_record(st);
        }
    }
    /* @bin 0x0043CFB7 facing the viewer (0, 6): drawn again in reverse order */
    if (dir == 0 || dir == 6) {
        int32_t n = S32(st, BRIDGE_COUNT);
        for (int32_t i = n - 1; i >= 0; i--) {
            uint32_t r = BRIDGE_RECS + 16u * (uint32_t) i;
            g_draw(st, S32(st, r), S32(st, r + 4), S32(st, r + 8));
        }
    }
    U16(st, G_TINT) = 0;
}

/* ---- the generic ghost (0x0043B950) ---- */

static int is_one_of(int32_t t, const int32_t *list, int n)
{
    for (int i = 0; i < n; i++) {
        if (t == list[i]) {
            return 1;
        }
    }
    return 0;
}

static void ghost_generic(game_state *st)
{
    int32_t red_all = 0; /* [esp+0x18]: every footprint tile red */
    int32_t bad = 0;     /* [esp+0x14] */
    /* @bin 0x0043B968 the R key turns gatehouses and the like: 1 <-> 2 */
    if (S32(st, G_BUILD_ORIENT) > 0 && (uint16_t) frame_key_state(st, 0x52) == 0x8001) {
        S32(st, G_BUILD_ORIENT) = S32(st, G_BUILD_ORIENT) == 1 ? 2 : 1;
    }
    int32_t hov = S32(st, G_HOVER);
    int32_t type = S32(st, G_TYPE);
    uint32_t m = G_MODELS + 20u * (uint32_t) type;
    int32_t size = S32(st, m);
    int32_t image = g_group(st, S32(st, m + 8)) + S32(st, m + 0xC);
    if (type == 0x48) {
        size = 3;
    }
    int32_t hx = S32(st, G_HOVER_X), hy = S32(st, G_HOVER_Y);
    int32_t ax = hx, ay = hy; /* @bin 0x0043B9D1 the anchor by the orientation */
    int32_t orient = S32(st, G_ORIENT);
    if (orient == 0) {
    } else if (orient == 2) {
        ax = hx - size + 1;
    } else {
        if (orient == 4) {
            ax = hx - size + 1;
        }
        ay = hy - size + 1;
    }
    if (S32(st, G_EDITOR) == 0) {
        /* @bin 0x0043BA2D 3 tiles from the map's edges */
        if (hx < 3 || hx + size - 1 >= S32(st, G_MAP_W) - 3 || hy < 3 || hy + size - 1 >= S32(st, G_MAP_H) - 3) {
            red_all = bad = 1;
        }
    }
    if (GAME_CALL(st, 0x00482650u, ax, ay, size, 1, 0x80000) != 0) {
        red_all = bad = 1;
    }
    /* @bin 0x0043BA7E the required terrain by the placement flags (docs/commands.md 2.3) */
    int32_t r = 1;
    int check = 1;  /* r is a check result (0x0043BC2E) */
    int extra = 1;  /* go on to the city checks (0x0043BCF2) */
    if (S32(st, 0x006059CCu) != 0) {
        r = GAME_CALL(st, 0x00482650u, ax, ay, 3, 1, 0x10800);
    } else if (S32(st, 0x00605A48u) != 0) {
        r = GAME_CALL(st, 0x00482720u, ax, ay, 2, 1, S32(st, 0x00605A48u));
    } else if (S32(st, 0x006059C8u) != 0) {
        r = GAME_CALL(st, 0x00482650u, ax, ay, 2, 1, 1);
    } else if (S32(st, 0x006059C4u) != 0) {
        r = GAME_CALL(st, 0x00482650u, ax, ay, 2, 3, 0x10004);
    } else if (S32(st, 0x006059C0u) != 0) {
        r = GAME_CALL(st, 0x00482650u, ax, ay, 2, 1, 0x40000);
    } else if (S32(st, 0x006059BCu) != 0) {
        r = GAME_CALL(st, 0x004828E0u, ax, ay, 2, 0, 0x4000);
    } else if (S32(st, 0x006059B8u) != 0) {
        r = GAME_CALL(st, 0x004828E0u, ax, ay, 1, 0, 0x40);
    } else if (S32(st, 0x006059B4u) != 0) {
        r = GAME_CALL(st, 0x004829B0u, ax, ay, size, 0, 0x80, S32(st, 0x006059B4u));
    } else {
        check = 0;
        if (type == 0x7E || type == 0x7F || type == 0x80 || type == 0x82 || type == 0x83) {
            /* @bin 0x0043BC62 near the map's edge */
            if ((GAME_CALL(st, 0x0046F9A0u, S32(st, G_CAMERA_TX) + hx, S32(st, G_CAMERA_TY) + hy, 4) & 0xFF) == 0) {
                red_all = bad = 1;
            }
            type = S32(st, G_TYPE);
            if (type == 0x7E) {
                extra = 0;
                if (S32(st, 0x00605A28u) > 8 && !(U8(st, G_TERRAIN + 4u * U32(st, G_HOVER)) & 4)) {
                    red_all = bad = 1;
                }
            } else if (type == 0x82 || type == 0x83) {
                r = ghost_47ed60(st, S32(st, G_HOVER));
                check = 1;
            }
        } else if (type == 0x84) {
            extra = 0;
            if (!(U8(st, G_TERRAIN + 4u * U32(st, G_HOVER)) & 4)) {
                red_all = bad = 1;
            }
        } else if (type == 0x85 || type == 0x8B) {
            r = GAME_CALL(st, 0x004EC410u, (int32_t) g_terrain(st, S32(st, G_HOVER)), type == 0x8B ? 1 : 0) & 0xFF;
            check = 1;
        } else if (type == 0xD6) {
            r = ghost_477d40(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y));
            check = 1;
        } else if (type >= 0x29 && type <= 0x2B) {
            image += S32(st, G_BUILD_ORIENT);
        }
    }
    if (check && r == 0) {
        red_all = bad = 1;
    }
    if (extra) {
        /* @bin 0x0043BCF2 what the city has: 0x4486, 0x499C, 0x4600, 0x49D0, 0x00705C6C */
        type = S32(st, G_TYPE);
        uint32_t city = CURRENT_CITY(st);
        int32_t v = 0;
        int have = 1;
        if (type == 0xBB || type == 0xBC || type == 0xBD) {
            v = S16(st, city + 0x4486);
        } else if (type == 0x4D || type == 0x4E || type == 0x4F) {
            v = S32(st, city + 0x499C);
        } else if (type == 0x5F) {
            v = S32(st, 0x00705C6Cu);
        } else if (type == 0x32) {
            v = S32(st, city + 0x4600);
        } else if (type == 0x5E) {
            v = S32(st, city + 0x49D0);
        } else {
            have = 0;
        }
        if (have && v != 0) {
            red_all = bad = 1;
        }
    }
    /* @bin 0x0043BE0F the footprint tiles */
    int32_t sx, sy;
    g_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    int32_t y1 = sy + S32(st, G_TILE_H);
    int32_t count = S32(st, G_FOOT_COUNT + 4u * (uint32_t) size);
    int32_t half = S32(st, G_ORIENT) / 2;
    uint32_t row = G_FOOT_TILES + 0x90u * (uint32_t) half;
    uint8_t seen_land = 0, seen_plain = 0;
    uint8_t on_roads = (uint8_t) GAME_CALL(st, 0x0046FDE0u, S32(st, G_TYPE));
    type = S32(st, G_TYPE);
    for (int32_t i = 0; i < count; i++) {
        int32_t off = S32(st, row + 4u * (uint32_t) i) + hov;
        if (g_flooded(st, off)) {
            red_all = bad = 1;
            continue;
        }
        uint32_t t = g_terrain(st, off);
        if (t & 0x10000) {
            if (seen_plain) {
                bad = 1;
            }
            seen_land = 1;
            if (!on_roads) {
                bad = 1;
            }
        } else {
            if (seen_land) {
                bad = 1;
            }
            seen_plain = 1;
        }
        t &= 0xEEFED77Fu;
        if (type == 0x3B || type == 0xAC || type == 0xAD) {
            t &= 0xFF3FBFFFu;
        }
        int test = 1;
        if (type == 0x38 || type == 0x26) {
            t &= ~0x40u;
        } else if (type == 0x8A) {
            t &= 8;
        } else {
            if (type == 0x7F || type == 0x80 || type == 0x77) {
                t &= ~0x40u;
            }
            if (type == 0x7E) {
                if (S32(st, 0x00605A28u) > 8) {
                    test = 0;
                } else {
                    t &= ~0x40u;
                }
            } else {
                if (type == 0x82 || type == 0x83) {
                    t = 0;
                }
                if (type == 0x84 || type == 0x8B || type == 0x85 || type == 0xD6) {
                    test = 0;
                }
            }
        }
        if (test && t != 0) {
            bad = 1;
        }
        if (S16(st, G_FIGURES + 2u * (uint32_t) (S32(st, row + 4u * (uint32_t) i) + hov)) != 0) {
            bad = 1;
        }
    }
    int32_t verdict;
    if (type == 0x26) {
        /* @bin 0x0043BFEE a plaza: on a road, paved */
        verdict = 1;
        if (U8(st, G_TERRAIN + 4u * (uint32_t) (S32(st, row) + hov)) & 0x40) {
            verdict = bad;
        }
        S32(st, CANAL_TILE) = hov;
        if (GAME_CALL0(st, 0x0047B920u) == 0) {
            verdict = 1;
        }
        type = S32(st, G_TYPE);
    } else {
        verdict = bad;
    }
    if (type == 0x8A) {
        uint32_t t = g_terrain(st, S32(st, row) + hov);
        if (!(t & 0x40) || (t & 0x10000) || U8(st, G_SPRITE + (uint32_t) hov) != 0) {
            verdict = 1;
        }
    }
    if (g_treasury_low(st)) {
        red_all = 1;
    } else if (verdict == 0) {
        goto good;
    }
    /* @bin 0x0043C0A1 the footprint tiles one by one */
    for (int32_t i = 0; i < count; i++) {
        uint32_t d = G_FOOT_OFFS + 8u * (uint32_t) i;
        int32_t x = S32(st, d) + sx;
        int32_t y = S32(st, d + 4) - S32(st, G_TILE_H) + y1;
        int32_t off = S32(st, row + 4u * (uint32_t) i) + hov;
        uint32_t t = g_terrain(st, off);
        uint32_t land = t & 0x10000;
        if (S32(st, G_TYPE) == 0x26) {
            S32(st, CANAL_TILE) = off;
            if (GAME_CALL0(st, 0x0047B920u) == 0) {
                t = 1;
            }
        }
        t &= 0xEEFED77Fu;
        type = S32(st, G_TYPE);
        if (type == 0x7E || type == 0x7F || type == 0x80 || type == 0x77) {
            t &= ~0x40u;
        }
        if (S16(st, G_FIGURES + 2u * (uint32_t) off) != 0) {
            t = 1;
        }
        int32_t colour;
        if (red_all || t != 0 || g_flooded(st, off) || (land && !on_roads)) {
            colour = RED;
        } else {
            colour = GREEN;
        }
        g_foot(st, S32(st, IMG_FOOTPRINT), x, y, colour);
    }
    return;
good:
    /* @bin 0x0043C203 the building's image, tinted green */
    type = S32(st, G_TYPE);
    {
        static const int32_t plain[] = {0x7E, 0x84, 0x85, 0x7F, 0x80, 0x77, 0x82, 0x83, 0x8B, 0xD6};
        if (is_one_of(type, plain, 10)) {
            g_foot(st, S32(st, IMG_FOOTPRINT), sx, y1 - S32(st, G_TILE_H), GREEN);
            return;
        }
    }
    int32_t y, img;
    if (type == 0xC3 || type == 0xC2) {
        uint32_t g = type == 0xC3 ? 0x00F564DCu : 0x00F56764u;
        y = y1 - S32(st, G_TILE_H);
        U16(st, G_TINT) = GREEN;
        g_draw(st, S32(st, g), sx, y);
        img = S32(st, g);
    } else if ((GAME_CALLT(st, 0x00BCA350u, 0x00487F80u, type) & 0xFF) != 0) {
        /* @bin 0x0043C2FD a farm: the image by the fertility (0x00489050 / 14) */
        y = y1 - S32(st, G_TILE_H);
        U16(st, G_TINT) = GREEN;
        if ((GAME_CALLT(st, 0x00BCA350u, 0x00489470u, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), 3, 3) & 0xFF) != 0) {
            int32_t base = S32(st, 0x00F56544u);
            img = GAME_CALLT(st, 0x00BCA350u, 0x00489050u, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), 3, 3) / 14 + base;
        } else {
            img = S32(st, 0x00F56B24u);
        }
        g_draw(st, img, sx, y);
    } else if (S32(st, G_TYPE) == 0xCF || S32(st, G_TYPE) == 0xD0) {
        /* @bin 0x0043C385 the obelisks: by 0x00576144 */
        y = y1 - S32(st, G_TILE_H);
        int32_t k = S32(st, 0x00576144u);
        U16(st, G_TINT) = GREEN;
        img = k == 1 ? S32(st, 0x00F56D54u) : k == 2 ? S32(st, 0x00F56D5Cu) : k == 3 ? S32(st, 0x00F56D64u) : 0;
        g_draw(st, img, sx, y);
    } else {
        type = S32(st, G_TYPE);
        if (type == 0x48 || type == 0xC1) {
            /* @bin 0x0043C4BB the image, a top 0x00F566AC + 0x11, the 8 other tiles 0x00F566B4 */
            y = y1 - S32(st, G_TILE_H);
            U16(st, G_TINT) = GREEN;
            g_draw(st, image, sx, y);
            g_top(st, image, sx, y);
            g_draw(st, S32(st, 0x00F566ACu) + 0x11, sx - 4, y - 0x2A);
            for (uint32_t d = 0x005CFCFCu; d < 0x005CFD3Cu; d += 8) {
                int32_t ty = S32(st, d) - S32(st, G_TILE_H) + y1;
                int32_t tx = S32(st, d - 4) + sx;
                g_draw(st, S32(st, 0x00F566B4u), tx, ty);
                g_top(st, S32(st, 0x00F566B4u), tx, ty);
            }
            U16(st, G_TINT) = 0;
            return;
        }
        if (type == 0x0A) {
            U16(st, G_TINT) = GREEN;
            y = y1 - S32(st, G_TILE_H);
            g_draw(st, S32(st, 0x00F5653Cu), sx, y);
            img = S32(st, 0x00F5653Cu);
        } else if (type != 0x47 && type != 0xBF && type >= 0x0B && type <= 0x1D) {
            /* @bin 0x0043C43C the footprint, green */
            for (int32_t i = 0; i < count; i++) {
                uint32_t d = G_FOOT_OFFS + 8u * (uint32_t) i;
                g_foot(st, S32(st, IMG_FOOTPRINT), S32(st, d) + sx, S32(st, d + 4) - S32(st, G_TILE_H) + y1, GREEN);
            }
            return;
        } else if (type != 0x47 && type != 0xBF && type == 0x38) {
            return;
        } else {
            y = y1 - S32(st, G_TILE_H);
            U16(st, G_TINT) = GREEN;
            g_draw(st, image, sx, y);
            img = image;
        }
    }
    g_top(st, img, sx, y);
    U16(st, G_TINT) = 0;
}

void ghost_draw(game_state *st)
{
    /* @bin 0x0043B630 */
    int32_t type = S32(st, G_TYPE);
    U8(st, MONUMENT_FLAG) = (type == 0xE5 || type == 0xEA || type == 0xEB || type == 0xEC) ? 1 : 0;
    if (S32(st, G_HOVER) == 0) {
        return;
    }
    if (S32(st, 0x00E38DCCu) != 0 && U8(st, 0x00605A5Cu) == 0) {
        return; /* the map scrolls */
    }
    if (S32(st, 0x00605994u) != 0 || type <= 0) {
        return;
    }
    if (S32(st, G_EDITOR) == 1) {
        /* @bin 0x0043B69F the editor's types only */
        static const int32_t editor[] = {0x7E, 0x84, 0x85, 0x7F, 0x80, 0x77, 0x82, 0x83, 0x8B, 0xD6, 0x58, 0x59, 0x5D, 0x81};
        if (!is_one_of(type, editor, 14)) {
            return;
        }
    }
    switch (type) {
    case 7: case 0x5A: ghost_43dd70(st, 1, 1); return;
    case 8: ghost_canal(st); return;
    case 0x52: case 0x53: ghost_bridge(st); return;
    case 0x28: case 0x2C: case 0x2D: ghost_43d720(st); return;
    case 0xD2: ghost_441490(st); return;
    case 0xD5: ghost_541c00(st); return;
    case 0xE1: ghost_4e5490(st, 0x006FD5D8u, 0x006FD5D8u + 0x4C, 0x19); return; /* 0x0040D870 */
    case 0xE6: ghost_4e5490(st, 0x006DE650u, 0x006DE650u + 0x4C, 0x10); return; /* 0x00401420 */
    case 0xE3: ghost_4e5490(st, 0x006FD368u, 0x006FD368u + 0x4C, 9); return; /* 0x00416F90 */
    case 0xE5: ghost_545fc0(st, 0x006F9320u, 0x006F9320u + 0x1E5C, 0xA7); return; /* 0x005456E0 */
    case 0xEA: ghost_545fc0(st, 0x006F5648u, 0x006F5648u + 0x1CBC, 0x9E); return; /* 0x00558B80 */
    case 0xEB: ghost_545fc0(st, 0x006DE9E0u, 0x006DE9E0u + 0x4EE0, 0x1B5); return; /* 0x00555E20 */
    case 0xEC: ghost_545fc0(st, 0x006E9188u, 0x006E9188u + 0x5C88, 0x201); return; /* 0x0054F410 */
    case 0xE4: ghost_4e5490(st, 0x006DE548u, 0x006DE548u + 0x4C, 2); return; /* 0x004E6610 */
    case 0xDE: ghost_4debe0(st); return;
    case 0x4A: ghost_43df10(st, 0x20B); return;
    case 0xB7: ghost_4edbb0(st); return;
    case 0x4C: case 0xB5: ghost_43dd70(st, 1, 0); return;
    case 0xB6: ghost_43df10(st, 0x20D); return;
    case 0x4B: ghost_43df10(st, 0x222); return;
    case 0xC0: ghost_43df10(st, 0x20C); return;
    case 5: ghost_road(st); return;
    case 0x81: ghost_43e090(st); return;
    case 0xD3: case 0xD4: ghost_4c2f50(st, type); return;
    case 0xAB: case 0xCA: ghost_4bfcd0(st); return;
    case 0x88: ghost_ferry(st); return;
    case 0x1F: case 0x21: case 0x1E: case 0xD1: ghost_43f5b0(st, type); return;
    default:
        break;
    }
    if (type >= 0x41 && type <= 0x45) {
        ghost_4c0f30(st, type);
        return;
    }
    if (type >= 0x29 && type <= 0x2B) {
        ghost_441f80(st);
        return;
    }
    ghost_generic(st);
}

static int32_t run_ghost(game_state *st, const int32_t *a) { (void) a; ghost_draw(st); return 0; }
static int32_t run_47ed00(game_state *st, const int32_t *a) { return ghost_47ed00(st, a[0], (uint32_t) a[1]); }
static int32_t run_47ed30(game_state *st, const int32_t *a) { return ghost_47ed30(st, a[0], (uint32_t) a[1]); }
static int32_t run_47ed60(game_state *st, const int32_t *a) { return ghost_47ed60(st, a[0]); }
static int32_t run_477d40(game_state *st, const int32_t *a) { return ghost_477d40(st, a[0], a[1]); }
static int32_t run_51c460(game_state *st, const int32_t *a) { return ghost_51c460(st, a[0]); }
static int32_t run_4c0ed0(game_state *st, const int32_t *a) { (void) st; return ghost_4c0ed0(a[0]); }
static int32_t run_43dd70(game_state *st, const int32_t *a) { ghost_43dd70(st, a[0], a[1]); return 0; }
static int32_t run_43df10(game_state *st, const int32_t *a) { ghost_43df10(st, a[0]); return 0; }
static int32_t run_43e090(game_state *st, const int32_t *a) { (void) a; ghost_43e090(st); return 0; }

const ported_function cityview_ghost_ports[] = {
    {0x0043B630u, "ghost_draw", 0, 0, 0, run_ghost, 0},
    {0x0047ED00u, "ghost_47ed00", 2, 32, 0, run_47ed00, 0},
    {0x0047ED30u, "ghost_47ed30", 2, 32, 0, run_47ed30, 0},
    {0x0047ED60u, "ghost_47ed60", 1, 32, 0, run_47ed60, 0},
    {0x00477D40u, "ghost_477d40", 2, 32, 0, run_477d40, 0},
    {0x0051C460u, "ghost_51c460", 1, 32, 0, run_51c460, 0},
    {0x004C0ED0u, "ghost_4c0ed0", 1, 32, 0, run_4c0ed0, 0},
    {0x0043DD70u, "ghost_43dd70", 2, 0, 0, run_43dd70, 0},
    {0x0043DF10u, "ghost_43df10", 1, 0, 0, run_43df10, 0},
    {0x0043E090u, "ghost_43e090", 0, 0, 0, run_43e090, 0},
};
const int cityview_ghost_ports_count = sizeof(cityview_ghost_ports) / sizeof(cityview_ghost_ports[0]);
