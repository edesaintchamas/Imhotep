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

/* Package P3 (phase 10.3): named from the ports' comments and docs/formats (the @bin of each
 * line); G_U<address> where the meaning is not established. */
/* the tutorial steps: records of G_MISSION_LISTS, flags G_MISSION_LIST_FLAGS + step (0x00417AE0) */
#define G_MISSION_LIST_STRIDE 0x1Cu /* @bin 0x00417AE0 the stride of G_MISSION_LISTS */
#define MISSIONLIST_MISSION 0x00u /* @bin 0x004179F0 i32: the mission of the step (G_MISSION_LISTS entries) */
#define MISSIONLIST_COUNT 0x0Cu   /* @bin 0x004179F0 i32: the indexes in MISSIONLIST_INDEXES */
#define MISSIONLIST_INDEXES 0x10u /* @bin 0x004179F0 u32: pointer to the list of advisor indexes */
#define MISSIONLIST_MESSAGE 0x14u /* @bin 0x00417AE0 i32: the step's message */
#define G_K_100 0x0056F470u /* @bin 0x00476B0F f64 100.0 (core/map/terrain_land.c), 0x0040D940 */
#define G_U56F478 0x0056F478u /* @bin 0x00401540 f64: 1 / 56, the first part's stages (core/city/monument_rating.c) */
#define G_U56F4C8 0x0056F4C8u /* @bin 0x0040D940 f64: 1 / 21, the stages (core/city/monument_rating.c) */
#define G_U56F550 0x0056F550u /* @bin 0x00546790 f32 (fimul by the parts; 4.0f in present/ui/cityframe.c) */
#define G_U56F568 0x0056F568u /* @bin 0x00469B90 f32: the population factor of the spawn pass (0.02 in its port) */
#define G_K_0_01 0x0056F598u /* @bin 0x0047A049 f64 0.01 (core/map/terrain_flood.c), 0x004BE7DB */
#define G_U56F5B8 0x0056F5B8u /* @bin 0x0045EA30 i32 month table: ten rows of four months, -1 ends a row (core/city/day_buildings.c) */
#define G_U56F688 0x0056F688u /* @bin 0x004BEA51 f64: the sixths of a flood day (core/city/day_flood.c) */
#define G_K_100F 0x0056F6DCu /* @bin 0x004E5F3C f32 100.0 (present/city/parts.c), 0x00546790 */
#define G_U56F720 0x0056F720u /* @bin 0x004C47A0 i32[5]: a god's blessing / wrath table, mood >= 0x5A (core/city/day_city.c) */
#define G_U56F734 0x0056F734u /* @bin 0x004C47A0 i32[5]: mood >= 0x50 */
#define G_U56F748 0x0056F748u /* @bin 0x004C47A0 i32[5]: mood >= 0x46 */
#define G_U56F75C 0x0056F75Cu /* @bin 0x004C47A0 i32[5]: mood > 0x32 */
#define G_U56F784 0x0056F784u /* @bin 0x004C47A0 i32[5]: mood > 0x1E */
#define G_U56F798 0x0056F798u /* @bin 0x004C47A0 i32[5]: mood > 0x14 */
#define G_U56F7AC 0x0056F7ACu /* @bin 0x004C47A0 i32[5]: mood > 0xA */
#define G_U56F7C0 0x0056F7C0u /* @bin 0x004C47A0 i32[5]: the lowest moods */
#define G_U56F7F8 0x0056F7F8u /* @bin 0x004F171A i32 per pyramid size: out[3] of the earlier stages, other kinds (core/city/monument_rating.c) */
#define G_U56F80C 0x0056F80Cu /* @bin 0x004F171A i32 per pyramid size: the same, G_PYRAMID_KIND 1 */
#define G_U56F820 0x0056F820u /* @bin 0x004F171A i32 per pyramid size: the same, G_PYRAMID_KIND 5 */
#define G_U56F82C 0x0056F82Cu /* @bin 0x004F13D3 i32 per pyramid size: the extra stage work, G_PYRAMID_KIND 1 */
#define G_U5701E0 0x005701E0u /* @bin 0x00546790 f64: 99.9, above it 100 (core/city/monument_rating.c) */
#define G_U596084 0x00596084u /* @bin 0x004204A0 16-byte entries to 0x00596154 (core/city/day2_monument.c) */
#define G_U596154 0x00596154u /* @bin 0x004204A0 the end of G_U596084 */
#define G_U5C92F8 0x005C92F8u /* @bin 0x004E6EB0 char[]: "audio/ambient/hailstorm.mp3" */
#define G_U5C9348 0x005C9348u /* @bin 0x004269B0 i32: the lowest culture coverage below 100 (core/city/month_ratings.c) */
#define G_RINGS 0x005C9BA8u /* @bin 0x00481AA0 the rings of sizes 2..6, static data (core/city/ring_tables.c gives each table) */
#define G_U5CCCC8 0x005CCCC8u /* @bin 0x0047CBD0 i32[4]: grid offsets of a 2x2 block */
#define G_U5D19B0 0x005D19B0u /* @bin 0x00446A30 i32 per difficulty: an invasion percent (core/city/events_troops.c) */
#define G_U5D2A38 0x005D2A38u /* @bin 0x0044E9C0 u32, 8 bytes per kind (core/city/events_handlers.c) */
#define G_U5D2A3C 0x005D2A3Cu /* @bin 0x00559FC0 i32, 8 bytes per walker kind (present/ui/info/house.c) */
#define G_U5D2E8C 0x005D2E8Cu /* @bin 0x004433C3 i32[6]: the kinds valued by population (core/city/day_city.c) */
#define G_U5D9A40 0x005D9A40u /* @bin 0x0044BA90 i32: the event being run (core/city/month.c) */
#define G_U5D9C18 0x005D9C18u /* @bin 0x00449370 u8 per event type */
#define G_U5D9C38 0x005D9C38u /* @bin 0x00449380 u8 per event type */
#define G_EVENT_TYPE_FLAGS 0x005D9C78u /* @bin 0x004493C0 u8 per event type (save.md §3, chunk 53) */
#define G_U5D9C98 0x005D9C98u /* @bin 0x004493D0 i32 per event type: the start of its range */
#define G_U5D9D10 0x005D9D10u /* @bin 0x004493E0 i32 per event type: the end of its range */
#define G_U5DA174 0x005DA174u /* @bin 0x0044BB10 char[]: "ERR Could not add event" */
#define G_U5DA330 0x005DA330u /* @bin 0x00452160 i32[10] per difficulty: kingdom rating change in debt */
#define G_U5DA498 0x005DA498u /* @bin 0x0045A140 weight table (0x1C bytes per group) of building types 0x4D..0x4F */
#define G_U5DA4F0 0x005DA4F0u /* @bin 0x0045A140 weight table of building types 0x54..0x57, 0xB8, 0xBB..0xBD */
#define G_U5DBDA8 0x005DBDA8u /* @bin 0x0047CBD0 i32 patterns, 4 dwords each (core/city/day2_misc.c) */
#define G_U5DBDE8 0x005DBDE8u /* @bin 0x0047CC70 i32 grid offsets of the tiles in front, 2 per pattern */
#define G_U5DBE08 0x005DBE08u /* @bin 0x0047CC70 the same, second row */
#define G_U5DBE28 0x005DBE28u /* @bin 0x0047CC70 the same, third row */
#define G_U5DF75C 0x005DF75Cu /* @bin 0x004BEA51 i32: the receding flood's step */
#define G_U5DF760 0x005DF760u /* @bin 0x004BEA51 i32: the last receding step applied */
#define G_U5E4940 0x005E4940u /* @bin 0x004E7200 char[]: "audio/ambient/frogs.mp3" */
#define G_U5E4958 0x005E4958u /* @bin 0x004E7573 char[]: the format of the frog log line */
#define G_U5E4990 0x005E4990u /* @bin 0x004E7700 char[]: "audio/ambient/locusts.mp3" */
#define G_U5E49AC 0x005E49ACu /* @bin 0x004E8720 i32 per decade: the births percent (core/city/day_labour.c) */
#define G_DEATH_PCT 0x005E49D4u /* @bin 0x004E8650 i32 by decade and health / 10: the death percent (save.md §5) */
#define G_U5E4B8C 0x005E4B8Cu /* @bin 0x004E8350 10 (limit, bonus) i32 pairs by population */
#define G_U5E4BDC 0x005E4BDCu /* @bin 0x004EA2C0 the window opened on a win (core/city/day_goals.c) */
#define G_U5E4BF4 0x005E4BF4u /* @bin 0x004EA2C0 the other window */
#define G_U5E4C0C 0x005E4C0Cu /* @bin 0x004EA2C0 the window opened on a loss */
#define G_U5E657C 0x005E657Cu /* @bin 0x004F13D3 i32, 8 bytes per part, 0x24 parts per size: G_PYRAMID_KIND 5 (core/city/monument_rating.c) */
#define G_U5F5C88 0x005F5C88u /* @bin 0x004F13D3 i32 per pyramid size: the total, G_PYRAMID_KIND 1..4 */
#define G_U5F5C9C 0x005F5C9Cu /* @bin 0x004F13D3 i32 per pyramid size: the total, G_PYRAMID_KIND 5 */
#define G_U6DEA2C 0x006DEA2Cu /* @bin 0x00452200 a flag */
#define G_U6E91D4 0x006E91D4u /* @bin 0x00452200 a flag */
#define G_U6F5694 0x006F5694u /* @bin 0x00452200 a flag */
#define G_U6F936C 0x006F936Cu /* @bin 0x00452200 a flag cleared with no monument (core/city/month.c) */
#define G_U705734 0x00705734u /* @bin 0x004269B0 i32: the culture coverage sum (core/city/month_ratings.c) */
#define G_U7057B4 0x007057B4u /* @bin 0x00468170 i32: a service building count, x 700 (core/city/day_city.c; counts 0x007057B4..0x007057D0) */
#define G_U7057B8 0x007057B8u /* @bin 0x00468170 i32: a service building count, x 400 */
#define G_U7057BC 0x007057BCu /* @bin 0x00468170 i32: a service building count, x 5000 */
#define G_U7057C0 0x007057C0u /* @bin 0x00468170 i32: a service building count, x 1200 */
#define G_U705808 0x00705808u /* @bin 0x00468170 i32: the people served, x 300 (present/ui/advisors/c_pages.c) */
#define G_U705810 0x00705810u /* @bin 0x00468170 i32: the people served, x 800 */
#define G_U705AC4 0x00705AC4u /* @bin 0x00468170 i32: a service building count, x 7500 */
#define G_RINGS_1X1_RING6 0x0070BE3Cu /* @bin 0x00481AA0 the 1x1 rings (dx, dy byte pairs), built at start-up by 0x0046F8D0 in .data: ring 6 first, ring 1 at +0x140 */
#define G_HOUSE_MESSAGE_148 0x0070E124u /* @bin 0x004302B0 i32: the house of the pending message 0x148 (0x00430060) */
#define G_HOUSE_MESSAGE_146 0x0070E128u /* @bin 0x004302B0 i32: the house of the pending message 0x146 (0x00430040) */
#define G_HOUSE_MESSAGE_147 0x0070E12Cu /* @bin 0x004302B0 i32: the house of the pending message 0x147 */
#define G_HOUSE_MESSAGE_148_ON 0x0070E130u /* @bin 0x004302B0 i32: message 0x148 pending */
#define G_HOUSE_MESSAGE_146_ON 0x0070E134u /* @bin 0x004302B0 i32: message 0x146 pending */
#define G_HOUSE_MESSAGE_147_ON 0x0070E138u /* @bin 0x004302B0 i32: message 0x147 pending */
#define G_U775814 0x00775814u /* @bin 0x00443330 i32[35]: a value per kind 1..35 (core/city/day_city.c) */
#define G_TRADE_RECOMMENDS 0x007758A0u /* @bin 0x00541310 trade_recommends.txt: 4 records of 6 i32 (core/boot/steps.c), by population */
#define G_INVASION_SEQUENCE 0x00775900u /* @bin 0x00446A30 i16: invasion id counter (save.md §3 chunk 126) */
#define G_U775930 0x00775930u /* @bin 0x0044F0A0 records of 0x90 bytes (core/city/month.c) */
#define G_U784B60 0x00784B60u /* @bin 0x004F6C6B i16: no population goal when set (present/ui/advisors/b_ratings.c) */
#define G_POINTS_X 0x00784B74u /* @bin 0x00480C60 i16[8]: x of 8 points, > 0 in use (core/newgame/figures.c) */
#define G_POINTS_Y 0x00784B84u /* @bin 0x00480C60 i16[8]: y of the 8 points */
#define G_MAP_POINTS_X 0x00784BC0u /* @bin 0x004D52A0 i16[16]: x of 16 map points (core/state/scenario_start.c) */
#define G_MAP_POINTS_Y 0x00784BE0u /* @bin 0x004D52A0 i16[16]: y of the 16 map points */
#define G_GOAL_CULTURE 0x00784C24u /* @bin 0x004EA2C0 i32: the culture goal (save.md §5 "Ratings") */
#define G_GOAL_PROSPERITY 0x00784C28u /* @bin 0x004EA2C0 i32 */
#define G_GOAL_MONUMENT 0x00784C2Cu /* @bin 0x004EA2C0 i32 */
#define G_GOAL_KINGDOM 0x00784C30u /* @bin 0x004EA2C0 i32 */
#define G_GOAL_HOUSES 0x00784C34u /* @bin 0x004F6D0A i32: the houses goal, kind G_GOAL_HOUSE_LEVEL (present/ui/advisors/b_ratings.c) */
#define G_GOAL_HOUSE_LEVEL 0x00784C38u /* @bin 0x004F6D0A i32 */
#define G_GOAL_CULTURE_ON 0x00784C3Cu /* @bin 0x004EA2C0 u8: the culture goal is set */
#define G_GOAL_PROSPERITY_ON 0x00784C3Du /* @bin 0x004EA2C0 u8 */
#define G_GOAL_MONUMENT_ON 0x00784C3Eu /* @bin 0x004EA2C0 u8 */
#define G_GOAL_KINGDOM_ON 0x00784C3Fu /* @bin 0x004EA2C0 u8 */
#define G_GOAL_TIME 0x00784C48u /* @bin 0x004DE9C8 i32: time left to the victory (present/sidebar/sidebar.c) */
#define G_GOAL_DEADLINE 0x00784C50u /* @bin 0x004DE9C8 i32: a deadline (present/sidebar/sidebar.c) */
#define G_GOAL_POPULATION_ON 0x00784C58u /* @bin 0x004F6C6B i32: the population goal is set */
#define G_GOAL_POPULATION 0x00784C5Cu /* @bin 0x004F6C6B i32: the population goal */
#define G_U784C60 0x00784C60u /* @bin 0x0044BCA0 i16: -1 stops the events (core/city/day_basic.c) */
#define G_U784C62 0x00784C62u /* @bin 0x0044BCA0 i16: -1 stops the events */
#define G_LOAN_PCT 0x00784C94u /* @bin 0x004460B0 i32: the debt loan, percent of G_TREASURY_PCT (core/city/day_city.c) */
#define G_U784C98 0x00784C98u /* @bin 0x0046EA8B i32: a goal year from the start (core/city/month_ratings.c) */
#define G_U784C9C 0x00784C9Cu /* @bin 0x0046EA8B i32: a goal year */
#define G_U784CA0 0x00784CA0u /* @bin 0x0046EA8B i32: a goal year */
#define G_INTEREST_RATE 0x00784DDCu /* @bin 0x0045A350 u32: the interest rate on debt (core/city/month.c) */
#define G_GOAL_TARGETS 0x00784DECu /* @bin 0x004EA930 i32 per kind: the goals the counters of G_COUNTERS reach (core/city/day_goals.h) */
#define G_COUNTERS 0x00784E7Cu /* @bin 0x004EA930 i32 per kind: the tested counters (core/city/day_goals.h) */
#define G_U78A624 0x0078A624u /* @bin 0x0047F1D0 u8: drops the pending action (core/city/day_basic.c) */
#define G_U78A634 0x0078A634u /* @bin 0x0044BA90 u8: := 1 while the events run (core/city/month.c) */
#define G_U78C400 0x0078C400u /* @bin 0x00486A30 i32: tiles counted by the network walk (core/city/day_buildings.c) */
#define G_U78C404 0x0078C404u /* @bin 0x00486A30 i32: first-neighbour steps, 30 a round */
#define G_U78C408 0x0078C408u /* @bin 0x00486A30 i32: the mark of the second grid */
#define G_U78D5C8 0x0078D5C8u /* @bin 0x0046A060 i32 := 0 (0x0078D5C8..0x0078D5EC: core/newgame/reset.c) */
#define G_U78D5CC 0x0078D5CCu /* @bin 0x0046A060 i32 := 0 */
#define G_U78D5DC 0x0078D5DCu /* @bin 0x0046A060 i32 := 0 */
#define G_U78D5E0 0x0078D5E0u /* @bin 0x0046A060 i32 := 0 */
#define G_COVERAGE 0x0078D5F0u /* @bin 0x00468170 i32[5]: the advisors' coverage percents (core/city/day_city.c) */
#define G_U78D604 0x0078D604u /* @bin 0x00468170 i32: a coverage of the young people, x 300 (present/ui/advisors/c_pages.c) */
#define G_U78D608 0x0078D608u /* @bin 0x00468170 i32: a coverage, x 800 */
#define G_GOD_COVERAGE_AVERAGE 0x0078D60Cu /* @bin 0x004683FF i32: the gods' average coverage */
#define G_U78D610 0x0078D610u /* @bin 0x00468170 i32: an output of houses_access_ratios (core/city/day_city.c) */
#define G_U78D614 0x0078D614u /* @bin 0x00468170 i32: an output of houses_access_ratios */
#define G_U78D618 0x0078D618u /* @bin 0x00468170 i32: an output of houses_access_ratios */
#define G_U78D61C 0x0078D61Cu /* @bin 0x00468170 i32: an output of houses_access_ratios */
#define G_GOD_COVERAGE 0x0078D620u /* @bin 0x00468170 i32[5]: the gods' coverage */
#define G_LOCUST_TIMER 0x0078D79Cu /* @bin 0x004E6EB0 i32: locust plague timer, -1 armed (save.md §3 chunk 40) */
#define G_U78D844 0x0078D844u /* @bin 0x00461A20 i32 per resource (core/city/day_buildings.c) */
#define G_STORAGE_BUILDINGS 0x0078D8D4u /* @bin 0x0045DE80 i16 list of storage buildings, 99 at most (core/figure/actions_a2.c) */
#define G_STORAGE_LIST_COUNT 0x0078D9A0u /* @bin 0x0045DE80 i32: entries of G_STORAGE_LIST */
#define G_U978D80 0x00978D80u /* @bin 0x00469B90 i32: day counter, every 10 days */
#define G_FERTILITY_GRID 0x00A5D4CCu /* @bin 0x00489050 u8 per tile: fertility (core/io/map_file.h chunk 15) */
#define G_UBCA344 0x00BCA344u /* @bin 0x0047E290 i32: day counter, every 20 days */
#define G_HARVEST_SIZE 0x00BCA34Cu /* @bin 0x004883D0 i32: the farms' harvest size (core/boot/ctors.c) */
#define G_UE38EC3 0x00E38EC3u /* @bin 0x004F5340 u8: an option (Options 9, core/command/menu_items.c) */
#define G_UE3907C 0x00E3907Cu /* @bin 0x004EA2C0 i32: toggles which win window opens */
#define G_UE39084 0x00E39084u /* @bin 0x004C47A0 i32 (core/city/day_city.c) */
#define G_UEA1380 0x00EA1380u /* @bin 0x004EA920 u8: := 0x004183D0() (present/ui/frontend/victory.c) */
#define G_UEA17FC 0x00EA17FCu /* @bin 0x004EA2C0 u8 (present/ui/frontend/victory.c) */
#define G_UEAAC2C 0x00EAAC2Cu /* @bin 0x004E81D0 u8: passed to house_mood_month, then cleared */
#define G_UEAAE84 0x00EAAE84u /* @bin 0x004ED360 u8: the building G_UEAAE88 was reached */
#define G_UEAAE88 0x00EAAE88u /* @bin 0x004ED360 i32: the building looked for */
#define G_UEAAE90 0x00EAAE90u /* @bin 0x004ED360 i32: the tile found (core/building/monument.c) */
#define G_UEAAE94 0x00EAAE94u /* @bin 0x004ED360 i32 := 2 */
#define G_UEAAE98 0x00EAAE98u /* @bin 0x004ED360 i32: the steps, at most 20 */
#define G_UEAAEA0 0x00EAAEA0u /* @bin 0x004ED360 i32: the step eastwards, x 2 */
#define G_UEAAEA4 0x00EAAEA4u /* @bin 0x004ED360 i32: the first tile */
#define G_GOODS_AVAILABLE 0x00F53E70u /* @bin 0x005596D0 i32 list of available goods, 1..35 (core/city/goods.h) */
#define G_UF55B94 0x00F55B94u /* @bin 0x004E6EB0 i32 := 1 with an ambient track (core/sim/tick.c) */
#define G_UF560A4 0x00F560A4u /* @bin 0x00488B1E i32: an image base of the farm overlay */
#define G_UF560AC 0x00F560ACu /* @bin 0x00488B1E i32: an image base of the farm overlay */
#define G_UF56444 0x00F56444u /* @bin 0x0044E9C0 i32: an image base (core/city/events_handlers.c) */
#define G_UF564DC 0x00F564DCu /* @bin 0x00488C20 i32: an image base for type 0xC3 */
#define G_UF56544 0x00F56544u /* @bin 0x00488840 i32: the large farm's image base */
#define G_UF56764 0x00F56764u /* @bin 0x00488C20 i32: an image base */
#define G_UF5693C 0x00F5693Cu /* @bin 0x0044E9C0 i32: an image base */
#define G_UF56954 0x00F56954u /* @bin 0x0044E9C0 i32: an image base */
#define G_UF56964 0x00F56964u /* @bin 0x0044E9C0 i32: an image base */
#define G_UF56B24 0x00F56B24u /* @bin 0x00488690 i32: the small farm's image base */

