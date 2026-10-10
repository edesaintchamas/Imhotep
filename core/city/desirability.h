/* Imhotep - daily desirability update (Pharaoh.exe 0x0046A0C0 + 0x0046A140, tick 37 of each day).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Rules: docs/formats/desirability.md. Tests: tests/differential/desirability_check.c (stored
 * grids), tools/oracle/harness.py (the original functions on the same state). */
#ifndef IMHOTEP_CITY_DESIRABILITY_H
#define IMHOTEP_CITY_DESIRABILITY_H

#include "state/layout/g_building.h"
#include "state/state.h"

#define DESIRABILITY_GRID_SIDE 228

/* Variables read and written (state/layout.h): G_DESIRABILITY_GRID (i8 per tile, chunk 16, written),
 * G_TILE_CURSOR (the grid offset of the last footprint, written), G_DAY2_DESTROY_BITS (u8 per tile,
 * chunk 13: bit 0x80 may be cleared), G_GRID_TERRAIN (chunk 10), the buildings (chunk 32),
 * G_BUILDING_HIGHEST (chunk 93), G_MODEL_BUILDING (0x28-byte records, model.md) and the map size
 * (G_MAP_WIDTH, G_MAP_HEIGHT, G_MAP_START, G_MAP_STRIDE_EXTRA). The two names below are kept for
 * the differential test. */
#define DESIRABILITY_GRID G_DESIRABILITY_GRID
#define MAX_BUILDING_ID G_BUILDING_HIGHEST

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
