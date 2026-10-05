/* Imhotep - the game's record arrays in the state (addresses in Pharaoh.exe, docs/formats/save.md).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Ports address a record field as FIGURE_ADDR(id) + offset (field offsets: docs/formats/save.md §5,
 * named in the port where their meaning is established). */
#ifndef IMHOTEP_STATE_RECORDS_H
#define IMHOTEP_STATE_RECORDS_H

#include <stdint.h>

#include "state/state.h"

/* @bin chunk 21: 2000 figures of 0x184 bytes (record 0 unused) */
#define FIGURE_BASE 0x00BCD5F8u
#define FIGURE_SIZE 0x184u
#define FIGURE_COUNT 2000
#define FIGURE_ADDR(id) (FIGURE_BASE + FIGURE_SIZE * (uint32_t) (id))

/* @bin chunk 32: 4000 buildings of 0x108 bytes */
#define BUILDING_BASE 0x007999D8u
#define BUILDING_REC_SIZE 0x108u
#define BUILDING_COUNT 4000
#define BUILDING_ADDR(id) (BUILDING_BASE + BUILDING_REC_SIZE * (uint32_t) (id))

/* @bin chunk 28: city data, one record of 0x49D8 bytes per player; the current player 0x00EAAC28 */
#define CITY_BASE 0x00EA1870u
#define CITY_REC_SIZE 0x49D8u
#define CURRENT_PLAYER_ADDR 0x00EAAC28u
#define CITY_ADDR(player) (CITY_BASE + CITY_REC_SIZE * (uint32_t) (player))
#define CURRENT_CITY(st) CITY_ADDR(U32(st, CURRENT_PLAYER_ADDR))

/* @bin map grids: 228 x 228 tiles, one entry per tile at grid offset y * 228 + x */
#define GRID_SIZE 228
#define GRID_TILES (GRID_SIZE * GRID_SIZE)

#endif
