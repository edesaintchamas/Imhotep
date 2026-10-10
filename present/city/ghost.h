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
#include "sim/call.h"
#include "state/records.h"

/* @bin globals of the construction (docs/commands.md 2) */
#define G_TYPE 0x006059E8u        /* i32 the building type chosen */
#define G_DRAGGING 0x006059E4u    /* i32 1: a construction drag is in progress */
#define G_HOVER 0x00BCA300u       /* i32 the hovered tile's grid offset (0: none) */
#define G_HOVER_X 0x00BCA304u
#define G_HOVER_Y 0x00BCA308u
#define G_ORIENT 0x00BCA2E0u      /* i32 the view orientation 0, 2, 4, 6 */
#define G_CAMERA_TX 0x00BCA2E8u   /* i32 the camera tile */
#define G_CAMERA_TY 0x00BCA2ECu
#define G_MAP_W 0x00BCA2D0u
#define G_MAP_H 0x00BCA2D4u
#define G_MAP_START 0x00BCA2D8u
#define G_EDITOR 0x00605A3Cu      /* i32 1: the map editor */
#define G_BUILD_ORIENT 0x00BC9714u /* i32 the building orientation (R key) */
#define G_MODELS 0x005D06DCu      /* 0x14 bytes per type: +0 size, +8 image group, +0xC image offset */
#define G_GROUPS 0x00F55BB4u      /* i32 per image group (8 bytes): the group's first image */
#define G_TINT 0x0070E120u        /* u16 the tint of the tinted blitters */
#define G_TILE_H 0x0070E44Cu      /* i32 the tile's half height (view) */
#define G_TILE_W 0x0070E45Cu      /* i32 the tile's half width - 1 (view) */
#define G_TILE_TOP 0x0070E448u
#define G_TERRAIN 0x00AF598Cu     /* u32 per tile */
#define G_EDGE 0x00B285CCu        /* u8 per tile */
#define G_IMAGE 0x00B7482Cu       /* i32 per tile */
#define G_FIGURES 0x00B350DCu     /* i16 per tile */
#define G_BUILDINGS 0x00B4E6FCu   /* i16 per tile */
#define G_SPRITE 0x00ADC36Cu      /* u8 per tile */
#define G_FLOOD 0x00D05720u       /* u32 per tile: a flood-plain record (+0x18 u8) or 0 */
#define G_FOOT_OFFS 0x005CFCF0u   /* (dx, dy) i32 pairs: the tiles of a footprint on the screen */
#define G_FOOT_COUNT 0x005CFCD4u  /* i32 per size: the tile count */
#define G_FOOT_TILES 0x005CFE10u  /* i32[36] per orientation / 2: the footprint's grid offsets */
#define IMG_FOOTPRINT 0x00F56BE4u /* i32 the plain tile footprint image */

#define GREEN 0x1FE3
#define RED ((int32_t) 0xFFFFF863u)

/* @bin 0x0053E590 / 0x0053F250 / 0x0053F4F0 (thiscall on 0x00F55BB0): the image draw, the top part
 * of a building image, the tinted footprint (package blit) */
static inline void g_draw(game_state *st, int32_t image, int32_t x, int32_t y)
{
    GAME_CALLT(st, GFX, IMAGE_DRAW, image, x, y);
}

static inline void g_top(game_state *st, int32_t image, int32_t x, int32_t y)
{
    GAME_CALLT(st, GFX, 0x0053F250u, image, x, y);
}

static inline void g_foot(game_state *st, int32_t image, int32_t x, int32_t y, int32_t colour)
{
    GAME_CALLT(st, GFX, 0x0053F4F0u, image, x, y, colour);
}

static inline int32_t g_group(game_state *st, int32_t group)
{
    return S32(st, G_GROUPS + 8u * (uint32_t) group);
}

static inline uint32_t g_terrain(game_state *st, int32_t off)
{
    return U32(st, G_TERRAIN + 4u * (uint32_t) off);
}

static inline int32_t g_treasury_low(game_state *st)
{
    /* @bin cmp city +0xC (0x00EA187C + player * 0x49D8), -5000: jg */
    return S32(st, CURRENT_CITY(st) + 0xC) <= -5000;
}

/* a flood-plain tile whose record is not dry (+0x18 = 0) */
static inline int g_flooded(game_state *st, int32_t off)
{
    uint32_t p = U32(st, G_FLOOD + 4u * (uint32_t) off);
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
