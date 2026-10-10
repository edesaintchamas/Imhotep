/* Imhotep - pending names of package p3 (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P3_CITY_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P3_CITY_H

/* record fields other packages own (building.h) */
#define BLD_MONUMENT_STAGES 0x54u /* @bin 0x00401540 i32: the monuments of 0x00401540: the stages done, 56 at the end (BLD_MONUMENT_KIND's offset) */
#define BLD_MONUMENT_WORK 0x60u /* @bin 0x00546790 u32: a part's work, summed over the parts of a part-built monument (BLD_MON_ANNOUNCED_60's offset) */
/* record fields other packages own (figure.h) */
#define FIG_WORKSITE 0x68u /* @bin 0x004F26A0 i16: a figure of type 0x55 at work: the building worked on (FIG_LEADER's offset) */
#define FIG_BUILDINGS_5C 0x5Cu /* @bin 0x00452200 i16[2]: a figure of type 0x55 at work (actions 0xF, 0x10, 0x24): two buildings (FIG_TARGET_HOUSE's offset) */

/* fields of tables other packages own */
#define MONMODEL_U0C 0x0Cu /* @bin 0x004F78A0 i16: a monument's weight (G_MONUMENT_MODELS entries; g_building.h) */
#define G_FIGURE_TYPE_STRIDE 0x38u /* @bin 0x00446A30 the stride of G_FIGURE_TYPES (g_figure.h) */
#define FIGTYPE_U04 0x04u /* @bin 0x00446A30 i32 of a G_FIGURE_TYPES entry (core/city/events_troops.c; g_figure.h) */
#define FIGTYPE_U1C 0x1Cu /* @bin 0x00446A30 i32 */
#define FIGTYPE_U20 0x20u /* @bin 0x00446EF9 i32 */
#define FIGTYPE_U24 0x24u /* @bin 0x00446A30 i32 */
#define FIGTYPE_U2C 0x2Cu /* @bin 0x00446A30 i32 */
#define FIGTYPE_U30 0x30u /* @bin 0x00446A30 i32 */

