/* Imhotep - daily desirability update (Pharaoh.exe 0x0046A0C0 + 0x0046A140, tick 37 of each day).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Rules: docs/formats/desirability.md. Tests: tests/differential/desirability_check.c (stored
 * grids), tools/oracle/harness.py (the original functions on the same state). */
#ifndef IMHOTEP_CITY_DESIRABILITY_H
#define IMHOTEP_CITY_DESIRABILITY_H

#include "state/state.h"

#define DESIRABILITY_GRID_SIDE 228

/* Variables read and written (Pharaoh.exe addresses, game_state). */
#define DESIRABILITY_GRID 0x00AC2D4Cu      /* i8 per tile, chunk 16 (written) */
#define DESIRABILITY_BASE 0x00BC9768u      /* grid offset of the last footprint (written) */
#define BITFIELDS_GRID 0x00B285CCu         /* u8 per tile, chunk 13 (bit 0x80 may be cleared) */
#define TERRAIN_GRID 0x00AF598Cu           /* u32 per tile, chunk 10 */
#define BUILDINGS 0x007999D8u              /* 4000 x 264 bytes, chunk 32 */
#define BUILDING_SIZE 264
#define MAX_BUILDING_ID 0x0078D790u        /* chunk 93 */
#define MODEL_BUILDING 0x00705EA4u         /* building model: 0x28-byte records (model.md) */
#define MAP_WIDTH 0x00BCA2D0u              /* map size and grid offsets, set at scenario start */
#define MAP_HEIGHT 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u              /* grid offset of (0, 0) */
#define MAP_ROW_SKIP 0x00BCA2DCu           /* 228 - width */

/* @bin 0x0046A0C0 clear the grid, then every building's rings. */
void desirability_buildings(game_state *st);
/* @bin 0x0046A140 gardens, plazas and rubble tiles (and the plaza-bit clean-up). */
void desirability_tiles(game_state *st);
/* Both, in the order of the tick dispatcher 0x00451C50 (step 37). */
void desirability_update(game_state *st);

/* Same as desirability_buildings over buildings 1..last_id only (the test's explanation of grids
 * saved before buildings placed since the last update). */
void desirability_buildings_upto(game_state *st, int32_t last_id);

#endif
