/* Imhotep - named globals: the map, its grids and the road routing.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Grids: GRID_SIZE x GRID_SIZE tiles (state/records.h), one entry per grid offset.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_MAP_H
#define IMHOTEP_STATE_LAYOUT_G_MAP_H

/* @bin 0x0046F7B0 the map globals at scenario start (docs/formats/state.md) */
#define G_MAP_WIDTH 0x00BCA2D0u  /* i32 */
#define G_MAP_HEIGHT 0x00BCA2D4u /* i32 */
#define G_MAP_START 0x00BCA2D8u  /* i32: grid offset of the map's tile (0, 0) */
#define G_VIEW_ORIENTATION 0x00BCA2E0u /* @bin 0x00417490 i32: the view rotation, 0 2 4 6 */

/* grids */
#define G_GRID_TERRAIN 0x00AF598Cu      /* @bin 0x00481210 u32 terrain flags per tile (0x40: road) */
#define G_GRID_ROAD_NETWORK 0x00A76AECu /* @bin 0x00457AF0 u8 road network id per tile */
#define G_GRID_ROUTING 0x00EE0C50u      /* @bin 0x00517A40 i8 per tile: passable when >= 0 */
#define G_GRID_ROAD_DISTANCE 0x00EED760u /* @bin 0x00517A40 i32 per tile: distance + 1 from the search start */

/* @bin 0x00517A40 the breadth-first road search */
#define G_ROUTE_QUEUE 0x00F203A0u      /* i32[GRID_TILES] tiles, a ring */
#define G_ROUTE_QUEUE_HEAD 0x00EAE00Cu /* i32 */
#define G_ROUTE_QUEUE_TAIL 0x00EAE008u /* i32 */
#define G_ROUTE_START 0x00EAE004u      /* i32: the start tile */
#define G_ROUTE_SEARCHES 0x00EADFD4u   /* i32: searches made */

/* scratch tiles of the neighbour and road helpers */
#define G_TILE_CURSOR 0x00BC9768u  /* @bin 0x0047FEB0 i32: the tile a search is at (the grid offset of the road found) */
#define G_ROAD_ACCESS_X 0x00D3B4E4u /* @bin 0x00487D00 i32: the road tile found by the access helpers */
#define G_ROAD_ACCESS_Y 0x00D3B4E0u /* i32 */

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_M_VARIANT 0x00BC9740u /* core/map/last_terrain.c */
#define G_M_EXTRA 0x00BC9744u /* core/map/last_terrain.c */
#define G_M_OFFSET 0x00BC9748u /* core/map/last_terrain.c */
#define G_SCRATCH 0x00BC97C0u /* core/figure/actions_a3.c: per direction (0 .. 7): non-zero when open (0x0047E340) */
#define G_MAP_STRIDE_EXTRA 0x00BCA2DCu /* core/city/desirability.h: 228 - width */
#define G_PYRAMIDS 0x00EAAC30u /* core/map/terrain_land.c */
#define G_PYRAMID_COUNT 0x00EAAF64u /* core/map/terrain_land.c */
#define G_SEARCH_COUNT2 0x00EADFD8u /* core/map/route_army.c */
#define G_VISITS 0x00EAE010u /* core/map/route_search.c: u8 per tile: times a -2 tile was taken from the queue */
#define G_PATH_BUFFER 0x00F533C8u /* core/map/route_path.c: u8 x 500: the directions, last step first */
#define G_CANAL_BASE 0x00F56B84u /* core/map/last_terrain.c */
/* --- end harvested --- */

#endif