/* globals used most by other packages (the area of each: [header]) */
#define G_MONUMENT_LIST_CF 0x00586B08u /* @bin 0x004E5A80 10 entries of 16 bytes for type 0xCF: +0 stage, +4 figure type, +8 (core/building/monument.c) [g_building.h] */
#define G_MONUMENT_LIST_D0 0x0058FE48u /* @bin 0x004E5A80 18 entries of 16 bytes for the other type (0xD0) [g_building.h] */
#define G_U596088 0x00596088u /* @bin 0x0041FF40 the part list of type 0xD5, 16-byte entries (present/city/parts.c) [g_building.h] */
#define G_U596158 0x00596158u /* @bin 0x0041FF40 its end [g_building.h] */
#define G_U59EDF0 0x0059EDF0u /* @bin 0x004206D0 a part list, 16-byte entries (present/city/parts.c) [g_building.h] */
#define G_U59EFA0 0x0059EFA0u /* @bin 0x004206D0 its end [g_building.h] */
#define G_U5A8148 0x005A8148u /* @bin 0x00420A10 the part list of type 0xE6, 16-byte entries (present/city/parts.c) [g_building.h] */
#define G_U5A84C8 0x005A84C8u /* @bin 0x00420A10 its end [g_building.h] */
#define G_U5AD218 0x005AD218u /* @bin 0x00420D40 the part list of type 0xE3, 16-byte entries (present/city/parts.c) [g_building.h] */
#define G_U5AD3C8 0x005AD3C8u /* @bin 0x00420D40 its end [g_building.h] */
#define G_U5B0060 0x005B0060u /* @bin 0x00421040 the part list of type 0xE4, 16-byte entries (present/city/parts.c) [g_building.h] */
#define G_U5B0150 0x005B0150u /* @bin 0x00421040 its end [g_building.h] */
#define G_ANIM_LISTS 0x005BE510u /* @bin 0x004216D0 25 lists of three 16-byte entries (0x30 bytes each) stepped by 0x004216D0 [g_building.h] */
#define G_U5C2750 0x005C2750u /* @bin 0x0041FF20 the part list of type 0xD2, 16-byte entries (present/city/parts.c) [g_building.h] */
#define G_U5C28B0 0x005C28B0u /* @bin 0x0041FF20 its end [g_building.h] */
#define G_U5CD678 0x005CD678u /* @bin 0x00480F00 i32 grid offset per direction (core/city/day_buildings.c) [g_building.h] */
#define G_MONUMENT_PARTS 0x005E2AE4u /* @bin 0x004DF040 13 dwords (0x34 bytes) per part number: low byte the kind (core/building/monument.c) [g_building.h] */
#define G_U5F5C40 0x005F5C40u /* @bin 0x004F13D3 i32 per pyramid size: the stone work, G_PYRAMID_KIND 2 and 4 [g_building.h] */
#define G_U5F5C54 0x005F5C54u /* @bin 0x004F13D3 i32 per pyramid size: G_PYRAMID_KIND 3 [g_building.h] */
#define G_U5F5C68 0x005F5C68u /* @bin 0x004F13D3 i32 per pyramid size: G_PYRAMID_KIND 1 [g_building.h] */
#define G_U5F5C7C 0x005F5C7Cu /* @bin 0x004F13D3 i32 per pyramid size: G_PYRAMID_KIND 5 [g_building.h] */
#define G_PYRAMID_RAMP 0x005F5D08u /* @bin 0x004ECEA0 u32 grid offset of the ramp tile per pyramid size, 0: none (core/map/terrain_land.c) [g_building.h] */
#define G_MONUMENT_PART_KINDS 0x00601DE4u /* @bin 0x00541DE0 13 dwords (0x34 bytes) per part kind (core/building/monument.c) [g_building.h] */
#define G_HOUSE_LEVEL_U44 0x0070CE10u /* @bin the house level table, i32 (save.md §5: 0x0070CDCC + level * 0x58) */
#define G_HOUSE_LEVEL_U50 0x0070CE1Cu /* @bin the house level table, i32 (save.md §5: 0x0070CDCC + level * 0x58) */
#define G_HOUSE_LEVEL_U54 0x0070CE20u /* @bin the house level table, i32 (save.md §5: 0x0070CDCC + level * 0x58) */
#define G_ENTRY_POINT_X 0x00784C64u /* @bin 0x004DE8A9 i16: entry point (core/newgame/setup.c) [g_scenario.h] */
#define G_ENTRY_POINT_Y 0x00784C66u /* @bin 0x004DE8A9 i16 [g_scenario.h] */
#define G_EXIT_POINT_X 0x00784C68u /* @bin 0x004DE8A9 i16: exit point [g_scenario.h] */
#define G_EXIT_POINT_Y 0x00784C6Au /* @bin 0x004DE8A9 i16 [g_scenario.h] */
#define G_U784DC4 0x00784DC4u /* @bin 0x004AEAB0 three points (core/figure/actions_d4.c) [g_figure.h] */
#define G_U784DD0 0x00784DD0u /* @bin 0x004AEAB0 the end of G_U784DC4 [g_figure.h] */
#define G_QUEUE_TAIL 0x0078C40Cu /* @bin 0x00486A30 i32: tail of the ring queue (core/building/day_water.c) [g_building.h] */
#define G_QUEUE_HEAD 0x0078C410u /* @bin 0x00486A30 i32: head of the ring queue [g_building.h] */
#define G_HAIL_TIMER 0x0078D7A0u /* @bin 0x004E6EB0 i32: := 0x198 with the hail; the screen shakes while positive (core/frame/frame.c) [g_scenario.h] */
#define G_DEADLINE_YEAR 0x0078D7B8u /* @bin 0x004DE820 i32: deadline year (save.md §3 chunk 76) [g_scenario.h] */
#define G_WIND 0x0078D808u /* @bin 0x00455530 i32: the CRT rand & 7, a new year; the wind direction (core/building/day_state.c) [g_building.h] */
#define G_TILE_VISITS 0x00978D9Cu /* @bin 0x0047E250 u32 per tile: walker tile-visit counters (save.md §3 chunk 148) [g_figure.h] */
#define G_UBC96D8 0x00BC96D8u /* @bin 0x0047CC70 i32: the pattern (turn) found (core/building/map_add.c) [g_building.h] */
#define G_UBC96DC 0x00BC96DCu /* @bin 0x0047CC70 i32: the turn against the view (core/building/map_add.c) [g_building.h] */
#define G_FLOOD_STATE 0x00D386D4u /* @bin 0x0048BCAE i32: a mode 1..3 (flood state 1, core/building/building_extra.c) [g_figure.h] */
#define G_UEAAE8C 0x00EAAE8Cu /* @bin 0x004ED360 i32: the kind 0xDA / 0xDB / 0xDC [g_building.h] */
#define G_UF5736C 0x00F5736Cu /* @bin 0x004ED360 u32: a construction image (core/building/monument_obj.c) [g_building.h] */

#endif
