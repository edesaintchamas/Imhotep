/* Imhotep - the construction ghost 0x0043B630 and its helpers (package cityview, internal).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Shared by present/city/ghost*.c: the globals the ghost reads and the draw calls (package blit's
 * primitives through GAME_CALLT on the graphics object 0x00F55BB0). */
#ifndef IMHOTEP_PRESENT_CITY_GHOST_H
#define IMHOTEP_PRESENT_CITY_GHOST_H

#include <stdint.h>

#include "city.h"
#include "frame/view.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/records.h"

#define GREEN 0x1FE3
#define RED ((int32_t) 0xFFFFF863u)

/* @bin 0x0053E590 / 0x0053F250 / 0x0053F4F0 (thiscall on 0x00F55BB0): the image draw, the top part
 * of a building image, the tinted footprint (package blit) */
static inline void g_draw(game_state *st, int32_t image, int32_t x, int32_t y)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, image, x, y);
}

static inline void g_top(game_state *st, int32_t image, int32_t x, int32_t y)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW_TOP, image, x, y);
}

static inline void g_foot(game_state *st, int32_t image, int32_t x, int32_t y, int32_t colour)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW_TINTED, image, x, y, colour);
}

static inline int32_t g_group(game_state *st, int32_t group)
{
    return S32(st, G_IMAGE_GROUP((uint32_t) group));
}

static inline uint32_t g_terrain(game_state *st, int32_t off)
{
    return U32(st, G_GRID_TERRAIN + 4u * (uint32_t) off);
}

static inline int32_t g_treasury_low(game_state *st)
{
    /* @bin cmp city +0xC (0x00EA187C + player * 0x49D8), -5000: jg */
    return S32(st, CURRENT_CITY(st) + CITY_TREASURY) <= -5000;
}

/* a flood-plain tile whose record is not dry (+0x18 = 0) */
static inline int g_flooded(game_state *st, int32_t off)
{
    uint32_t p = U32(st, G_FLOODPLAIN_MANAGER + 4u * (uint32_t) off);
    return p != 0 && P8(st, p + 0x18) == 0;
}

/* @bin 0x004F0BA0 the hovered tile's screen position (core/frame/view.c) */
static inline void g_screen(game_state *st, int32_t tx, int32_t ty, int32_t *x, int32_t *y)
{
    *x = 0;
    *y = 0;
    view_tile_to_screen(st, tx, ty, x, y);
}

/* ghost.c */
void ghost_draw(game_state *st); /* 0x0043B630 */
int32_t ghost_47ed00(game_state *st, int32_t off, uint32_t mask);
int32_t ghost_47ed30(game_state *st, int32_t off, uint32_t mask);
int32_t ghost_47ed60(game_state *st, int32_t off);
int32_t ghost_477d40(game_state *st, int32_t x, int32_t y);
int32_t ghost_51c460(game_state *st, int32_t off);
int32_t ghost_4c0ed0(int32_t type);
void ghost_43dd70(game_state *st, int32_t unused, int32_t floodable);
void ghost_43df10(game_state *st, int32_t group);
void ghost_43e090(game_state *st);
/* ghost_ferry.c */
void ghost_ferry(game_state *st);
/* ghost_big.c */
void ghost_43d720(game_state *st);
void ghost_43f5b0(game_state *st, int32_t type);
void ghost_441490(game_state *st);
void ghost_441f80(game_state *st);
/* ghost_gate.c */
void ghost_4bfcd0(game_state *st);
void ghost_4c0040(game_state *st, int32_t x, int32_t y, int32_t rot);
void ghost_4c0f30(game_state *st, int32_t type);
void ghost_4c2f50(game_state *st, int32_t type);
/* ghost_monument.c */
void ghost_4e5490(game_state *st, uint32_t self, uint32_t parts, int32_t count);
void ghost_545fc0(game_state *st, uint32_t self, uint32_t parts, int32_t count);
void ghost_541c00(game_state *st);
void ghost_4debe0(game_state *st);
void ghost_4edbb0(game_state *st);

#endif
