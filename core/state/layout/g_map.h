/* Imhotep - named globals: the map, its grids, the terrain images and the routing.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Grids: GRID_SIZE x GRID_SIZE tiles (state/records.h), one entry per grid offset.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_MAP_H
#define IMHOTEP_STATE_LAYOUT_G_MAP_H

/* @bin 0x0046F7B0 the map globals at scenario start (docs/formats/state.md, map.md §5) */
#define G_MAP_WIDTH 0x00BCA2D0u  /* i32 */
#define G_MAP_HEIGHT 0x00BCA2D4u /* i32 */
#define G_MAP_START 0x00BCA2D8u  /* i32: grid offset of the map's tile (0, 0) */
#define G_MAP_STRIDE_EXTRA 0x00BCA2DCu /* @bin 0x004D52A0 i32: 228 - width, added to the offset after each row */
#define G_VIEW_ORIENTATION 0x00BCA2E0u /* @bin 0x00417490 i32: the view rotation, 0 2 4 6 */
#define G_MAP_XE 0x00BCA2F0u     /* @bin 0x0046F7B0 i32: the map's last column (x0 + width - 1) */
#define G_MAP_CENTER_X 0x00BCA2F8u /* @bin 0x0046F7B0 i32: (x0 + x1) / 2, the diamond's centre */
#define G_MAP_CENTER_Y 0x00BCA2FCu /* @bin 0x0046F7B0 i32: (y0 + y1) / 2 */
/* @bin 0x0046F7B0 per map row, i32: the first column, the end column (excluded), the first tile's grid
 * offset (the playable diamond) */
#define G_ROW_OFFSET 0x00BC9804u
#define G_ROW_END 0x00BC9B94u
#define G_ROW_FIRST 0x00BC9F24u
#define G_UBCA30C 0x00BCA30Cu    /* @bin 0x004D52A0 u8 := 1 for files below version 0x8D, with 0x00486E70 (which runs
                                  * when it is set: core/building/day_state.c) */

/* grids (save.md chunks) */
#define G_GRID_TERRAIN 0x00AF598Cu      /* @bin 0x00481210 u32 terrain flags per tile (0x40: road) */
#define G_GRID_ROAD_NETWORK 0x00A76AECu /* @bin 0x00457AF0 u8 road network id per tile */
#define G_GRID_ROUTING 0x00EE0C50u      /* @bin 0x00517A40 i8 per tile: passable when >= 0 */
#define G_GRID_ROAD_DISTANCE 0x00EED760u /* @bin 0x00517A40 i32 per tile: distance + 1 from the search start */
#define G_GRID_FERTILITY 0x00A5D4CCu    /* @bin 0x00477500 u8 per tile: soil fertility (chunk 135) */
#define G_GRID_MOISTURE 0x00A509BCu     /* @bin 0x00477500 u8 per tile: moisture (chunk 170) */
#define G_GRID_DAMAGE 0x00A9CC1Cu       /* @bin 0x004690F0 i16 per tile: damage counter (chunk 18) */
#define G_GRID_RUBBLE 0x00A9010Cu       /* @bin 0x0047C480 u8 per tile: rubble building type (chunk 168) */

/* @bin 0x00517A40 the breadth-first road search */
#define G_ROUTE_QUEUE 0x00F203A0u      /* i32[GRID_TILES] tiles, a ring */
#define G_ROUTE_QUEUE_HEAD 0x00EAE00Cu /* i32 */
#define G_ROUTE_QUEUE_TAIL 0x00EAE008u /* i32 */
#define G_ROUTE_START 0x00EAE004u      /* i32: the start tile */
#define G_ROUTE_SEARCHES 0x00EADFD4u   /* i32: searches made (chunk 120, debug 'routings') */
#define G_ROUTE_SEARCHES_ENEMY 0x00EADFD8u /* @bin 0x0051A420 i32: enemy searches made (chunk 119, debug 'barb') */
#define G_VISITS 0x00EAE010u           /* @bin 0x0051CE90 u8 per tile: times a -2 tile was taken from the queue */
#define G_PATH_BUFFER 0x00F533C8u      /* @bin 0x0051E720 u8 x 500: the directions found, last step first */

