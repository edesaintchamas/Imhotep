/* Imhotep - named globals: buildings (the record array itself: state/records.h, its fields:
 * layout/building.h) and the per-type tables of the exe.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_BUILDING_H
#define IMHOTEP_STATE_LAYOUT_G_BUILDING_H

/* @bin 0x0046A400 the constructor (save.md §5 "Buildings"; chunks 93, 117) */
#define G_BUILDING_HIGHEST 0x0078D790u  /* i32: highest building id in use (chunk 93) */
#define G_BUILDING_SEQUENCE 0x0078D7B4u /* i32: next creation sequence number, BLD_SEQUENCE (chunk 117) */
/* @bin 0x00488030, 0x00488050 thiscall on it */
#define G_BUILDING_MANAGER 0x00BCA350u

/* per-type tables (exe data): i32 entries, index the building type */
#define G_TYPE_LABOUR_CATEGORY 0x005D06ECu /* @bin 0x00459670 property table 0x005D06DC + 0x10, stride 0x14 */
#define G_TYPE_LABOUR_STRIDE 0x14u
#define G_TYPE_WORKERS 0x00705EB8u         /* @bin 0x00459670 workers needed (save.md: per-type table +0) */
#define G_TYPE_TABLE_STRIDE 0x28u          /* @bin 0x00468B70 save.md: 0x00705EB8 + type * 0x28 */
#define G_TYPE_COUNT 0x0070573Cu           /* @bin 0x00461950 i32 per type: the number of such buildings */
#define G_U705C10 0x00705C10u              /* @bin 0x004C4F05 i32: tested with G_U705DF4 by a small wrath of god 2 */
#define G_U705DF4 0x00705DF4u

/* house levels (exe data): 0x0070CDCC + level * 0x58 (save.md §5) */
#define G_HOUSE_LEVEL_STRIDE 0x58u
#define G_HOUSE_LEVEL_U28 0x0070CDF4u /* @bin 0x004E8840 i32: 0 selects the other sentiment rule; 0x0045E4B0 divides by it */
#define G_HOUSE_LEVEL_U3C 0x0070CE08u /* @bin 0x004E8840 i32: added to BLD_CRIME_RISK per pass */

/* the ring of tiles around a building, per size (exe data) */
#define G_RING_OFFSETS 0x005CC9E0u /* @bin 0x00481000, 0x00481210 i32 grid offsets, 0 ends; 0x60 bytes per size */
#define G_RING_OFFSETS_STRIDE 0x60u
#define G_RING_COORDS 0x005CCE98u  /* @bin 0x00481210 i32 (x, y) per ring tile; 0xC0 bytes per size */
#define G_RING_COORDS_STRIDE 0xC0u

/* the festival's performers (exe data, 0x00461950) */
#define G_FESTIVAL_SHARES 0x005DA544u /* @bin 0x00461950 i32[4]: percent of the performers per kind of venue */
#define G_FESTIVAL_VENUES 0x005DA554u /* @bin 0x00461950 four 0-terminated lists of building types, 12 i32 each */
#define G_FESTIVAL_VENUES_STRIDE 0x30u

/* monuments: the object of each monument type (thiscall `this` of its functions; WinMain's set-up,
 * core/boot/monuments.c) */
