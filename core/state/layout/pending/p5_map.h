/* Imhotep - pending names of package p5 (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P5_MAP_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P5_MAP_H

/* city record fields (layout/city.h, package P3) */
#define CITY_POPULATION_AGES 0x25C4u /* @bin 0x00451C50 i16[100]: summed into CITY_POPULATION at tick 0xB */
#define CITY_U273C 0x273Cu  /* @bin 0x00452370 i32: copied to CITY_U475C, then := 0, each month */
#define CITY_U2794 0x2794u  /* @bin 0x004E7E40 i16: decremented while > 0 every tick (record 1) */
#define CITY_U438C 0x438Cu  /* @bin 0x00452370 i32: += 1 each month */
#define CITY_POPULATION_HIGHEST 0x469Cu /* @bin 0x00451C50 i32: the highest CITY_POPULATION */
#define CITY_U471C 0x471Cu  /* @bin 0x00452370 i32: := 2 in months 0 and 6 */
#define CITY_U4720 0x4720u  /* @bin 0x00452370 i32: := 1 once G_MISSION_LIST_FLAGS[0] is set */
#define CITY_U4724 0x4724u  /* @bin 0x00452370 i32: := 1 once G_MISSION_LIST_FLAGS[6] is set (message 0x13E) */
#define CITY_U475C 0x475Cu  /* @bin 0x00452370 i32: := CITY_U273C each month */
#define CITY_U4760 0x4760u  /* @bin 0x00452370 i32: := 0 each month */
#define CITY_U4764 0x4764u  /* @bin 0x00452370 i32: := 0 each month */
#define CITY_U4990 0x4990u  /* @bin 0x00451C50 i32: 10 / 0 on alternate days of 0x00454E20 (tick 0x23) */
#define CITY_U44A0 0x44A0u  /* @bin 0x004D58B0 i32 := 0 after a campaign record is loaded */
#define CITY_U47A0 0x47A0u  /* @bin 0x004D5BC0 i32: x of the first building of type 0xD1, -1 none */
#define CITY_U47A4 0x47A4u  /* @bin 0x004D5BC0 i32: its y */
#define CITY_U49A0 0x49A0u  /* @bin 0x004BCA90 i32: figures in use of types 0x5D and 0x64 */

/* building record fields (layout/building.h, package P2) */
#define BLD_U68 0x68u /* @bin 0x00475FD0 i32: monument types 0xE5, 0xEA..0xEC: compared with 0x12 (the cliff neighbours);
                       * not BLD_MON_ANNOUNCED_68 (its types: 0xD5, 0xE1, 0xE3, 0xE4) */

/* goods lists (g_building.h / g_command.h) */

/* the scenario info (chunk 75, G_START_YEAR = its +0; g_scenario.h) */
#define G_SCENARIO_MAP_WIDTH 0x00784904u  /* @bin 0x004D52A0 i32 (+0x2C): copied to G_MAP_WIDTH (map.md §5) */
#define G_SCENARIO_MAP_HEIGHT 0x00784908u /* i32 (+0x30) */
#define G_SCENARIO_MAP_START 0x0078490Cu  /* i32 (+0x34): copied to G_MAP_START */
#define G_SCENARIO_MAP_STRIDE_EXTRA 0x00784910u /* i32 (+0x38): copied to G_MAP_STRIDE_EXTRA */

/* the pyramids (g_building.h) */
#define G_PYRAMID_PART_LISTS 0x005F6CD0u    /* @bin 0x004F3D70 {u32 list, i32 count} per BLD_MONUMENT_KIND: the parts
                                        * {i32 part, i32 offset} of a stage-3 pyramid */
#define G_PYRAMID_PART_LISTS_KIND5 0x005F6D58u /* @bin 0x004F3D70 the same when G_PYRAMID_KIND == 5 */

/* the image table (G_IMAGE_ENTRIES, 64-byte entries; g_city.h) */
#define IMAGE_ENTRY_WIDTH 0x0Cu  /* @bin 0x004D4720 i16 */
#define IMAGE_ENTRY_HEIGHT 0x0Eu /* @bin 0x004D4720 i16 */

/* the empire map objects (G_EMPIRE_OBJECTS, 0x62-byte records; g_scenario.h); +0: the kind (u8) */
#define EMPOBJ_IN_USE 0x01u  /* @bin 0x00442660 u8 */
#define EMPOBJ_WIDTH 0x08u   /* @bin 0x004426C0 u16: the image's width */
#define EMPOBJ_HEIGHT 0x0Au  /* @bin 0x004426C0 u16 */
#define EMPOBJ_IMAGE 0x0Cu   /* @bin 0x00442660 u16 */
#define EMPOBJ_U0E 0x0Eu     /* @bin 0x00442660 u16: kind 0: the image's offset in group 459 */

/* the empire routes (G_ROUTES, chunk 143, 0x144-byte records; g_scenario.h) */
#define ROUTE_POINTS 0x04u   /* @bin 0x004D4E20 u32: the heap block of the points (released before a save) */
#define ROUTE_U134 0x134u    /* @bin 0x004D4E20 i32: record 0's := 0 before a save */
#define ROUTE_U138 0x138u    /* @bin 0x004D4E20 i32 := 0 with the points */
#define ROUTE_U141 0x141u    /* @bin 0x004D4E20 u8: record 0's := 0 before a save */

/* ferry slots (G_FERRY_SLOTS, chunk 138, 0x2C-byte records, +0 in use; g_figure.h) */
#define FERRY_SLOT_FIGURES 0x04u /* @bin 0x004A3560 i32[4]: the figures listed */

/* the overlay (g_ui.h) */

/* ferry-landing queues (chunk 137): G_REC88 + 0xE0 * id, ids 1..49; the building's BLD_UCA holds the id */
#define REC88_BUILDING 0xDCu /* @bin 0x0046EFD0 i32: the building */

#endif