/* scratch tiles of the neighbour and road helpers */
#define G_TILE_CURSOR 0x00BC9768u  /* @bin 0x0047FEB0 i32: the tile a search is at (the grid offset of the road found) */
#define G_NEIGHBOUR_FLAGS 0x00BC97C0u /* @bin 0x0047DB00 i32 per direction 0..7 (16 cleared by 0x00475FD0), north then
                                       * clockwise: the neighbour tests' results */
#define G_ROAD_ACCESS_X 0x00D3B4E4u /* @bin 0x00487D00 i32: the road tile found by the access helpers */
#define G_ROAD_ACCESS_Y 0x00D3B4E0u /* i32 */

/* the terrain image passes (map.md §6) */
#define G_MATCH_VARIANT 0x00BC9740u /* @bin 0x00484BE0 i32: the pattern matcher's variant */
#define G_MATCH_EXTRA 0x00BC9744u   /* @bin 0x00484BE0 i32: its extra byte (canals: the aqueduct grid value) */
#define G_MATCH_OFFSET 0x00BC9748u  /* @bin 0x00484BE0 i32: its image offset */
#define G_CANAL_BASE 0x00F56B84u    /* @bin 0x0047BAE0 i32: the first image of the canal group (506) */
#define G_ALL_ROADS_PAVED 0x00978D74u /* @bin 0x0047B670 i32: 1 = every road drawn paved */
#define G_UBCA328 0x00BCA328u       /* @bin 0x0047BAE0 i32 := 0 after the canals rectangle */
/* the neighbour pattern tables (map.md §6, core/map/terrain_tables.c): entries whose byte +0x0E is
 * cleared by 0x0046FA40 */
/* their 16-byte entries (@bin 0x00484BE0): +0..+7 the neighbours (0, 1; 2: any) */
#define PATTERN_OFFSET 0x08u   /* u8[4]: the image offset, by orientation / 2 */
#define PATTERN_EXTRA 0x0Cu    /* u8 */
#define PATTERN_VARIANTS 0x0Du /* u8: the variant count */
#define PATTERN_COUNTER 0x0Eu  /* u8: the cycling variant (cleared by 0x00484EE0) */
#define G_PATTERN_SHORE 0x005CA658u
#define G_U5CA958 0x005CA958u       /* @bin 0x0046FA40 a pattern table, 0x30 entries */
#define G_PATTERN_FLOODPLAIN 0x005CAC58u
#define G_PATTERN_WALLS 0x005CB9D8u
#define G_PATTERN_WALLS_BUILDING 0x005CBCD8u
#define G_U5CBDC8 0x005CBDC8u       /* @bin 0x0046FA40 a pattern table, 0x0E entries */
#define G_PATTERN_SPECIAL_ROCKS 0x005CBEA8u
#define G_PATTERN_DEEP 0x005CBFB8u
#define G_U5CC158 0x005CC158u       /* @bin 0x0046FA40 a pattern table, 0x11 entries */
#define G_PATTERN_ROADS 0x005CC268u
#define G_PATTERN_PAVED_ROADS 0x005CC378u /* @bin 0x0047B670 the paved road pattern */
#define G_PATTERN_CANALS 0x005CC678u
#define G_CLIFF_IMAGES 0x005DBCC8u  /* @bin 0x00475FD0 i32[14][4]: cliff image index per shape, by orientation / 2
                                     * (map.md: tables 0x005DBCC8-0x005DBD98) */

/* @bin 0x00481F20 the tile rings around a footprint of size n (1..6): per ring, the count of tiles
 * (i16), the (dx, dy) byte pairs of the sources and the packed offsets dx + 228 dy (i16) */
