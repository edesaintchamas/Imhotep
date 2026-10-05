/* Imhotep - terrain image rebuild at scenario start (Pharaoh.exe 0x004D52A0 / 0x004DEA50).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Rules: docs/formats/map.md §6. Differential test: tests/differential/terrain_check.c. */
#ifndef IMHOTEP_MAP_TERRAIN_H
#define IMHOTEP_MAP_TERRAIN_H

#include <stdint.h>

#include "graphics/image_groups.h"
#include "io/map_file.h"
#include "state/state.h"

#define GRID_SIDE 228
#define GRID_SIZE (GRID_SIDE * GRID_SIDE)
#define FLOODPLAIN_MAX_DEPTH 30

typedef struct {
    int32_t land_0x10000000_unported;
    int32_t floodplain_objects;
    int32_t signposts_placed;
} terrain_stats;

/* The terrain code works on the game state (core/state/state.h): grids, map geometry, image
 * group table, pattern tables and matcher variables at the original's addresses. */
typedef struct {
    game_state *g;
    terrain_stats stats;
} terrain_state;

/* Image group table filled at start-up (0x00F55BB0 graphics object +4 + 8 * group: first image
 * id): from the startup numbering of image_groups. @diff terrain_check (915 / 915 groups equal to
 * the table after the original's start-up, tools/oracle) */
void terrain_store_groups(game_state *g, const image_groups *groups);

/* Load the grids of a parsed map into the state at their addresses, and the map globals.
 * Returns 0, or -1 if a needed chunk is missing or too small. */
int terrain_init(terrain_state *t, game_state *g, const map_file *m, const image_groups *groups);

/* Same for a campaign scenario record of mission1.pak (saved-game chunk layout). */
int terrain_init_campaign(terrain_state *t, game_state *g, const map_file *m, const image_groups *groups);

/* @bin 0x004DEA50 terrain image rebuild (every sub-pass). */
void terrain_rebuild(terrain_state *t);

/* @bin 0x004BE160 (this = 0x00D05720) floodplain build over a rectangle, then 0x004DEA50. */
void terrain_floodplain_build(terrain_state *t, int32_t x0, int32_t y0, int32_t x1, int32_t y1);

/* @bin 0x004D52A0: zero the image grid, rebuild, floodplain build (which rebuilds again). */
void terrain_scenario_start(terrain_state *t);

#endif