#define G_MONUMENT_E1 0x006FD5D8u /* @bin 0x0040EB90 type 0xE1 */
#define G_MONUMENT_E3 0x006FD368u /* @bin 0x00417490 type 0xE3 */
#define G_MONUMENT_E4 0x006DE548u /* @bin 0x0046F200 type 0xE4 (0x004E6710) */
#define G_MONUMENT_E5 0x006F9320u /* @bin 0x00457AF0 type 0xE5 (0x00545A30) */
#define G_MONUMENT_E6 0x006DE650u /* @bin 0x00401AF0 type 0xE6 */
#define G_MONUMENT_EA 0x006F5648u /* @bin 0x00457AF0 type 0xEA (0x00558F00) */
#define G_MONUMENT_EB 0x006DE9E0u /* @bin 0x00457AF0 type 0xEB (0x00556230) */
#define G_MONUMENT_EC 0x006E9188u /* @bin 0x00457AF0 type 0xEC (0x0054F8A0) */
/* the pyramid (type 0xB7) tables, by BLD_MONUMENT_KIND (exe data) */
#define G_PYRAMID_PARTS 0x0056F7D8u       /* @bin 0x0046F200 i32: number of parts, G_PYRAMID_KIND != 5 */
#define G_PYRAMID_PARTS_KIND5 0x0056F7ECu /* @bin 0x0046F200 i32: the same, G_PYRAMID_KIND 5 */
#define G_PYRAMID_ENTRY_PART 0x005F3878u  /* @bin 0x004F30B0 i32: the part holding the entry, 0x3C0 per kind */
#define G_PYRAMID_ENTRY_PART_STRIDE 0x3C0u
#define G_PYRAMID_ENTRY_PART_KIND5 0x005F5498u /* @bin 0x004F30B0 the same, G_PYRAMID_KIND 5, 0x80 per kind */
#define G_PYRAMID_ENTRY_PART_KIND5_STRIDE 0x80u
#define G_PYRAMID_PART_CORNER 0x005E4EF8u /* @bin 0x004F30B0 i32 corner code, 8 bytes per part, 0x90 parts per kind */
#define G_PYRAMID_PART_CORNER_STRIDE 8u
#define G_PYRAMID_PART_CORNER_PARTS 0x90u
/* a monument object's fields */
#define MONOBJ_PART_TABLE 0x54u /* @bin 0x0040EB90 i32[13] per part: [0] the part's size */
#define MONOBJ_PART_ENTRIES 13

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_LIB_EXTRA2 0x00576278u /* core/building/rest_monument.c */
#define G_LIB_EXTRA 0x00576284u /* core/building/rest_monument.c */
#define G_LIB_MAIN_C5 0x00576290u /* core/building/rest_monument.c */
#define G_LIB_MAIN_C3 0x0057629Cu /* core/building/rest_monument.c */
#define G_LIB_MAIN_C2 0x005762A8u /* core/building/rest_monument.c */
#define G_LIB_MAIN_C1 0x005762B4u /* core/building/rest_monument.c */
#define G_LIB_MAIN_A5 0x005762C0u /* core/building/rest_monument.c */
#define G_LIB_MAIN_C4 0x005762CCu /* core/building/rest_monument.c */
#define G_LIB_MAIN_A4 0x005762D8u /* core/building/rest_monument.c */
#define G_LIB_MAIN_A3 0x005762E4u /* core/building/rest_monument.c */
#define G_LIB_MAIN_A2 0x005762F0u /* core/building/rest_monument.c */
#define G_LIB_MAIN_A1 0x005762FCu /* core/building/rest_monument.c */
#define G_FOOTPRINT_OFFSETS 0x005CC8C0u /* core/map/map_building.c: rows of 6 x (i32 grid offset, u8 edge bits, 3 pad), 0x30 bytes */
#define G_MONUMENT_MODELS 0x005D19C8u /* core/command/menu.c: 16-byte records by monument code */
#define G_COMPLEX_TABLE 0x005DF9B0u /* present/city/ghost_gate.c: 7 rows x 13 records of 0x14 bytes: +0 group, +4..+0x10 image by turn */
#define G_PYR_SKIP 0x005E68D8u /* core/city/monument_rating.c: bytes by size * 6 + level: 1 = a level of 3, else 6 */
#define G_HOUSE_LEVELS 0x0070CDCCu /* core/building/day_houses.c: 0x58 bytes per house level */
#define G_FESTIVAL_TENTH 0x00775678u /* core/command/orders.c */
#define G_MONUMENT_SLOTS 0x00784DE0u /* core/command/menu.c: i16[3]: the scenario's monument codes */
#define G_DAY_WATER_QUEUE 0x0078C414u /* core/city/day_buildings.c: i32[1000] [generic] */
#define G_GOODS_SUM 0x0078D590u /* core/building/day_houses.c: i32[12]: goods of merged houses */
#define G_MERGE_Y 0x0078D76Cu /* core/building/day_houses.c */
#define G_MERGE_X 0x0078D770u /* core/building/day_houses.c */
#define G_TICK_DAY 0x0078D7C4u /* core/sim/tick.c: day within the month, 0..15 (chunk 35) [generic] */
#define G_HOUSE_RESULT 0x0078D7E4u /* core/building/day_houses.c: i32: the evolution check's result (-1, 0, 1) */
#define G_HOUSE_ID 0x0078D838u /* core/building/day_houses.c: the house being evolved (global loop counter) */
#define G_MARKET_RECORDS 0x007900BCu /* core/figure/helpers_xa.c: records of 0xC4 bytes: +0 i16 building, +4 u8 in use [generic] */
#define G_STORAGE_ORDERS 0x007900C2u /* core/figure/helpers_xa.c: u8 per good of the record selected by building +0xC1 */
#define G_COUNTS 0x0096E644u /* core/figure/helpers_xa.c: 27 i32: candidates per good index */
#define G_DESIRABILITY_GRID 0x00AC2D4Cu /* core/city/desirability.h: i8 per tile, chunk 16 (written) */
#define G_RANDOM 0x00ACF85Cu /* core/map/route_x.c: u8 per tile */
#define G_DAY2_DESTROY_BITS 0x00B285CCu /* core/city/day2_flood.c: u8 per tile [generic] */
#define G_BUILDING_GRID 0x00B4E6FCu /* core/city/day2_flood.c: i16 per tile */
#define G_EDGE 0x00B67D1Cu /* core/command/clear.c: u8 */
#define G_IMAGE 0x00B7482Cu /* core/command/rotate.c: i32 per tile */
#define G_UNDO_TYPE 0x00BC9690u /* core/command/undo.c */
#define G_OUT_OF_MAP 0x00BC9734u /* core/building/day2_destroy.c */
#define G_FOOTPRINT_ROWS 0x00BC9738u /* core/command/construction_checks.c */
#define G_FOOTPRINT_COLS 0x00BC973Cu /* core/command/construction_checks.c */
#define G_PLACES 0x00EAAC44u /* core/figure/builders.c */
#define G_PLACE_COUNT 0x00EAAF58u /* core/figure/builders.c */
#define G_GOODS_COUNT 0x00F53DD8u /* core/command/orders_storage.c: i32 count, then i32 resources (granaries, bazaars) */
#define G_GOODS_LIST 0x00F53DDCu /* core/building/market.c: i32 per entry: a good (1..0x13), 0: none */
#define G_GRAPHICS_GFX 0x00F55BB0u /* core/city/day2_farm.c: `this` of 0x0053E590 [generic] */
#define G_DAY2_REDRAW_GROUPS 0x00F55BB4u /* core/figure/actions_b.h: sprite group base ids, 8 bytes per group [generic] */
#define G_FORT_IMAGE 0x00F5662Cu /* present/city/ghost_big.c: i32 the fort's first image */
#define G_PLAIN 0x00F56B54u /* core/building/rest_monument.c: the plain ground image */
#define G_IMAGE_FLAT_TOP 0x00F572ECu /* present/city/overlay.c: group base: images + 0x30 .. + 0x47 are drawn 0x96 higher */
/* --- end harvested --- */

#endif
