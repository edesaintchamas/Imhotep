/* Imhotep - named globals: the scenario and the city as a whole (the city record: layout/city.h).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Conventions: docs/naming.md. */
#ifndef IMHOTEP_STATE_LAYOUT_G_CITY_H
#define IMHOTEP_STATE_LAYOUT_G_CITY_H

/* the scenario */
#define G_DIFFICULTY 0x00E39080u   /* @bin 0x004DE820 i32: 0..4 (chunk 174, save.md §5) */
#define G_MISSION 0x00E38EA8u      /* @bin 0x004F46D0 i8: the campaign mission (0x7F: none) */
#define G_CUSTOM 0x00E38EBCu       /* @bin 0x004E8840 i32: != 0 a custom map */
#define G_PYRAMID_KIND 0x005E4EF4u /* @bin 0x004F30B0 i32: the scenario's pyramid kind 1..5 (5: the other tables) */
#define G_FLOODPLAIN 0x00D38360u   /* @bin 0x004BD330 the floodplain settings (`this`; docs/formats/state.md) */
#define G_REQUEST_EVENT 0x006057E4u /* @bin 0x004E0F90 i32: the request passed to 0x0044C1A0 */

/* sentiment (0x004E8840) */
#define G_SENTIMENT_BASE 0x005D199Cu /* @bin 0x004E8840 u8 by difficulty, 4 bytes each: an empty house's base */
#define G_SENTIMENT_TABLE 0x00EA1384u /* @bin 0x004E8840 i32[11] per CITY_TAX_RATE (loaded by 0x00541160) */

/* the gods */
#define G_GOD_MODE 0x007848E2u     /* @bin 0x004C47A0 i16 per god: 0 not in the city */

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
#define G_K_ONE 0x0056F520u /* core/city/monument_rating.c */
#define G_K_ZERO 0x0056F5A0u /* core/city/monument_rating.c */
#define G_K_HALF 0x0056F690u /* core/city/monument_rating.c */
#define G_K_SCALE 0x00570128u /* core/city/monument_rating.c */
#define G_MISSION_LISTS 0x005794B0u /* core/command/menu.c: @bin 0x004050DA 26 records of 0x1C bytes */
#define G_CHEAT_MISSION 0x0057961Cu /* core/command/menu.c */
#define G_PART_DESC_SETS 0x005BE9B8u /* present/city/parts.c: 0xFD descriptor sets of 0x28 bytes */
#define G_RATING_TABLE 0x005C9350u /* core/city/month_ratings.c: 11 records of 14 i32: index into 0x0078D5F0, 7 values, 6 limits */
#define G_DEFAULT_SHARES 0x005DA44Cu /* core/city/day_labour.c: (i32 category, i32 workers)[9] */
#define G_GLYPH_MAP 0x005E047Cu /* core/city/text_lookup.c: u8 per character: glyph number (0: none) */
#define G_CHEAT_FLAG 0x007027C5u /* core/command/menu.c */
#define G_DAY_BUILDINGS_TYPE_COUNT 0x00705AF0u /* core/city/day_buildings.c: i32 per building type: buildings [single+clash] */
#define G_DOCK_COUNT 0x00705C1Cu /* core/building/day2_docks.c: i32 */
#define G_U70E448 0x0070E448u /* core/city/day2_farm.c [unresolved] */
#define G_TILE_W 0x0070E45Cu /* core/frame/view.c: tile width - 1 (the step is +1) */
#define G_TRADE_ROUTE_LIMITS 0x0077646Cu /* core/city/goods.c: chunk 114, i32[route][36] */
#define G_EMPIRE_CITIES 0x0077DE00u /* core/city/goods.c: chunk 50, 61 x 0x6A */
#define G_START_YEAR 0x007848D8u /* core/io/save_load.c: chunk 75 */
#define G_SCENARIO_FLAG_10 0x00784D32u /* core/city/goods.c: i16, tested for good 10 (scenario info +0x45A) */
#define G_SCENARIO_FLAG_1C 0x00784D34u /* core/city/goods.c: i16, tested for good 0x1C (scenario info +0x45C) */
#define G_EVENTS 0x00785D70u /* core/city/day_city.h: the event list: i32 count, then records of 0x7C bytes */
#define G_INDUSTRY_WORKING 0x0078D64Cu /* present/ui/advisors/d_trade.c: i32 per resource */
#define G_U78D6DC 0x0078D6DCu /* core/command/trade.c: i32 per resource [unresolved] */
#define G_WORK_INDEX 0x0078D7F4u /* core/figure/actions_d2.c: @bin the candidate being examined */
#define G_HOUSE_LIST_COUNT 0x0078D83Cu /* core/city/day_labour.c */
#define G_HOUSE_LIST_INDEX 0x0078D840u /* core/city/day_labour.c */
#define G_HOUSE_LIST 0x0078E178u /* core/city/day_labour.c: i16 list of the used houses (0x004583F0) */
#define G_NETWORK2 0x00A69FDCu /* core/city/day_buildings.c */
#define G_ELEVATION_GRID 0x00AB623Cu /* core/command/construction_ui.c: u8 per tile */
#define G_FIGURES_END 0x00C8AD42u /* present/ui/info/queries.c: the loops' bound for +0x12 of the 0x184-byte figure records from 0x00BCD5F0 */
#define G_DAY_FLOOD_PERIOD 0x00D05718u /* core/city/day_flood.c: double: days between two floods [generic] */
#define G_UD05720 0x00D05720u /* core/city/month.c: `this` of 0x004BE640, 0x004BE770 [unresolved] */
#define G_R_POOL_INDEX 0x00D3B1C4u /* core/city/month_events.c */
#define G_R_POOL 0x00D3B1C8u /* core/city/month_events.c */
#define G_R_1_7 0x00D3B374u /* core/city/month_events.c */
#define G_R_IV2 0x00D3B380u /* core/city/month_events.c */
#define G_GLYPH_LAST 0x00D3B5C0u /* core/city/text_lookup.c: the last glyph looked up */
#define G_TEXT_GROUPS 0x00E34C38u /* core/city/text_lookup.c: 1000 group entries of 8 bytes: +0 offset into the text */
#define G_UE38DD9 0x00E38DD9u /* core/city/day_goals.c: i8: 1 won, -1 (0xFF) lost, 0 neither [unresolved] */
#define G_FONT_TITLE 0x00F56FB4u /* core/city/text_lookup.c: first glyph ids of three fonts */
#define G_FONT_LARGE2 0x00F56FBCu /* present/ui/info/figure.c: i32 the figure's name font */
#define G_IMAGE_ENTRIES 0x010D6A00u /* core/city/day2_farm.c: 0x40-byte records */
#define G_U0D6A0C 0x010D6A0Cu /* core/city/route_points.c: i16 width, i16 height of a city's picture, records of 64 bytes [unresolved] */
#define G_IMAGE_COUNT 0x0126517Cu /* present/ui/info/military.c: i32 image entries */
/* --- end harvested --- */

#endif