/* harvested names (wave 0) checked by P3 against their uses in core/city; evidence: the defining
 * port named on each line */
#define G_K_ONE 0x0056F520u /* core/city/monument_rating.c */
#define G_K_ZERO 0x0056F5A0u /* core/city/monument_rating.c */
#define G_K_HALF 0x0056F690u /* core/city/monument_rating.c */
#define G_K_SCALE 0x00570128u /* core/city/monument_rating.c */
#define G_MISSION_LISTS 0x005794B0u /* core/command/menu.c: @bin 0x004050DA 26 records of 0x1C bytes */
#define G_PART_DESC_SETS 0x005BE9B8u /* present/city/parts.c: 0xFD descriptor sets of 0x28 bytes */
#define G_RATING_TABLE 0x005C9350u /* core/city/month_ratings.c: 11 records of 14 i32: index into 0x0078D5F0, 7 values, 6 limits */
#define G_DEFAULT_SHARES 0x005DA44Cu /* core/city/day_labour.c: (i32 category, i32 workers)[9] */
#define G_GLYPH_MAP 0x005E047Cu /* core/city/text_lookup.c: u8 per character: glyph number (0: none) */
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

/* --- harvested: names the ports gave these addresses in local #defines (tools/naming/harvest.py).
 * @bin evidence: the defining port (file) and its comment. A package agent checks each name it
 * uses, and moves it above this block once checked. --- */
/* P3: core/city writes these three from their tables: G_MISSION_LISTS + G_MISSION_LIST_STRIDE * 13
 * (tutorial step 13, 0x00417AE0), G_MISSION_LIST_FLAGS + 13, FIGURE_ADDR(FIGURE_COUNT) + FIG_TYPE */
#define G_CHEAT_MISSION 0x0057961Cu /* core/command/menu.c */
#define G_CHEAT_FLAG 0x007027C5u /* core/command/menu.c */
#define G_FIGURES_END 0x00C8AD42u /* present/ui/info/queries.c: the loops' bound for +0x12 of the 0x184-byte figure records from 0x00BCD5F0 */
/* --- end harvested --- */

#endif
