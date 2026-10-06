/* Imhotep - terrain grid passes reached by the map refresh 0x00486E70 on a new-game start (the
 * 0x10000 terrain flag spread over the map and its neighbour updates, the 0x80 marks and their
 * strength byte 0x00A509BC) and the two 0x00B67D1C mark rectangles of 0x0046D480.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_NEWGAME_START2_H
#define IMHOTEP_NEWGAME_START2_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00478420 0x00478330 over the whole map */
void terrain_0x00478420(game_state *st);
/* @bin 0x00478330 per tile of the rectangle: 4 -> 0x00477F60, 0x10000 -> 0x00477F70,
 * 0x22341A33 -> 0x00478090, else 0x00478270 */
void terrain_0x00478330(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
/* @bin 0x00477F60 returns 0 */
int32_t terrain_0x00477f60(game_state *st);
/* @bin 0x00477F70 the 0x10000 flag spread around (x, y), recursively; 1 if a tile changed */
int32_t terrain_0x00477f70(game_state *st, int32_t x, int32_t y);
/* @bin 0x00478090 the 0x10000 flag cleared around (x, y), recursively; 1 if a tile changed */
int32_t terrain_0x00478090(game_state *st, int32_t x, int32_t y);
/* @bin 0x00478270 the 0x10000 flag of (x, y)'s best neighbours moved to 0x80, recursively */
int32_t terrain_0x00478270(game_state *st, int32_t x, int32_t y);
/* @bin 0x004781A0 the offset of the neighbour of tile `tile` with `mask` and the most free sides */
int32_t terrain_0x004781a0(game_state *st, int32_t tile, uint32_t mask);
/* @bin 0x00477EB0 0 if `mask` closes tile `tile` on a side (or a diagonal, with `diag`), else 1 */
int32_t terrain_0x00477eb0(game_state *st, int32_t tile, uint32_t mask, int32_t diag);
/* @bin 0x00477DD0 the terrain image passes over (x, y) +/- r */
void terrain_0x00477dd0(game_state *st, int32_t x, int32_t y, int32_t r);
/* @bin 0x00477430 0x00479AC0(x, y, 1) for the tiles with 0x2000000 and no 0x600 */
void terrain_0x00477430(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
/* @bin 0x00479AC0 the 0x2000000 flag of a tile inside the map set (mode 1) or kept alone */
void terrain_0x00479ac0(game_state *st, int32_t x, int32_t y, int32_t mode);

/* @bin 0x00487450 the 0x80 marks and their strength (0x00A509BC) over the rectangle */
void terrain_0x00487450(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
/* @bin 0x00486D30 the first ring (1 .. maxr - 1) around (x, y) holding a `mask` tile, or -1 */
int32_t terrain_0x00486d30(game_state *st, int32_t x, int32_t y, uint32_t mask, int32_t maxr);
/* @bin 0x00486EC0 (int) (a / b * 96.0) - 1 (x87) after its clamps */
int32_t terrain_0x00486ec0(game_state *st, int32_t a, int32_t b);
/* @bin 0x00486F20 the neighbour flags 0x00BC97C0..DC of a marked tile, then 0x00484F40 */
int32_t terrain_0x00486f20(game_state *st, int32_t tile);
/* @bin 0x00487260 tile between two pointer-grid objects (N-S or W-E) with +0x18 == 0 */
int32_t terrain_0x00487260(game_state *st, int32_t tile);
/* @bin 0x004872C0 tile between two `mask` tiles (N-S or W-E) */
int32_t terrain_0x004872c0(game_state *st, int32_t tile, uint32_t mask);
/* @bin 0x00487300 the strength byte of a tile, or 100 when closed */
int32_t terrain_0x00487300(game_state *st, int32_t tile);
/* @bin 0x00487390 the strength of tile + d, 100 when closed or occupied, 0 when >= 0x80 */
int32_t terrain_0x00487390(game_state *st, int32_t tile, int32_t d);

/* @bin 0x00482A90 0x00B67D1C |= 0x80 over (x, y, size) grown by r */
void terrain_0x00482a90(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r);
/* @bin 0x00482B50 0x00B67D1C &= 0x7F over (x, y, size) grown by r */
void terrain_0x00482b50(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r);

#endif