#define G_RINGS_1X1 0x005DBBD8u     /* @bin 0x0046F8D0 u32[12]: where the 1x1 rings 1..12 are written */
#define G_RINGS_1X1_DATA 0x0070BAACu /* @bin 0x0046F8D0 the 1x1 rings' pairs, ring 12 at the lowest address */
#define G_RING_COUNTS_1 0x005C9B00u /* i16[12] */
#define G_RING_COUNTS_2 0x005C9B18u /* i16[6] */
#define G_RING_COUNTS_3 0x005C9B24u
#define G_RING_COUNTS_4 0x005C9B30u
#define G_RING_COUNTS_5 0x005C9B3Cu
#define G_RING_COUNTS_6 0x005C9B48u
#define G_RING_SOURCES_2 0x005C9BA8u /* rings 1..6 in order */
#define G_RING_SOURCES_3 0x005C9D28u
#define G_RING_SOURCES_4 0x005C9ED8u
#define G_RING_SOURCES_5 0x005CA0B8u
#define G_RING_SOURCES_6 0x005CA2C8u
#define G_RING_OFFSETS_1 0x0070C8ECu
#define G_RING_OFFSETS_2 0x0070C76Cu
#define G_RING_OFFSETS_3 0x0070C5BCu
#define G_RING_OFFSETS_4 0x0070C3DCu
#define G_RING_OFFSETS_5 0x0070C1CCu
#define G_RING_OFFSETS_6 0x0070BF8Cu

/* the pyramids' tiles (0x004F3D70, 0x004F3F70, 0x004F60D0) */
#define G_PYRAMID_REDRAW 0x00EAAC30u /* @bin 0x004F60D0 i32[4]: pyramids (main building ids) to redraw */
#define G_PYRAMID_REDRAW_COUNT 0x00EAAF64u /* @bin 0x004F60D0 i32 */
#define G_UEAAF60 0x00EAAF60u        /* @bin 0x004F3F70 i32: the image offset of tile state 3 (entry 0 of a table) */
#define G_MONUMENT_EDGE_IMAGES_5 0x005F6D70u /* @bin 0x004F3F70 i32 per neighbour mask: image offset, state 5 */
#define G_MONUMENT_EDGE_IMAGES 0x005F7170u   /* @bin 0x004F3F70 i32 per neighbour mask: image offset, states 6..10 */
#define G_MONUMENT_NEIGHBOURS 0x005F7570u    /* @bin 0x004F3F70 i32 grid offset per direction 0..7 */

/* the entry and exit signposts (0x0047D870; save.md chunks 122-125, 132, 133) */
#define G_ENTRY_FLAG_X 0x00BCA318u /* i32 */
#define G_ENTRY_FLAG_Y 0x00BCA31Cu /* i32 */
#define G_EXIT_FLAG_X 0x00BCA320u  /* i32 */
#define G_EXIT_FLAG_Y 0x00BCA324u  /* i32 */
#define G_ENTRY_FLAG_TILE 0x00BC96C4u /* @bin 0x0047D870 i32: the entry signpost's grid offset */
#define G_EXIT_FLAG_TILE 0x00BC96C0u  /* @bin 0x0047D870 i32: the exit signpost's grid offset */

/* the floodplain objects (@bin 0x004BD610, 0x1C bytes on the heap, one per floodplain tile: the pointer
 * grid of the manager G_UD05720; map.md "Floodplain build"); +0: the list */
#define FLOODOBJ_NODE 0x04u  /* u32: its node in the list */
#define FLOODOBJ_TILE 0x08u  /* i32: the grid offset */
#define FLOODOBJ_X 0x0Cu     /* i32 */
#define FLOODOBJ_Y 0x10u     /* i32 */
#define FLOODOBJ_DEPTH 0x14u /* i32 */
#define FLOODOBJ_FLAG 0x18u  /* u8: the tile had terrain 0x10000 */

/* @bin 0x00451C50 i32: 3 with the city record's CITY_U4998 == 2 resets it (tick); the flood state per
 * core/building/building_extra.c, the game mode per core/figure/actions_a.c: not settled */
#define G_UD386D4 0x00D386D4u

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
/* --- end harvested --- */

#endif
