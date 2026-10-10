/* Imhotep - pending names of package p8b (docs/naming.md "Pending names").
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Globals this package names that belong to another package's area header (or to an area header it
 * does not own): defined here, one per line with its evidence, until the lead merges them into the
 * owning header and empties this file. Only this package edits this file. @bin evidence: on each
 * name's line. */
#ifndef IMHOTEP_STATE_LAYOUT_PENDING_P8B_UI_INFO_H
#define IMHOTEP_STATE_LAYOUT_PENDING_P8B_UI_INFO_H

/* [g_city.h] */
#define G_RAMP_FOUND 0x00EAAE84u   /* @bin 0x004F5D90 u8 := 1 by the ramp walk 0x004ED360 when it reaches G_RAMP_TARGET (core/city/events_actions.c) */
#define G_RAMP_TARGET 0x00EAAE88u  /* @bin 0x004F5D90 i32 the building the ramp walk looks for */
#define G_HARVEST_MONTHS 0x0056F5B8u /* @bin 0x00489240 i32[10][4] the farms' harvest months by season row, -1 ends a row (core/city/day_buildings.c) */
#define G_U5D2A3C 0x005D2A3Cu      /* @bin 0x0050E3C0 i32, 8 bytes per walker type: its kind + 0x26 (core/city/day_city.c 0x00559FC0) */
#define G_U78D728 0x0078D728u      /* @bin 0x0050E3C0 i32 > 0: the house's entertainment need 0 is met (core/city/day_buildings.c: type 0x71) */

/* [g_building.h] the monuments' work lists: 16-byte entries (+0 stage or key, +4 resource / figure type, +8 the
 * work object: +0x0C amount, +0x12 done), END the bound (core/building/monument.c) */
#define G_WORKS_SUN_TEMPLE_END 0x00596150u
#define G_WORKS_E1_END 0x0059EF98u
#define G_WORKS_E6_END 0x005A84C0u
#define G_WORKS_E3_END 0x005AD3C0u
#define G_WORKS_E4_END 0x005B0148u

/* [g_figure.h] image groups (i32 the first image) */

/* [building.h] fields */
#define BLD_TOMB_PART_ROLE 0x60u  /* @bin 0x00513D40 i32 the parts of a tomb (types 0xE5, 0xEA..0xEC), records after the main one: role 1, 2 or 3 */
#define BLD_SHIPYARD_PROGRESS 0x2Cu /* @bin 0x0050318C i16 the shipyard's ship under construction, in percent of 0xA0 */

/* [figure.h] fields */
#define FIG_LOADS 0x9Cu            /* @bin 0x004FBF30 i32[4] the resources a figure carries (-1: none) */
#define FIG_LOAD_AMOUNTS 0xBCu     /* @bin 0x004FC040 i32[4] their amounts */

/* [city.h] fields (save.md section 5) */
#define CITY_PALACE_ID 0x2790u     /* @bin 0x00504C20 i32 the palace's building (save.md: unique buildings, palace x, y, offset, id) */
#define CITY_U284C 0x284Cu         /* @bin 0x0050E4D9 i16 1 or 3: the entertainment kinds cleared */
#define CITY_U28C4 0x28C4u         /* @bin 0x005036DD i16 (the fishing wharf's text 9) */
#define CITY_U2B18 0x2B18u         /* @bin 0x0053D48C i32 the city's mood, category 1 */
#define CITY_UNEMPLOYED 0x2BF8u    /* @bin 0x0053D638 i32 unemployed (save.md) */
#define CITY_U2C20 0x2C20u         /* @bin 0x0053CBCC i32 (save.md: +0x2C20..+0x2C60 taxed / untaxed population and tax due by class) */
#define CITY_U2C24 0x2C24u
#define CITY_U2C28 0x2C28u
#define CITY_U2C2C 0x2C2Cu
#define CITY_THEFT_THIS_YEAR 0x2C84u /* @bin 0x004FF400 i32 theft this year (save.md) */
#define CITY_U4384 0x4384u         /* @bin 0x0053D68C i32 the city's mood, category 7 */
#define CITY_U448C 0x448Cu         /* @bin 0x004FF400 i16 */
#define CITY_U4820 0x4820u         /* @bin 0x0053CC66 i32 != 0 tested by the phrases */

#endif
